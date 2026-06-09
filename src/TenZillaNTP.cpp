#include "TenZillaNTP.h"
#include "TenZillaConfig.h"
#include <Preferences.h>

// Статические переменные
char TenZillaNTP::ntpServer[64] = "pool.ntp.org";
// По умолчанию - московский часовой пояс без перехода на летнее время
// Пример POSIX TZ: "MSK-3"
char TenZillaNTP::timezoneStr[64] = "MSK-3";
unsigned long TenZillaNTP::ntpInterval = 3600; // 1 час по умолчанию
unsigned long TenZillaNTP::lastNTPUpdate = 0;
bool TenZillaNTP::timeSynced = false;
unsigned long TenZillaNTP::lastSyncTime = 0;
unsigned long TenZillaNTP::systemStartTime = 0;

Preferences ntpPreferences;

// Вспомогательная функция для парсинга POSIX TZ строки и вычисления смещения
// В POSIX TZ: отрицательное число означает восточное полушарие (UTC+)
// Например: "MSK-3" = UTC+3 часа, "EST5" = UTC-5 часов
// Возвращает смещение в секундах от UTC (положительное = восточнее UTC)
static int parseTimezoneOffset(const char* tz) {
  if (tz == nullptr || strlen(tz) == 0) return 0;
  
  String tzStr = String(tz);
  tzStr.trim();
  
  // Ищем последний знак минус или плюс
  int lastDash = tzStr.lastIndexOf('-');
  int lastPlus = tzStr.lastIndexOf('+');
  int signPos = (lastDash > lastPlus) ? lastDash : lastPlus;
  
  int offsetHours = 0;
  int offsetMinutes = 0;
  
  if (signPos >= 0) {
    // Есть знак - извлекаем число после него
    String numPart = tzStr.substring(signPos + 1);
    // Убираем все после первого нецифрового символа (кроме двоеточия и точки)
    int colonPos = numPart.indexOf(':');
    if (colonPos >= 0) {
      offsetHours = numPart.substring(0, colonPos).toInt();
      String minPart = numPart.substring(colonPos + 1);
      int dotPos = minPart.indexOf('.');
      if (dotPos >= 0) minPart = minPart.substring(0, dotPos);
      offsetMinutes = minPart.toInt();
    } else {
      // Ищем конец числа
      int endPos = 0;
      while (endPos < numPart.length() && (isdigit(numPart[endPos]) || numPart[endPos] == '.')) {
        endPos++;
      }
      String hourStr = numPart.substring(0, endPos);
      int dotPos = hourStr.indexOf('.');
      if (dotPos >= 0) {
        offsetHours = hourStr.substring(0, dotPos).toInt();
        // Дробная часть - это минуты (например 5.5 = 5 часов 30 минут)
        float frac = hourStr.toFloat() - offsetHours;
        offsetMinutes = (int)(frac * 60);
      } else {
        offsetHours = hourStr.toInt();
      }
    }
    
    // В POSIX TZ: минус означает восточное полушарие (UTC+), плюс - западное (UTC-)
    // configTime() ожидает: положительное = восточнее UTC
    if (lastDash > lastPlus) {
      // Минус = восточное полушарие = положительное смещение
      return offsetHours * 3600 + offsetMinutes * 60;
    } else {
      // Плюс = западное полушарие = отрицательное смещение
      return -(offsetHours * 3600 + offsetMinutes * 60);
    }
  } else {
    // Нет знака - ищем число в конце строки (например "EST5", "GMT0")
    // Это обычно означает западное полушарие (UTC-)
    for (int i = tzStr.length() - 1; i >= 0; i--) {
      if (isdigit(tzStr[i])) {
        int end = i + 1;
        int start = i;
        while (start > 0 && (isdigit(tzStr[start - 1]) || tzStr[start - 1] == '.')) {
          start--;
        }
        String numStr = tzStr.substring(start, end);
        int dotPos = numStr.indexOf('.');
        if (dotPos >= 0) {
          offsetHours = numStr.substring(0, dotPos).toInt();
          float frac = numStr.toFloat() - offsetHours;
          offsetMinutes = (int)(frac * 60);
        } else {
          offsetHours = numStr.toInt();
        }
        // Без знака = западное полушарие = отрицательное смещение
        return -(offsetHours * 3600 + offsetMinutes * 60);
      }
    }
  }
  
  return 0; // Не удалось распарсить
}

void TenZillaNTP::begin() {
  systemStartTime = millis();
  
  // Загружаем настройки из NVS
  ntpPreferences.begin("tenzilla-ntp", true);
  
  // Загружаем NTP сервер
  String server = ntpPreferences.getString("ntp_server", "pool.ntp.org");
  server.toCharArray(ntpServer, sizeof(ntpServer));

  // Загружаем часовой пояс (POSIX строка TZ)
  String tz = ntpPreferences.getString("tz", "MSK-3");
  tz.toCharArray(timezoneStr, sizeof(timezoneStr));
  
  // Загружаем интервал обновления (в секундах)
  ntpInterval = ntpPreferences.getULong("ntp_interval", 3600);
  
  ntpPreferences.end();
  
  // ВАЖНО: Устанавливаем часовой пояс сразу при инициализации
  // Это нужно для правильной работы getLocalTime() даже до первой синхронизации
  setenv("TZ", timezoneStr, 1);
  tzset();
  
  // Вычисляем смещение для использования в configTime()
  int gmtOffset = parseTimezoneOffset(timezoneStr);
  
  Serial.println("🕐 NTP initialized:");
  Serial.println("   Server: " + String(ntpServer));
  Serial.println("   Interval: " + String(ntpInterval) + " seconds");
  Serial.println("   Timezone: " + String(timezoneStr));
  Serial.print("   GMT Offset: ");
  Serial.print(gmtOffset / 3600);
  Serial.println(" hours");
  Serial.println("   Note: Time will sync when WiFi is connected");
}

void TenZillaNTP::update() {
  if (WiFi.status() != WL_CONNECTED) {
    timeSynced = false;
    return;
  }
  
  unsigned long now = millis();
  
  // Проверяем, нужно ли обновлять время
  // Первая синхронизация сразу при подключении к WiFi (lastNTPUpdate == 0)
  // Последующие - по интервалу
  if (lastNTPUpdate == 0 || (now - lastNTPUpdate) >= (ntpInterval * 1000)) {
    syncTime();
    lastNTPUpdate = now;
  }
}

void TenZillaNTP::syncTime() {
  Serial.println("🕐 Syncing time with NTP server: " + String(ntpServer));
  
  // Парсим часовой пояс и вычисляем смещение в секундах
  int gmtOffset = parseTimezoneOffset(timezoneStr);
  int daylightOffset = 0; // Пока не поддерживаем автоматический переход на летнее время
  
  Serial.print("🕐 Parsed timezone offset: ");
  Serial.print(gmtOffset / 3600);
  Serial.print(" hours (");
  Serial.print(gmtOffset);
  Serial.println(" seconds)");
  
  // Устанавливаем TZ для совместимости (может использоваться другими функциями)
  setenv("TZ", timezoneStr, 1);
  tzset();

  // Синхронизируем время с NTP сервером с правильным смещением
  // configTime(gmtOffset_sec, daylightOffset_sec, server1, server2, server3)
  configTime(gmtOffset, daylightOffset, ntpServer);
  
  // Ждем синхронизации (максимум 10 секунд)
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo, 0) && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (attempts < 20) {
    timeSynced = true;
    lastSyncTime = millis();
    Serial.println("✅ Time synced successfully");
  } else {
    timeSynced = false;
    Serial.println("❌ Time sync failed");
  }
}

void TenZillaNTP::setNTPServer(const char* server) {
  if (server != nullptr && strlen(server) > 0 && strlen(server) < sizeof(ntpServer)) {
    strncpy(ntpServer, server, sizeof(ntpServer) - 1);
    ntpServer[sizeof(ntpServer) - 1] = '\0';
    
    // Сохраняем в NVS
    ntpPreferences.begin("tenzilla-ntp", false);
    ntpPreferences.putString("ntp_server", ntpServer);
    ntpPreferences.end();
    
    // Пересинхронизируем время
    if (WiFi.status() == WL_CONNECTED) {
      syncTime();
    }
  }
}

const char* TenZillaNTP::getNTPServer() {
  return ntpServer;
}

void TenZillaNTP::setNTPInterval(unsigned long intervalSeconds) {
  if (intervalSeconds >= 60 && intervalSeconds <= 86400) { // От 1 минуты до 24 часов
    ntpInterval = intervalSeconds;
    
    // Сохраняем в NVS
    ntpPreferences.begin("tenzilla-ntp", false);
    ntpPreferences.putULong("ntp_interval", ntpInterval);
    ntpPreferences.end();
  }
}

unsigned long TenZillaNTP::getNTPInterval() {
  return ntpInterval;
}

bool TenZillaNTP::isTimeSynced() {
  return timeSynced && (WiFi.status() == WL_CONNECTED);
}

unsigned long TenZillaNTP::getLastSyncTime() {
  return lastSyncTime;
}

unsigned long TenZillaNTP::getUptimeMs() {
  return millis() - systemStartTime;
}

String TenZillaNTP::getUptimeString() {
  unsigned long uptimeMs = getUptimeMs();
  unsigned long seconds = uptimeMs / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return String(buffer);
}

void TenZillaNTP::setTimezone(const char* tz) {
  if (tz != nullptr && strlen(tz) > 0 && strlen(tz) < sizeof(timezoneStr)) {
    strncpy(timezoneStr, tz, sizeof(timezoneStr) - 1);
    timezoneStr[sizeof(timezoneStr) - 1] = '\0';

    // Сохраняем в NVS
    ntpPreferences.begin("tenzilla-ntp", false);
    ntpPreferences.putString("tz", timezoneStr);
    ntpPreferences.end();

    // Применяем новый часовой пояс немедленно
    setenv("TZ", timezoneStr, 1);
    tzset();

    // При наличии WiFi можно сразу пересинхронизировать время
    if (WiFi.status() == WL_CONNECTED) {
      syncTime();
    }
  }
}

const char* TenZillaNTP::getTimezone() {
  return timezoneStr;
}
