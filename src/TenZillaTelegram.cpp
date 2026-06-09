/*
 * Telegram Bot API via FastBot2 (GyverLibs/FastBot2) для send();
 * для sendTestWithError — разбор результата FastBot2 (fb::Result), чтобы показывать
 * "Telegram: Unauthorized" / "chat not found" и т.д.
 */
#define FB_NO_OTA
#define FB_NO_UNICODE
#define FB_NO_FILE
// #define FB_DYNAMIC
#include <FastBot2.h>

#include "TenZillaTelegram.h"
#include "TenZillaConfig.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

static FastBot2 s_bot;

// Неблокирующая очередь сообщений для обычных уведомлений (send()).
// Сами HTTP-запросы выполняются в processQueue() из главного цикла.
static const size_t TG_QUEUE_SIZE = 4;
static const size_t TG_MSG_MAX_LEN = 160;
static char s_queue[TG_QUEUE_SIZE][TG_MSG_MAX_LEN];
static bool s_queueUsed[TG_QUEUE_SIZE] = { false, false, false, false };
static unsigned long s_lastSendMs = 0;
static const unsigned long TG_SEND_MIN_INTERVAL_MS = 1000;

static int tg_enqueue(const char* msg) {
  if (!msg || !msg[0]) return -1;
  for (size_t i = 0; i < TG_QUEUE_SIZE; i++) {
    if (!s_queueUsed[i]) {
      strncpy(s_queue[i], msg, TG_MSG_MAX_LEN - 1);
      s_queue[i][TG_MSG_MAX_LEN - 1] = '\0';
      s_queueUsed[i] = true;
      return (int)i;
    }
  }
  // Очередь заполнена — тихо отбрасываем сообщение, чтобы не блокировать систему
  return -1;
}

static bool tg_dequeue(String& out) {
  for (size_t i = 0; i < TG_QUEUE_SIZE; i++) {
    if (s_queueUsed[i]) {
      out = String(s_queue[i]);
      s_queueUsed[i] = false;
      s_queue[i][0] = '\0';
      return true;
    }
  }
  return false;
}

static void urlEncode(String& out, const char* s) {
  out = "";
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.')
      out += (char)c;
    else if (c == ' ')
      out += '+';
    else {
      char hex[4];
      snprintf(hex, sizeof(hex), "%%%02X", c);
      out += hex;
    }
  }
}

static void extractDescription(const String& json, String& out) {
  out = "";
  int i = json.indexOf("\"description\":\"");
  if (i < 0) return;
  i += 14;
  int j = i;
  while (j < (int)json.length()) {
    if (json[j] == '\\' && j + 1 < (int)json.length()) { j += 2; continue; }
    if (json[j] == '"') break;
    j++;
  }
  if (j > i) out = json.substring(i, j);
}

void TenZillaTelegram::begin() {
  (void)0;
}

void TenZillaTelegram::send(const char* message) {
  // Обычные уведомления не отправляем сразу, а ставим в очередь,
  // чтобы фактический HTTP-запрос выполнялся в безопасном месте (главный цикл).
  tg_enqueue(message);
}

void TenZillaTelegram::processQueue() {
  // Минимальный интервал между отправками, чтобы не подвешивать WiFi/HTTP
  unsigned long now = millis();
  if (now - s_lastSendMs < TG_SEND_MIN_INTERVAL_MS) return;

  String msg;
  if (!tg_dequeue(msg)) return;

  TenZillaSettings s = TenZillaConfig::get();
  if (!s.tgEnabled || s.tgBotToken[0] == '\0' || s.tgChatId[0] == '\0') return;
  if (WiFi.status() != WL_CONNECTED) return;

  String chatIdStr = String(s.tgChatId);
  chatIdStr.trim();
  if (chatIdStr.length() == 0) return;

  s_bot.setToken(String(s.tgBotToken));

  fb::Message m;
  m.chatID = strtoll(chatIdStr.c_str(), nullptr, 10);
  m.text = msg;
  (void)s_bot.sendMessage(m);
  s_lastSendMs = now;
}

bool TenZillaTelegram::sendTest(const char* message) {
  String err;
  return sendTestWithError(message, err);
}

bool TenZillaTelegram::sendTestWithError(const char* message, String& errOut) {
  errOut = "";
  if (!message || !message[0]) {
    errOut = "Пустое сообщение";
    return false;
  }

  TenZillaSettings s = TenZillaConfig::get();
  if (s.tgBotToken[0] == '\0') {
    errOut = "Укажите токен бота";
    return false;
  }
  if (s.tgChatId[0] == '\0') {
    errOut = "Укажите Chat ID";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    errOut = "WiFi не подключен";
    return false;
  }

  // Trim chat ID to avoid "chat not found" from leading/trailing spaces
  String chatIdStr = String(s.tgChatId);
  chatIdStr.trim();
  if (chatIdStr.length() == 0) {
    errOut = "Укажите Chat ID";
    return false;
  }

  // Test send via FastBot2 to the configured chat, with error extraction from fb::Result.
  s_bot.setToken(String(s.tgBotToken));

  fb::Message m;
  m.chatID = strtoll(chatIdStr.c_str(), nullptr, 10);
  m.text = String(message);

  fb::Result res = s_bot.sendMessage(m, true);

  // FastBot2 может вернуть пустой Result даже при успешной отправке,
  // поэтому ориентируемся только на isError().
  if (!res.isError()) {
    return true;
  }

  auto errText = res.getError();
  String desc = errText.toString();
  if (desc.length() > 0) {
    errOut = "Telegram: " + desc;
    // Hint for common "chat not found" / "bad request"
    if (desc.indexOf("chat not found") >= 0 || desc.indexOf("Bad Request") >= 0) {
      errOut += ". Напишите боту /start и проверьте Chat ID (например @userinfobot).";
    }
  } else {
    errOut = "Telegram: неизвестная ошибка";
  }

  return false;
}
