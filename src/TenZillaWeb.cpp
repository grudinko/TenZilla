#include "TenZillaWeb.h"
#include "TenZillaScale.h"
#include "TenZillaProgram.h"
#include "TenZillaNTP.h"
#include "TenZillaMeasurements.h"
#include "TenZillaVersion.h"
#include "TenZillaTelegram.h"
#include <Preferences.h>
#include <cstring>
#include <time.h>
#ifdef ESP32
  #include <Update.h>
#endif
// #include <ArduinoJson.h>  // Временно отключено

WebServer TenZillaWeb::server(80);
TaskHandle_t TenZillaWeb::webServerTaskHandle = nullptr;
IPAddress TenZillaWeb::allowedClientIP(0, 0, 0, 0);
IPAddress TenZillaWeb::allowedClients[10];
int TenZillaWeb::allowedClientsCount = 0;

static unsigned long s_lastWebRequestTime = 0;
static void markWebRequestActivity() { s_lastWebRequestTime = millis(); }

/* Deferred Telegram test: POST returns 202, actual send runs in loop() to free web client memory before SSL. */
static struct {
  bool pending;
  bool done;
  bool ok;
  char err[96];
} s_tgDeferred = { false, false, false, "" };

#if defined(ESP32)
static bool s_otaStarted = false;
static uint32_t s_otaLastLogBytes = 0;
static uint32_t s_otaLastHeapLog = 0;
static bool s_otaFirstChunk = true;
#endif

// TenZillaDisplay подключаем только после определения статических членов
// Это предотвращает конфликт FS между TFT_eSPI и WebServer
#include "TenZillaDisplay.h"

void TenZillaWeb::begin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/apname", HTTP_GET, handleAPName);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/api/restart", HTTP_POST, handleRestart);
  
  // Новые API endpoints
  server.on("/api/scale", HTTP_GET, handleScaleData);
  server.on("/api/motor/up", HTTP_POST, handleMotorUp);
  server.on("/api/motor/down", HTTP_POST, handleMotorDown);
  server.on("/api/motor/stop", HTTP_POST, handleMotorStop);
  server.on("/api/counter/reset", HTTP_POST, handleCounterReset);
  server.on("/api/scale/calibrate", HTTP_POST, handleScaleCalibrate);
  server.on("/api/scale/max", HTTP_POST, handleScaleMax);
  server.on("/api/scale/negative-weight-limit", HTTP_POST, handleScaleNegativeWeightLimit);
  
  // Пошаговая калибровка
  server.on("/api/calibration/start", HTTP_POST, handleCalibrationStart);
  server.on("/api/calibration/zero", HTTP_POST, handleCalibrationZero);
  server.on("/api/calibration/point", HTTP_POST, handleCalibrationPoint);
  server.on("/api/calibration/noise-threshold", HTTP_POST, handleSetNoiseThreshold);
  server.on("/api/calibration/set-zero", HTTP_POST, handleSetZeroPoint);
  server.on("/api/calibration/set-point", HTTP_POST, handleSetCalibrationPoint);
  
  // Управление TFT дисплеем
  server.on("/api/display/screen", HTTP_POST, handleDisplayScreen);
  
  // Управление максимумами экранов
  server.on("/api/display/reset/main", HTTP_POST, handleResetMainMax);
  server.on("/api/display/reset/break", HTTP_POST, handleResetBreakMax);
  
  // Настройки энкодера
  server.on("/api/encoder/step", HTTP_POST, handleEncoderStep);
  server.on("/api/encoder/min", HTTP_POST, handleEncoderMin);
  server.on("/api/encoder/max", HTTP_POST, handleEncoderMax);
  server.on("/api/encoder/test-only-b", HTTP_POST, handleEncoderTestOnlyB);
  
  // Управление ограничениями перемещения
  server.on("/api/motor/limits/disable", HTTP_POST, handleMotorLimitsDisable);
  server.on("/api/motor/limits/enable", HTTP_POST, handleMotorLimitsEnable);
  server.on("/api/motor/limits/status", HTTP_GET, handleMotorLimitsStatus);
  
  // Управление ограничениями перемещения
  server.on("/api/motor/limits/disable", HTTP_POST, handleMotorLimitsDisable);
  server.on("/api/motor/limits/enable", HTTP_POST, handleMotorLimitsEnable);
  server.on("/api/motor/limits/status", HTTP_GET, handleMotorLimitsStatus);
  
  // Настройки веб-сервера
  server.on("/api/web/max-connections", HTTP_POST, handleMaxConnections);
  
  // Настройки программы
  server.on("/api/program/compression-threshold", HTTP_POST, handleProgramCompressionThreshold);
  server.on("/api/program/compression-target", HTTP_POST, handleProgramCompressionTarget);
  server.on("/api/program/compression-unload-retract", HTTP_POST, handleProgramCompressionUnloadRetract);
  server.on("/api/program/break-drop-threshold", HTTP_POST, handleProgramBreakDropThreshold);
  server.on("/api/program/stop", HTTP_POST, handleProgramStop);
  server.on("/api/program/start/compression", HTTP_POST, handleProgramStartCompression);
  server.on("/api/program/start/break", HTTP_POST, handleProgramStartBreak);
  
  // Настройки NTP
  server.on("/api/ntp", HTTP_GET, handleNTPGet);
  server.on("/api/ntp/server", HTTP_POST, handleNTPSetServer);
  server.on("/api/ntp/interval", HTTP_POST, handleNTPSetInterval);
  server.on("/api/ntp/timezone", HTTP_POST, handleNTPSetTimezone);
  
  // Настройки производительности чтения тензодатчика
  server.on("/api/scale/performance", HTTP_GET, handleScalePerformanceGet);
  server.on("/api/scale/performance/min-interval", HTTP_POST, handleScalePerformanceSetMinInterval);
  server.on("/api/scale/performance/wait-ms", HTTP_POST, handleScalePerformanceSetWaitMs);
  server.on("/api/scale/performance/i2c-speed", HTTP_POST, handleScalePerformanceSetI2CSpeed);

  server.on("/api/measurements", HTTP_GET, handleMeasurements);
  server.on("/api/telegram", HTTP_GET, handleTelegramGet);
  server.on("/api/telegram", HTTP_POST, handleTelegramSet);
  server.on("/api/telegram/test", HTTP_POST, handleTelegramTest);
  server.on("/api/telegram/test/status", HTTP_GET, handleTelegramTestStatus);

#if defined(ESP32)
  server.on("/update", HTTP_GET, handleUpdateGet);
  server.on("/update", HTTP_POST, handleUpdatePost, handleUpdateUpload);
#endif

  server.begin();
  Serial.println("🌐 TenZilla Web Server started on port 80");
  
  // Создаем задачу FreeRTOS для веб-сервера на втором ядре (Core 1)
  // Это предотвращает блокировку основного цикла при обработке веб-запросов
  // 64KB стека — OTA upload (Update.write + multipart) на /update
  xTaskCreatePinnedToCore(
    webServerTask,        // Функция задачи
    "WebServer",         // Имя задачи
    65536,               // 64KB — снижает риск abort при OTA
    nullptr,             // Параметры задачи
    1,                   // Приоритет (ниже основного цикла, но достаточный для веб-сервера)
    &webServerTaskHandle, // Handle задачи
    1                    // Ядро 1 (второе ядро ESP32)
  );
  
  if (webServerTaskHandle != nullptr) {
    Serial.println("✅ Web Server task created on Core 1");
  } else {
    Serial.println("❌ Failed to create Web Server task!");
  }
}

// Задача FreeRTOS для обработки веб-запросов на втором ядре
void TenZillaWeb::webServerTask(void* parameter) {
  (void)parameter;
  
  Serial.println("🔄 Web Server task started on Core 1");
  
  // Защита от переполнения стека - мониторинг свободного места
  UBaseType_t stackHighWaterMark;
  
  while (true) {
    server.handleClient();

    static unsigned long lastIdleCheck = 0;
    unsigned long now = millis();
    if (lastIdleCheck == 0) lastIdleCheck = now;
    if (now - lastIdleCheck >= 30000) {
      lastIdleCheck = now;
      WiFiClient c = server.client();
      bool idle = (c && c.connected() && s_lastWebRequestTime > 0 && (now - s_lastWebRequestTime) >= 120000);
#if defined(ESP32)
      if (s_otaStarted) idle = false;
#endif
      if (idle) c.stop();
    }

    stackHighWaterMark = uxTaskGetStackHighWaterMark(webServerTaskHandle);
    if (stackHighWaterMark < 2048) {  // Если осталось меньше 2KB
      Serial.print("⚠️ Web Server task stack low: ");
      Serial.println(stackHighWaterMark);
    }
    
    // Небольшая задержка для освобождения CPU
    // Используем vTaskDelay вместо delay() для неблокирующей задержки
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms задержка
  }
}

void TenZillaWeb::handleClient() {
  // Теперь handleClient() вызывается из задачи FreeRTOS
  // Оставляем для обратной совместимости, но основной вызов идет из webServerTask
  server.handleClient();
}

int TenZillaWeb::getMaxConnections() {
  TenZillaSettings config = TenZillaConfig::get();
  if (config.maxWebConnections < 1) config.maxWebConnections = 1;
  if (config.maxWebConnections > 10) config.maxWebConnections = 10;
  return config.maxWebConnections;
}

void TenZillaWeb::setMaxConnections(int maxConn) {
  if (maxConn < 1) maxConn = 1;
  if (maxConn > 10) maxConn = 10;
  
  TenZillaSettings config = TenZillaConfig::get();
  config.maxWebConnections = maxConn;
  TenZillaConfig::set(config);
  TenZillaConfig::save();
  
  // Очищаем список разрешенных клиентов при изменении лимита
  allowedClientsCount = 0;
  allowedClientIP = IPAddress(0, 0, 0, 0);
  for (int i = 0; i < 10; i++) {
    allowedClients[i] = IPAddress(0, 0, 0, 0);
  }
}

bool TenZillaWeb::isClientAllowed() {
  int maxConn = getMaxConnections();
  
  // Проверяем количество подключенных станций в AP режиме
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    int stationCount = WiFi.softAPgetStationNum();
    if (stationCount > maxConn) {
      // Если уже максимальное количество подключенных станций, отклоняем нового клиента
      return false;
    }
  }
  
  // Получаем IP текущего клиента (в контексте обработки запроса клиент всегда есть)
  WiFiClient client = server.client();
  if (!client || !client.connected()) {
    return false;
  }
  
  IPAddress clientIP = client.remoteIP();
  
  // Проверяем, что IP валидный (не 0.0.0.0)
  if (clientIP == IPAddress(0, 0, 0, 0)) {
    return false;
  }
  
  // Проверяем, есть ли уже этот IP в списке разрешенных
  for (int i = 0; i < allowedClientsCount; i++) {
    if (allowedClients[i] == clientIP) {
      return true;  // Уже разрешен
    }
  }
  
  // Если список еще не заполнен, добавляем новый IP
  if (allowedClientsCount < maxConn) {
    allowedClients[allowedClientsCount] = clientIP;
    allowedClientsCount++;
    // Для обратной совместимости также обновляем allowedClientIP
    if (allowedClientIP == IPAddress(0, 0, 0, 0)) {
      allowedClientIP = clientIP;
    }
    return true;
  }
  
  // Достигнут лимит подключений
  return false;
}

void TenZillaWeb::handleRoot() {
  markWebRequestActivity();
  const char* html = getHTMLPage();
  if (html == nullptr || strlen(html) == 0) {
    server.send(500, "text/html", "<html><body><h1>Error</h1><p>Failed to generate HTML page</p><p>HTML is null or empty</p></body></html>");
    return;
  }
  server.send_P(200, "text/html", html);
}

void TenZillaWeb::handleAPName() {
  markWebRequestActivity();
  server.send(200, "text/plain", TenZillaWiFi::getAPName());
}

void TenZillaWeb::handleStatus() {
  markWebRequestActivity();
  String response = "{";
  response += "\"version\":\"";
  response += TenZillaConfig::getVersion();
  response += "\",\"connected\":";
  response += TenZillaWiFi::isConnected() ? "true" : "false";
  
  if (TenZillaWiFi::isConnected()) {
    response += ",\"ssid\":\"";
    response += WiFi.SSID();
    response += "\",\"ip\":\"";
    response += WiFi.localIP().toString();
    response += "\",\"rssi\":";
    response += String(WiFi.RSSI());
    response += ",\"status\":\"Connected\",";
    
    // Определение качества сигнала
    String quality;
    int rssi = WiFi.RSSI();
    if (rssi >= -50) quality = "Excellent";
    else if (rssi >= -60) quality = "Very Good";
    else if (rssi >= -70) quality = "Good";
    else if (rssi >= -80) quality = "Low";
    else quality = "Very Low";
    
    response += "\"quality\":\"";
    response += quality;
    response += "\"";
  } else {
    response += ",\"ap_name\":\"";
    response += TenZillaWiFi::getAPName();
    response += "\",\"ap_ip\":\"";
    response += WiFi.softAPIP().toString();
    response += "\",\"status\":\"AP Mode\"";
  }
  
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleConfig() {
  markWebRequestActivity();
  if (server.hasArg("ssid")) {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    if (newSSID.length() == 0 || newSSID.length() > 31) {
      server.send(400, "text/plain", "Invalid SSID length");
      return;
    }
    
    TenZillaSettings newSettings = TenZillaConfig::get();
    strncpy(newSettings.ssid, newSSID.c_str(), sizeof(newSettings.ssid) - 1);
    strncpy(newSettings.password, newPassword.c_str(), sizeof(newSettings.password) - 1);
    newSettings.ssid[sizeof(newSettings.ssid) - 1] = '\0';
    newSettings.password[sizeof(newSettings.password) - 1] = '\0';
    
    TenZillaConfig::set(newSettings);
    TenZillaConfig::save();
    
    server.send(200, "text/plain", "Configuration saved. Restarting...");
    // Убираем блокирующий delay() - используем yield() для освобождения CPU
    yield();
    delay(100);  // Минимальная задержка для отправки ответа
    ESP.restart();
  } else {
    server.send(400, "text/plain", "SSID parameter missing");
  }
}

void TenZillaWeb::handleReset() {
  markWebRequestActivity();
  TenZillaConfig::reset();
  server.send(200, "text/plain", "Configuration reset. Restarting...");
  // Убираем блокирующий delay() - используем yield() для освобождения CPU
  yield();
  delay(100);  // Минимальная задержка для отправки ответа
  ESP.restart();
}

void TenZillaWeb::handleRestart() {
  markWebRequestActivity();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Перезапуск устройства...\"}");
  // Убираем блокирующий delay() - используем yield() для освобождения CPU
  yield();
  delay(100);  // Минимальная задержка для отправки ответа
  ESP.restart();
}

// Новые обработчики API
void TenZillaWeb::handleScaleData() {
  markWebRequestActivity();
  String response;
  response.reserve(1024);
  response += "{";
  response += "\"currentWeight\":";
  response += String(TenZillaScale::getCachedWeight(), 1);
  response += ",\"maxWeight\":";
  response += String(TenZillaScale::getMaxWeight(), 1);
  response += ",\"negativeWeightLimit\":";
  response += String(TenZillaScale::getNegativeWeightLimit(), 1);
  response += ",\"compressionMax\":";
  response += String(TenZillaDisplay::getMainScreenMax(), 1);
  response += ",\"breakMax\":";
  response += String(TenZillaDisplay::getBreakScreenMax(), 1);
  response += ",\"opticalCount\":";
  response += String(TenZillaScale::getOpticalCount());
  response += ",\"displacement\":";
  response += String(TenZillaScale::getDisplacement(), 2);
  response += ",\"workingDisplacement\":";
  response += String(TenZillaProgram::getWorkingDisplacement(), 2);
  response += ",\"encoderStepMm\":";
  response += String(TenZillaScale::getEncoderStepMm(), 4);
  response += ",\"encoderMin\":";
  response += String(TenZillaScale::getEncoderMin());
  response += ",\"encoderMax\":";
  // Возвращаем в мм для веб-интерфейса (хранится в импульсах, конвертируем)
  float encoderStepMm = TenZillaScale::getEncoderStepMm();
  int encoderMaxPulses = TenZillaScale::getEncoderMax();
  float encoderMaxMm = encoderMaxPulses * encoderStepMm;
  response += String(encoderMaxMm, 2);
  response += ",\"encoderTestOnlyB\":";
  response += TenZillaScale::getEncoderTestOnlyB() ? "true" : "false";
  response += ",\"limitsDisabled\":";
  response += TenZillaScale::areLimitsDisabled() ? "true" : "false";
  response += ",\"motorRunning\":";
  response += TenZillaScale::isMotorRunning() ? "true" : "false";
  response += ",\"motorDirection\":";
  response += String(TenZillaScale::getMotorDirection());
  response += ",\"rawValue\":";
  response += String(TenZillaScale::getRawValue());
  response += ",\"averagedRaw\":";
  response += String(TenZillaScale::getAveragedRawForZero());
  response += ",\"zeroRaw\":";
  response += String(TenZillaScale::getZeroRaw());
  response += ",\"calibrationRaw\":";
  response += String(TenZillaScale::getCalibrationRaw());
  response += ",\"calibrationInProgress\":";
  response += TenZillaScale::isCalibrationInProgress() ? "true" : "false";
  response += ",\"calibrationFactor\":";
  response += String(TenZillaScale::getCalibrationFactor(), 2);
  response += ",\"noiseLevel\":";
  response += String(TenZillaScale::getNoiseLevel(), 2);
  response += ",\"noiseThreshold\":";
  response += String(TenZillaScale::getNoiseThreshold(), 2);
  response += ",\"isStable\":";
  response += TenZillaScale::isStable() ? "true" : "false";
  response += ",\"maxWebConnections\":";
  response += String(getMaxConnections());
  response += ",\"compressionStartThreshold\":";
  response += String(TenZillaProgram::getCompressionStartThreshold(), 1);
  response += ",\"compressionTargetDisplacement\":";
  response += String(TenZillaProgram::getCompressionTargetDisplacement(), 2);
  response += ",\"compressionUnloadRetractMm\":";
  response += String(TenZillaProgram::getCompressionUnloadRetractMm(), 2);
  response += ",\"breakDropThreshold\":";
  response += String(TenZillaProgram::getBreakDropThreshold(), 1);
  response += ",\"currentScreen\":";
  response += String(TenZillaDisplay::getCurrentScreen());
  response += ",\"programRunning\":";
  response += TenZillaProgram::isRunning() ? "true" : "false";
  response += ",\"programElapsedMs\":";
  response += TenZillaProgram::isRunning() ? String(millis() - TenZillaProgram::getProgramStartTimeMs()) : "0";
  response += ",\"lastProgramDurationMs\":";
  response += String(TenZillaProgram::getLastProgramDurationMs());
  response += ",\"currentTimeEpoch\":";
  response += String((long)time(nullptr));
  response += ",\"uptimeMs\":";
  response += String(millis());
  response += ",\"programCompleted\":";
  response += TenZillaProgram::isCompletedSuccessfully() ? "true" : "false";
  response += ",\"programStatusMessage\":\"";
  response += TenZillaProgram::getStatusMessage();
  response += "\",\"ntpServer\":\"";
  response += TenZillaNTP::getNTPServer();
  response += "\",\"ntpInterval\":";
  response += String(TenZillaNTP::getNTPInterval());
  response += ",\"ntpSynced\":";
  response += TenZillaNTP::isTimeSynced() ? "true" : "false";
  response += ",\"ntpTimezone\":\"";
  response += TenZillaNTP::getTimezone();
  response += "\",\"minReadInterval\":";
  response += String(TenZillaScale::getMinReadInterval());
  response += ",\"nau7802WaitMs\":";
  response += String(TenZillaScale::getNAU7802WaitMs());
  response += ",\"i2cSpeed\":";
  response += String(TenZillaScale::getI2CSpeed());
  response += ",\"release\":\"";
  response += TENZILLA_RELEASE_NUMBER;
  response += "\",\"releaseDate\":\"";
  response += TENZILLA_RELEASE_DATE;
  response += "\"}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleMotorUp() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "text/plain", "Connection limit: Only 1 client allowed");
    return;
  }
  TenZillaScale::motorUp();
  server.send(200, "text/plain", "OK");
}

void TenZillaWeb::handleMotorDown() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "text/plain", "Connection limit: Only 1 client allowed");
    return;
  }
  TenZillaScale::motorDown();
  server.send(200, "text/plain", "OK");
}

void TenZillaWeb::handleMotorStop() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "text/plain", "Connection limit: Only 1 client allowed");
    return;
  }
  TenZillaScale::motorStop();
  server.send(200, "text/plain", "OK");
}


void TenZillaWeb::handleCounterReset() {
  markWebRequestActivity();
  TenZillaScale::resetOpticalCount();
  server.send(200, "text/plain", "Counter reset");
}

void TenZillaWeb::handleScaleCalibrate() {
  markWebRequestActivity();
  String body = server.arg("plain");
  // Простой парсинг JSON: ищем "weight":число
  float weight = 0.0;
  int weightPos = body.indexOf("\"weight\":");
  if (weightPos >= 0) {
    int startPos = weightPos + 9; // после "weight":
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String weightStr = body.substring(startPos, endPos);
      weight = weightStr.toFloat();
    }
  }
  
  TenZillaScale::calibrateScale(weight);  // weight в N
  server.send(200, "text/plain", "Scale calibrated");
}

void TenZillaWeb::handleScaleMax() {
  markWebRequestActivity();
  String body = server.arg("plain");
  // Простой парсинг JSON: ищем "weight":число
  float weight = 0.0;
  int weightPos = body.indexOf("\"weight\":");
  if (weightPos >= 0) {
    int startPos = weightPos + 9; // после "weight":
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String weightStr = body.substring(startPos, endPos);
      weight = weightStr.toFloat();
    }
  }
  
  TenZillaScale::setMaxWeight(weight);
  TenZillaScale::saveConfig();  // Сохраняем в NVS, чтобы значение пережило перезагрузку
  server.send(200, "text/plain", "Overload limit set to " + String(weight, 1) + " N");
}

void TenZillaWeb::handleScaleNegativeWeightLimit() {
  markWebRequestActivity();
  String body = server.arg("plain");
  float limitN = -50.0f;
  int pos = body.indexOf("\"limit\":");
  if (pos >= 0) {
    int startPos = pos + 8;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String s = body.substring(startPos, endPos);
      s.trim();
      limitN = s.toFloat();
    }
  }
  TenZillaScale::setNegativeWeightLimit(limitN);
  TenZillaScale::saveConfig();
  server.send(200, "text/plain", "Negative weight limit set to " + String(limitN, 1) + " N");
}


// ============================================
// ПОШАГОВАЯ КАЛИБРОВКА
// ============================================

void TenZillaWeb::handleCalibrationStart() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit: Only 1 client allowed\"}");
    return;
  }
  
  // Блокируем запуск калибровки при отключенных ограничениях
  if (TenZillaScale::areLimitsDisabled()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Calibration blocked: displacement limits are disabled\"}");
    return;
  }
  
  TenZillaScale::startCalibration();
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

void TenZillaWeb::handleCalibrationZero() {
  markWebRequestActivity();
  bool success = TenZillaScale::recordZeroPoint();
  if (success) {
    String response = "{";
    response += "\"status\":\"success\",";
    response += "\"zeroRaw\":" + String(TenZillaScale::getZeroRaw());
    response += "}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Показания нестабильны\"}");
  }
}

void TenZillaWeb::handleSetNoiseThreshold() {
  markWebRequestActivity();
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  float threshold = 0.0;
  
  // Парсинг JSON: ищем "threshold":число (как в handleCalibrationPoint)
  if (body.length() > 0) {
    int thresholdPos = body.indexOf("\"threshold\":");
    if (thresholdPos >= 0) {
      int startPos = thresholdPos + 12; // после "threshold":
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String thresholdStr = body.substring(startPos, endPos);
        thresholdStr.trim();
        threshold = thresholdStr.toFloat();
      }
    }
  }
  
  // Попытка получить из URL-параметра (для обратной совместимости)
  if (threshold == 0.0 && server.hasArg("threshold")) {
    threshold = server.arg("threshold").toFloat();
  }
  
  if (threshold >= 0.1f && threshold <= 50.0f) {
    TenZillaScale::setNoiseThreshold(threshold);
    server.send(200, "application/json", "{\"status\":\"ok\",\"threshold\":" + String(threshold, 2) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение порога (0.1-50.0)\"}");
  }
}

void TenZillaWeb::handleCalibrationPoint() {
  markWebRequestActivity();
  // Защита от слишком больших запросов
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  if (body.length() > 256) {  // Ограничение размера тела запроса
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
    return;
  }
  
  float weightNewtons = 0.0;
  
  // Парсинг JSON: ищем "weight":число
  int weightPos = body.indexOf("\"weight\":");
  if (weightPos >= 0) {
    int startPos = weightPos + 9; // после "weight":
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
      String weightStr = body.substring(startPos, endPos);
      weightStr.trim();
      weightNewtons = weightStr.toFloat();
    }
  }
  
  if (weightNewtons <= 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверный вес\"}");
    return;
  }
  
  bool success = TenZillaScale::recordCalibrationPoint(weightNewtons);
  if (success) {
    String response = "{";
    response += "\"status\":\"success\",";
    response += "\"zeroRaw\":" + String(TenZillaScale::getZeroRaw()) + ",";
    response += "\"calibrationRaw\":" + String(TenZillaScale::getCalibrationRaw()) + ",";
    response += "\"calibrationFactor\":" + String(TenZillaScale::getCalibrationFactor(), 2);
    response += "}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Калибровка не удалась\"}");
  }
}

void TenZillaWeb::handleSetZeroPoint() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  // Защита от слишком больших запросов
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    if (body.length() > 512) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
    
    float weightN = 0.0;
    long rawValue = 0;
    
    // Парсинг JSON: ищем "weight":число и "raw":число
    int weightPos = body.indexOf("\"weight\":");
    if (weightPos >= 0) {
      int startPos = weightPos + 9;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String weightStr = body.substring(startPos, endPos);
        weightStr.trim();
        weightN = weightStr.toFloat();
      }
    }
    
    int rawPos = body.indexOf("\"raw\":");
    if (rawPos >= 0) {
      int startPos = rawPos + 6;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String rawStr = body.substring(startPos, endPos);
        rawStr.trim();
        rawValue = rawStr.toInt();
      }
    }
    
    // Валидация: диапазон NAU7802 (24-битный АЦП: примерно -8,388,608 до 8,388,607)
    if (rawValue < -8388608 || rawValue > 8388607) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"RAW значение вне диапазона (-8388608 до 8388607)\"}");
      return;
    }
    
    // Устанавливаем нулевую точку (вес должен быть 0)
    if (abs(weightN) > 0.01) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Вес нулевой точки должен быть 0 Н\"}");
      return;
    }
    
    // Устанавливаем значения
    TenZillaScale::setZeroRaw(rawValue);
    
    String response = "{";
    response += "\"status\":\"success\",";
    response += "\"zeroRaw\":" + String(rawValue) + ",";
    response += "\"weight\":" + String(weightN, 1);
    response += "}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing request body\"}");
  }
}

void TenZillaWeb::handleSetCalibrationPoint() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  // Защита от слишком больших запросов
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  if (body.length() > 512) {  // Ограничение размера тела запроса
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
    return;
  }
  
  float weightN = 0.0;
  long rawValue = 0;
  
  // Парсинг JSON: ищем "weight":число и "raw":число
  int weightPos = body.indexOf("\"weight\":");
  if (weightPos >= 0) {
    int startPos = weightPos + 9;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
      String weightStr = body.substring(startPos, endPos);
      weightStr.trim();
      weightN = weightStr.toFloat();
    }
  }
  
  int rawPos = body.indexOf("\"raw\":");
  if (rawPos >= 0) {
    int startPos = rawPos + 6;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
      String rawStr = body.substring(startPos, endPos);
      rawStr.trim();
      rawValue = rawStr.toInt();
    }
  }
  
  // Валидация: вес должен быть положительным
  if (weightN <= 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Вес должен быть положительным\"}");
    return;
  }
  
  // Валидация: диапазон NAU7802 (24-бит)
  if (rawValue < -8388608 || rawValue > 8388607) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"RAW значение вне диапазона (-8388608 до 8388607)\"}");
    return;
  }
  
  // Получаем нулевую точку
  long zeroRaw = TenZillaScale::getZeroRaw();
  
  // Валидация: проверка деления на ноль
  long rawDiff = rawValue - zeroRaw;
  if (rawDiff == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Разность RAW равна нулю. Нулевая и вторая точки не могут совпадать.\"}");
    return;
  }
  
  // Вычисляем коэффициент калибровки
  // calibrationFactor переводит RAW напрямую в ньютоны: weightN = (raw - zeroRaw) / factor
  float calibrationFactor = (float)rawDiff / weightN;
  
  // Валидация: коэффициент должен быть разумным (например, от 0.001 до 100000)
  if (calibrationFactor <= 0 || calibrationFactor > 100000 || calibrationFactor < 0.001) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Недопустимый коэффициент калибровки. Проверьте значения.\"}");
    return;
  }
  
  // Устанавливаем значения
  TenZillaScale::setCalibrationRaw(rawValue);
  TenZillaScale::setCalibrationFactor(calibrationFactor);
  
  String response = "{";
  response += "\"status\":\"success\",";
  response += "\"calibrationRaw\":" + String(rawValue) + ",";
  response += "\"weight\":" + String(weightN, 1) + ",";
  response += "\"calibrationFactor\":" + String(calibrationFactor, 2);
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleDisplayScreen() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit: Only 1 client allowed\"}");
    return;
  }
  // Запрет переключения экрана, если программа запущена
  if (TenZillaProgram::isRunning()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Program is running\"}");
    return;
  }
  
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  int screenNumber = 1;
  
  // Парсинг JSON: ищем "screen":число
  if (body.length() > 0) {
    int screenPos = body.indexOf("\"screen\":");
    if (screenPos >= 0) {
      int startPos = screenPos + 9; // после "screen":
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String screenStr = body.substring(startPos, endPos);
        screenStr.trim();
        screenNumber = screenStr.toInt();
      }
    }
  }
  
  if (screenNumber < 1 || screenNumber > 7) screenNumber = 1;
  
  switch(screenNumber) {
    case 1: TenZillaDisplay::showMainScreen(); break;
    case 2: TenZillaDisplay::showBreakScreen(); break;
    case 3: TenZillaDisplay::showWiFiScreen(); break;
    case 4: TenZillaDisplay::showScaleSettingsScreen(); break;
    case 5: TenZillaDisplay::showMotorSettingsScreen(); break;
    case 6: TenZillaDisplay::showOtherSettingsScreen(); break;
    case 7: TenZillaDisplay::showHistoryScreen(); break;
    default: TenZillaDisplay::showMainScreen(); break;
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\",\"screen\":" + String(screenNumber) + "}");
}

void TenZillaWeb::handleResetMainMax() {
  markWebRequestActivity();
  TenZillaDisplay::resetMainScreenMax();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void TenZillaWeb::handleResetBreakMax() {
  markWebRequestActivity();
  TenZillaDisplay::resetBreakScreenMax();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void TenZillaWeb::handleEncoderStep() {
  markWebRequestActivity();
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  float stepMm = 0.0f;
  
  // Парсинг JSON: ищем "stepMm":число
  if (body.length() > 0) {
    int stepPos = body.indexOf("\"stepMm\":");
    if (stepPos >= 0) {
      int startPos = stepPos + 9;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String stepStr = body.substring(startPos, endPos);
        stepStr.trim();
        stepMm = stepStr.toFloat();
      }
    }
  }
  
  if (stepMm > 0.0f && stepMm <= 100.0f) {
    TenZillaScale::setEncoderStepMm(stepMm);
    server.send(200, "application/json", "{\"status\":\"ok\",\"stepMm\":" + String(stepMm, 4) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение шага (0.0001-100.0)\"}");
  }
}

void TenZillaWeb::handleEncoderMin() {
  markWebRequestActivity();
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  int minValue = 0;
  
  // Парсинг JSON: ищем "min":число
  if (body.length() > 0) {
    int minPos = body.indexOf("\"min\":");
    if (minPos >= 0) {
      int startPos = minPos + 6;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String minStr = body.substring(startPos, endPos);
        minStr.trim();
        minValue = minStr.toInt();
      }
    }
  }
  
  TenZillaScale::setEncoderMin(minValue);
  server.send(200, "application/json", "{\"status\":\"ok\",\"min\":" + String(minValue) + "}");
}

void TenZillaWeb::handleEncoderMax() {
  markWebRequestActivity();
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  float maxValueMm = 0.0f;
  
  // Парсинг JSON: ищем "max":число (теперь в мм)
  if (body.length() > 0) {
    int maxPos = body.indexOf("\"max\":");
    if (maxPos >= 0) {
      int startPos = maxPos + 6;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String maxStr = body.substring(startPos, endPos);
        maxStr.trim();
        maxValueMm = maxStr.toFloat();
      }
    }
  }
  
  // Конвертируем мм в импульсы
  float encoderStepMm = TenZillaScale::getEncoderStepMm();
  if (encoderStepMm <= 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Encoder step not set\"}");
    return;
  }
  int maxValuePulses = (int)(maxValueMm / encoderStepMm);
  
  TenZillaScale::setEncoderMax(maxValuePulses);
  server.send(200, "application/json", "{\"status\":\"ok\",\"max\":" + String(maxValueMm, 2) + "}");
}

void TenZillaWeb::handleEncoderTestOnlyB() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  if (!server.hasArg("plain")) {
    Serial.println("ENC testOnlyB: no plain body");
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
    return;
  }
  String body = server.arg("plain");
  body.trim();
  if (body.length() == 0 || body.length() > 256) {
    Serial.print("ENC testOnlyB: body len=");
    Serial.println(body.length());
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid body\"}");
    return;
  }
  bool enable = TenZillaScale::getEncoderTestOnlyB();
  const char* needle = "\"testOnlyB\":";
  int pos = body.indexOf(needle);
  if (pos >= 0) {
    int start = pos + (int)strlen(needle);
    while (start < (int)body.length()) {
      char c = body[start];
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { start++; continue; }
      break;
    }
    if (start + 4 <= (int)body.length() && body.substring(start, start + 4) == "true") enable = true;
    else if (start + 5 <= (int)body.length() && body.substring(start, start + 5) == "false") enable = false;
  }
  Serial.print("ENC testOnlyB: body=");
  Serial.println(body);
  Serial.print("ENC testOnlyB: enable=");
  Serial.println(enable ? 1 : 0);
  TenZillaScale::setEncoderTestOnlyB(enable);
  Preferences prefs;
  prefs.begin("tenzilla-scale", true);
  bool verify = prefs.getBool("encoder_test_only_b", false);
  prefs.end();
  Serial.print("ENC testOnlyB: NVS verify=");
  Serial.println(verify ? 1 : 0);
  server.send(200, "application/json", "{\"status\":\"ok\",\"testOnlyB\":" + String(enable ? "true" : "false") + "}");
}

// ============================================
// УПРАВЛЕНИЕ ОГРАНИЧЕНИЯМИ ПЕРЕМЕЩЕНИЯ
// ============================================

void TenZillaWeb::handleMotorLimitsDisable() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  // Защита от слишком больших запросов
  String body;
  if (server.hasArg("plain")) {
    body = server.arg("plain");
    if (body.length() > 256) {  // Ограничение размера тела запроса
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Request body too large\"}");
      return;
    }
  }
  
  int pinCode = 0;
  
  // Парсинг JSON: ищем "pinCode":число
  if (body.length() > 0) {
    int pinPos = body.indexOf("\"pinCode\":");
    if (pinPos >= 0) {
      int startPos = pinPos + 10;
      int endPos = body.indexOf(",", startPos);
      if (endPos < 0) endPos = body.indexOf("}", startPos);
      if (endPos > startPos && endPos - startPos < 32) {  // Защита от переполнения
        String pinStr = body.substring(startPos, endPos);
        pinStr.trim();
        pinCode = pinStr.toInt();
      }
    }
  }
  
  if (TenZillaScale::disableLimits(pinCode)) {
    server.send(200, "application/json", "{\"status\":\"ok\",\"limitsDisabled\":true,\"message\":\"Displacement limits disabled (temporary, not saved)\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid PIN code\"}");
  }
}

void TenZillaWeb::handleMotorLimitsEnable() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  TenZillaScale::enableLimits();
  server.send(200, "application/json", "{\"status\":\"ok\",\"limitsDisabled\":false,\"message\":\"Displacement limits enabled\"}");
}

void TenZillaWeb::handleMotorLimitsStatus() {
  markWebRequestActivity();
  bool disabled = TenZillaScale::areLimitsDisabled();
  String response = "{";
  response += "\"status\":\"ok\",";
  response += "\"limitsDisabled\":";
  response += disabled ? "true" : "false";
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleMaxConnections() {
  markWebRequestActivity();
  String body = server.arg("plain");
  int maxConn = 1;
  
  // Парсинг JSON: ищем "maxConnections":число
  int maxConnPos = body.indexOf("\"maxConnections\":");
  if (maxConnPos >= 0) {
    int startPos = maxConnPos + 17;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String maxConnStr = body.substring(startPos, endPos);
      maxConn = maxConnStr.toInt();
    }
  }
  
  if (maxConn >= 1 && maxConn <= 10) {
    setMaxConnections(maxConn);
    server.send(200, "application/json", "{\"status\":\"ok\",\"maxConnections\":" + String(maxConn) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение (1-10)\"}");
  }
}

void TenZillaWeb::handleProgramCompressionThreshold() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  float threshold = 0.0f;
  
  int thresholdPos = body.indexOf("\"threshold\":");
  if (thresholdPos >= 0) {
    int startPos = thresholdPos + 12;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String thresholdStr = body.substring(startPos, endPos);
      threshold = thresholdStr.toFloat();
    }
  }
  
  if (threshold > 0.0f && threshold <= 1000.0f) {
    TenZillaProgram::setCompressionStartThreshold(threshold);
    server.send(200, "application/json", "{\"status\":\"ok\",\"threshold\":" + String(threshold, 1) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение (0.1-1000.0 Н)\"}");
  }
}

void TenZillaWeb::handleProgramCompressionTarget() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  float target = 0.0f;
  
  int targetPos = body.indexOf("\"target\":");
  if (targetPos >= 0) {
    int startPos = targetPos + 9;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String targetStr = body.substring(startPos, endPos);
      target = targetStr.toFloat();
    }
  }
  
  if (target > 0.0f && target <= 1000.0f) {
    TenZillaProgram::setCompressionTargetDisplacement(target);
    server.send(200, "application/json", "{\"status\":\"ok\",\"target\":" + String(target, 2) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение (0.1-1000.0 мм)\"}");
  }
}

void TenZillaWeb::handleProgramCompressionUnloadRetract() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  String body = server.arg("plain");
  float mm = 0.0f;
  int pos = body.indexOf("\"mm\":");
  if (pos >= 0) {
    int start = pos + 5;
    int end = body.indexOf(",", start);
    if (end < 0) end = body.indexOf("}", start);
    if (end > start) mm = body.substring(start, end).toFloat();
  }
  if (mm >= 0.0f && mm <= 100.0f) {
    TenZillaProgram::setCompressionUnloadRetractMm(mm);
    server.send(200, "application/json", "{\"status\":\"ok\",\"mm\":" + String(mm, 2) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение (0-100.0 мм)\"}");
  }
}

void TenZillaWeb::handleProgramBreakDropThreshold() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  float threshold = 0.0f;
  
  int thresholdPos = body.indexOf("\"threshold\":");
  if (thresholdPos >= 0) {
    int startPos = thresholdPos + 12;
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String thresholdStr = body.substring(startPos, endPos);
      threshold = thresholdStr.toFloat();
    }
  }
  
  if (threshold > 0.0f && threshold <= 100.0f) {
    TenZillaProgram::setBreakDropThreshold(threshold);
    server.send(200, "application/json", "{\"status\":\"ok\",\"threshold\":" + String(threshold, 1) + "}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное значение (0.1-100.0%)\"}");
  }
}

void TenZillaWeb::handleProgramStop() {
  markWebRequestActivity();
  // Останавливаем программу и двигатель
  TenZillaScale::motorStop();
  TenZillaProgram::stopProgram(TenZillaProgram::STOP_REASON_WEB);
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

void TenZillaWeb::handleProgramStartCompression() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  if (TenZillaProgram::isRunning()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Program already running\"}");
    return;
  }
  
  // Блокируем запуск программы при отключенных ограничениях
  if (TenZillaScale::areLimitsDisabled()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Program blocked: displacement limits are disabled\"}");
    return;
  }
  
  TenZillaProgram::startCompressionProgram();
  server.send(200, "application/json", "{\"status\":\"started\",\"type\":\"compression\"}");
}

void TenZillaWeb::handleProgramStartBreak() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  if (TenZillaProgram::isRunning()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Program already running\"}");
    return;
  }
  
  // Блокируем запуск программы при отключенных ограничениях
  if (TenZillaScale::areLimitsDisabled()) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Program blocked: displacement limits are disabled\"}");
    return;
  }
  
  TenZillaProgram::startBreakProgram();
  server.send(200, "application/json", "{\"status\":\"started\",\"type\":\"break\"}");
}

// ============================================
// НАСТРОЙКИ NTP
// ============================================

void TenZillaWeb::handleNTPGet() {
  markWebRequestActivity();
  String response = "{";
  response += "\"ntpServer\":\"";
  response += TenZillaNTP::getNTPServer();
  response += "\",\"ntpInterval\":";
  response += String(TenZillaNTP::getNTPInterval());
  response += ",\"ntpSynced\":";
  response += TenZillaNTP::isTimeSynced() ? "true" : "false";
  response += ",\"ntpTimezone\":\"";
  response += TenZillaNTP::getTimezone();
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleNTPSetServer() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  String serverName = "";
  
  // Парсинг JSON: ищем "server":"строка"
  int serverPos = body.indexOf("\"server\":");
  if (serverPos >= 0) {
    int startPos = serverPos + 9; // после "server":
    int endPos = body.indexOf("\"", startPos + 1);
    if (endPos > startPos) {
      serverName = body.substring(startPos + 1, endPos);
    }
  }
  
  if (serverName.length() == 0 || serverName.length() > 63) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверное имя сервера (1-63 символа)\"}");
    return;
  }
  
  TenZillaNTP::setNTPServer(serverName.c_str());
  
  String response = "{";
  response += "\"status\":\"ok\",\"ntpServer\":\"";
  response += TenZillaNTP::getNTPServer();
  response += "\"}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleNTPSetInterval() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  unsigned long interval = 0;
  
  // Парсинг JSON: ищем "interval":число
  int intervalPos = body.indexOf("\"interval\":");
  if (intervalPos >= 0) {
    int startPos = intervalPos + 11; // после "interval":
    int endPos = body.indexOf(",", startPos);
    if (endPos < 0) endPos = body.indexOf("}", startPos);
    if (endPos > startPos) {
      String intervalStr = body.substring(startPos, endPos);
      interval = intervalStr.toInt();
    }
  }
  
  if (interval < 60 || interval > 86400) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверный интервал (60-86400 секунд)\"}");
    return;
  }
  
  TenZillaNTP::setNTPInterval(interval);
  
  String response = "{";
  response += "\"status\":\"ok\",\"ntpInterval\":";
  response += String(TenZillaNTP::getNTPInterval());
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleNTPSetTimezone() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }
  
  String body = server.arg("plain");
  String tz = "";
  
  // Парсинг JSON: ищем "timezone":"строка"
  int tzPos = body.indexOf("\"timezone\":");
  if (tzPos >= 0) {
    int startPos = tzPos + 11; // после "timezone":
    int firstQuote = body.indexOf("\"", startPos);
    if (firstQuote >= 0) {
      int endQuote = body.indexOf("\"", firstQuote + 1);
      if (endQuote > firstQuote) {
        tz = body.substring(firstQuote + 1, endQuote);
      }
    }
  }
  
  if (tz.length() == 0 || tz.length() >= 64) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Неверная строка часового пояса (1-63 символа)\"}");
    return;
  }
  
  TenZillaNTP::setTimezone(tz.c_str());
  
  String response = "{";
  response += "\"status\":\"ok\",\"ntpTimezone\":\"";
  response += TenZillaNTP::getTimezone();
  response += "\"}";
  server.send(200, "application/json", response);
}

// ============================================
// НАСТРОЙКИ ПРОИЗВОДИТЕЛЬНОСТИ ЧТЕНИЯ ТЕНЗОДАТЧИКА
// ============================================

void TenZillaWeb::handleScalePerformanceGet() {
  markWebRequestActivity();
  String response = "{";
  response += "\"minReadInterval\":";
  response += String(TenZillaScale::getMinReadInterval());
  response += ",\"nau7802WaitMs\":";
  response += String(TenZillaScale::getNAU7802WaitMs());
  response += ",\"i2cSpeed\":";
  response += String(TenZillaScale::getI2CSpeed());
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleScalePerformanceSetMinInterval() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }

  // Пытаемся получить значение из параметра запроса (?interval=...)
  unsigned long interval = 0;

  String intervalStr = server.arg("interval");
  String body; // для отладочного сообщения, если параметр не передан

  if (intervalStr.length() > 0) {
    interval = intervalStr.toInt();
  } else {
    // Fallback: поддержка старого варианта с JSON в теле
    body = server.arg("plain");
    body.trim();

    // Ищем "interval":число (с учетом пробелов)
    int intervalPos = body.indexOf("\"interval\"");
    if (intervalPos >= 0) {
      // Ищем двоеточие после "interval"
      int colonPos = body.indexOf(":", intervalPos);
      if (colonPos >= 0) {
        int startPos = colonPos + 1;
        // Пропускаем пробелы
        while (startPos < body.length() && (body[startPos] == ' ' || body[startPos] == '\t')) {
          startPos++;
        }
        // Ищем конец числа (запятая, закрывающая скобка или пробел)
        int endPos = startPos;
        while (endPos < body.length() &&
               body[endPos] != ',' && body[endPos] != '}' && body[endPos] != ' ' && body[endPos] != '\t') {
          endPos++;
        }
        if (endPos > startPos) {
          String intervalStrBody = body.substring(startPos, endPos);
          interval = intervalStrBody.toInt();
        }
      }
    }
  }

  if (interval < 10 || interval > 100 || interval == 0) {
    String errorMsg = "{\"status\":\"error\",\"message\":\"Неверный интервал (10-100 мс). Получено: ";
    errorMsg += String(interval);
    errorMsg += ", arg: ";
    errorMsg += intervalStr;
    if (body.length() > 0) {
      errorMsg += ", body: ";
      errorMsg += body;
    }
    errorMsg += "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }
  
  TenZillaScale::setMinReadInterval(interval);
  
  String response = "{";
  response += "\"status\":\"ok\",\"minReadInterval\":";
  response += String(TenZillaScale::getMinReadInterval());
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleScalePerformanceSetWaitMs() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }

  // Пытаемся получить значение из параметра запроса (?waitMs=...)
  unsigned long waitMs = 0;

  String waitStr = server.arg("waitMs");
  String body; // для отладки, если параметр не передан

  if (waitStr.length() > 0) {
    waitMs = waitStr.toInt();
  } else {
    // Fallback: поддержка старого варианта с JSON в теле
    body = server.arg("plain");
    body.trim();

    // Ищем "waitMs":число (с учетом пробелов)
    int waitPos = body.indexOf("\"waitMs\"");
    if (waitPos >= 0) {
      // Ищем двоеточие после "waitMs"
      int colonPos = body.indexOf(":", waitPos);
      if (colonPos >= 0) {
        int startPos = colonPos + 1;
        // Пропускаем пробелы
        while (startPos < body.length() && (body[startPos] == ' ' || body[startPos] == '\t')) {
          startPos++;
        }
        // Ищем конец числа (запятая, закрывающая скобка или пробел)
        int endPos = startPos;
        while (endPos < body.length() &&
               body[endPos] != ',' && body[endPos] != '}' && body[endPos] != ' ' && body[endPos] != '\t') {
          endPos++;
        }
        if (endPos > startPos) {
          String waitStrBody = body.substring(startPos, endPos);
          waitMs = waitStrBody.toInt();
        }
      }
    }
  }

  if (waitMs < 5 || waitMs > 50 || waitMs == 0) {
    String errorMsg = "{\"status\":\"error\",\"message\":\"Неверный таймаут (5-50 мс). Получено: ";
    errorMsg += String(waitMs);
    errorMsg += ", arg: ";
    errorMsg += waitStr;
    if (body.length() > 0) {
      errorMsg += ", body: ";
      errorMsg += body;
    }
    errorMsg += "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }
  
  TenZillaScale::setNAU7802WaitMs(waitMs);
  
  String response = "{";
  response += "\"status\":\"ok\",\"nau7802WaitMs\":";
  response += String(TenZillaScale::getNAU7802WaitMs());
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleScalePerformanceSetI2CSpeed() {
  markWebRequestActivity();
  if (!isClientAllowed()) {
    server.send(403, "application/json", "{\"status\":\"error\",\"message\":\"Connection limit exceeded\"}");
    return;
  }

  // Пытаемся получить значение из параметра запроса (?speed=...)
  unsigned long speed = 0;

  String speedStr = server.arg("speed");
  String body; // для отладки, если параметр не передан

  if (speedStr.length() > 0) {
    speed = speedStr.toInt();
  } else {
    // Fallback: поддержка старого варианта с JSON в теле
    body = server.arg("plain");
    body.trim();

    // Ищем "speed":число (с учетом пробелов)
    int speedPos = body.indexOf("\"speed\"");
    if (speedPos >= 0) {
      // Ищем двоеточие после "speed"
      int colonPos = body.indexOf(":", speedPos);
      if (colonPos >= 0) {
        int startPos = colonPos + 1;
        // Пропускаем пробелы
        while (startPos < body.length() && (body[startPos] == ' ' || body[startPos] == '\t')) {
          startPos++;
        }
        // Ищем конец числа (запятая, закрывающая скобка или пробел)
        int endPos = startPos;
        while (endPos < body.length() &&
               body[endPos] != ',' && body[endPos] != '}' && body[endPos] != ' ' && body[endPos] != '\t') {
          endPos++;
        }
        if (endPos > startPos) {
          String speedStrBody = body.substring(startPos, endPos);
          speed = speedStrBody.toInt();
        }
      }
    }
  }

  if (speed != 100000 && speed != 200000 && speed != 300000 && speed != 400000) {
    String errorMsg = "{\"status\":\"error\",\"message\":\"Неверная скорость (100000, 200000, 300000 или 400000 Гц). Получено: ";
    errorMsg += String(speed);
    errorMsg += ", arg: ";
    errorMsg += speedStr;
    if (body.length() > 0) {
      errorMsg += ", body: ";
      errorMsg += body;
    }
    errorMsg += "\"}";
    server.send(400, "application/json", errorMsg);
    return;
  }
  
  TenZillaScale::setI2CSpeed(speed);
  
  String response = "{";
  response += "\"status\":\"ok\",\"i2cSpeed\":";
  response += String(TenZillaScale::getI2CSpeed());
  response += "}";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleMeasurements() {
  markWebRequestActivity();
  String response;
  response.reserve(3072);
  response += "[";
  int n = TenZillaMeasurements::getCount();
  for (int i = 0; i < n; i++) {
    uint32_t ts;
    uint8_t type, outcome;
    float w;
    TenZillaMeasurements::getEntry(i, ts, type, outcome, w);
    if (i > 0) response += ",";
    response += "{\"ts\":";
    response += String((unsigned long)ts);
    response += ",\"type\":";
    response += String(type);
    response += ",\"outcome\":";
    response += String(outcome);
    response += ",\"weight\":";
    response += String(w, 1);
    response += "}";
  }
  response += "]";
  server.send(200, "application/json", response);
}

void TenZillaWeb::handleTelegramGet() {
  markWebRequestActivity();
  TenZillaSettings s = TenZillaConfig::get();
  String r;
  r.reserve(512);
  r += "{\"enabled\":";
  r += s.tgEnabled ? "true" : "false";
  r += ",\"token\":\"";
  for (size_t i = 0; s.tgBotToken[i]; i++) {
    if (s.tgBotToken[i] == '\\' || s.tgBotToken[i] == '"') r += '\\';
    r += s.tgBotToken[i];
  }
  r += "\",\"chatId\":\"";
  for (size_t i = 0; s.tgChatId[i]; i++) {
    if (s.tgChatId[i] == '\\' || s.tgChatId[i] == '"') r += '\\';
    r += s.tgChatId[i];
  }
  r += "\",\"notifyProgramResults\":";
  r += s.tgNotifyProgramResults ? "true" : "false";
  r += ",\"notifyStartup\":";
  r += s.tgNotifyStartup ? "true" : "false";
  r += ",\"notifyOverload\":";
  r += s.tgNotifyOverload ? "true" : "false";
  r += ",\"notifyStopped\":";
  r += s.tgNotifyStopped ? "true" : "false";
  r += "}";
  server.send(200, "application/json", r);
}

static String extractJsonString(const String& body, const char* key) {
  String needle = "\"";
  needle += key;
  needle += "\":\"";
  int pos = body.indexOf(needle);
  if (pos < 0) return "";
  int start = pos + (int)needle.length();
  String out;
  for (int i = start; i < (int)body.length(); i++) {
    char c = body[i];
    if (c == '\\' && i + 1 < (int)body.length()) {
      i++;
      if (body[i] == 'n') out += '\n';
      else if (body[i] == 'r') out += '\r';
      else out += body[i];
      continue;
    }
    if (c == '"') break;
    out += c;
  }
  return out;
}

static bool extractJsonBool(const String& body, const char* key, bool def) {
  String needle = "\"";
  needle += key;
  needle += "\":";
  int pos = body.indexOf(needle);
  if (pos < 0) return def;
  int start = pos + (int)needle.length();
  while (start < (int)body.length()) {
    char c = body[start];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { start++; continue; }
    break;
  }
  if (start + 4 <= (int)body.length() && body.substring(start, start + 4) == "true") return true;
  if (start + 5 <= (int)body.length() && body.substring(start, start + 5) == "false") return false;
  return def;
}

void TenZillaWeb::handleTelegramSet() {
  markWebRequestActivity();
  String body = server.arg("plain");
  TenZillaSettings s = TenZillaConfig::get();

  String token = extractJsonString(body, "token");
  String chatId = extractJsonString(body, "chatId");
  chatId.trim();  // avoid "chat not found" from spaces/newlines
  token.trim();
  bool hasToken = body.indexOf("\"token\":") >= 0;
  bool hasChatId = body.indexOf("\"chatId\":") >= 0;
  if (hasToken && token.length() > 0) {
    strncpy(s.tgBotToken, token.c_str(), sizeof(s.tgBotToken) - 1);
    s.tgBotToken[sizeof(s.tgBotToken) - 1] = '\0';
    Serial.print("TG save: token updated, len=");
    Serial.println(token.length());
  } else {
    Serial.println("TG save: token not updated (hasToken=" + String(hasToken ? 1 : 0) + " len=" + String(token.length()) + ")");
  }
  if (hasChatId && chatId.length() > 0) {
    strncpy(s.tgChatId, chatId.c_str(), sizeof(s.tgChatId) - 1);
    s.tgChatId[sizeof(s.tgChatId) - 1] = '\0';
    Serial.print("TG save: chatId updated, len=");
    Serial.println(chatId.length());
  } else {
    Serial.println("TG save: chatId not updated (hasChatId=" + String(hasChatId ? 1 : 0) + " len=" + String(chatId.length()) + ")");
  }
  s.tgEnabled = extractJsonBool(body, "enabled", s.tgEnabled);
  s.tgNotifyProgramResults = extractJsonBool(body, "notifyProgramResults", s.tgNotifyProgramResults);
  s.tgNotifyStartup = extractJsonBool(body, "notifyStartup", s.tgNotifyStartup);
  s.tgNotifyOverload = extractJsonBool(body, "notifyOverload", s.tgNotifyOverload);
  s.tgNotifyStopped = extractJsonBool(body, "notifyStopped", s.tgNotifyStopped);
  Serial.print("TG save: notifyStartup=");
  Serial.println(s.tgNotifyStartup ? 1 : 0);

  TenZillaConfig::set(s);
  TenZillaConfig::save();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void TenZillaWeb::handleTelegramTest() {
  markWebRequestActivity();
  TenZillaSettings s = TenZillaConfig::get();
  if (s.tgBotToken[0] == '\0') {
    server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"Укажите токен бота\"}");
    return;
  }
  if (s.tgChatId[0] == '\0') {
    server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"Укажите Chat ID\"}");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"WiFi не подключен\"}");
    return;
  }
  if (s_tgDeferred.pending) {
    server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"Отправка уже выполняется\"}");
    return;
  }
  s_tgDeferred.pending = true;
  s_tgDeferred.done = false;
  s_tgDeferred.err[0] = '\0';
  server.send(202, "application/json", "{\"status\":\"pending\"}");
}

void TenZillaWeb::handleTelegramTestStatus() {
  markWebRequestActivity();
  if (!s_tgDeferred.done) {
    server.send(200, "application/json", "{\"status\":\"pending\"}");
    return;
  }
  String esc;
  esc.reserve(96 + 16);
  for (size_t i = 0; i < sizeof(s_tgDeferred.err) && s_tgDeferred.err[i]; i++) {
    char c = s_tgDeferred.err[i];
    if (c == '\\' || c == '"') esc += '\\';
    esc += c;
  }
  String json = "{\"status\":\"done\",\"ok\":";
  json += s_tgDeferred.ok ? "true" : "false";
  json += ",\"message\":\"";
  json += esc;
  json += "\"}";
  s_tgDeferred.done = false;
  server.send(200, "application/json", json);
}

void TenZillaWeb::processDeferredTelegramTest() {
  if (!s_tgDeferred.pending) return;
  s_tgDeferred.pending = false;
  String errMsg;
  bool ok = TenZillaTelegram::sendTestWithError("TenZilla: тестовое сообщение", errMsg);
  s_tgDeferred.ok = ok;
  size_t n = errMsg.length();
  if (n >= sizeof(s_tgDeferred.err)) n = sizeof(s_tgDeferred.err) - 1;
  for (size_t i = 0; i < n; i++) s_tgDeferred.err[i] = errMsg[i];
  s_tgDeferred.err[n] = '\0';
  s_tgDeferred.done = true;
}

#if defined(ESP32)
void TenZillaWeb::handleUpdateGet() {
  markWebRequestActivity();
  server.sendHeader("Connection", "close");
  server.send(200, "text/html; charset=utf-8",
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>TenZilla OTA</title><style>"
    "body{font-family:sans-serif;background:#1a1a1a;color:#fff;margin:20px;}"
    "h1{color:#ff8800;} .box{background:#2d2d2d;padding:20px;border-radius:8px;margin:15px 0;}"
    "input[type=file]{margin:10px 0;} button{background:#007bff;color:#fff;border:none;padding:12px 24px;border-radius:8px;cursor:pointer;} button:disabled{opacity:0.6;cursor:not-allowed;}"
    "button:hover:not(:disabled){background:#0056b3;} a{color:#007bff;} small{color:#888;}"
    ".pg{background:#404040;height:24px;border-radius:6px;overflow:hidden;margin:12px 0;}"
    ".pg-inner{background:#007bff;height:100%;width:0%;transition:width 0.2s;}"
    ".pg-txt{margin-top:6px;font-size:14px;color:#b0b0b0;}"
    ".ok{color:#28a745;} .err{color:#dc3545;}"
    "</style></head><body>"
    "<h1>Обновление прошивки</h1>"
    "<div class=\"box\"><p>Выберите файл <strong>.bin</strong> (Sketch &rarr; Export compiled Binary).</p>"
    "<input type=\"file\" id=\"otaFile\" accept=\".bin\"><br>"
    "<div class=\"pg\"><div class=\"pg-inner\" id=\"otaBar\"></div></div>"
    "<p class=\"pg-txt\" id=\"otaTxt\">0 / 0 KB (0%)</p>"
    "<button type=\"button\" id=\"otaBtn\">Загрузить прошивку</button>"
    "<p id=\"otaStatus\" style=\"margin-top:12px;\"></p></div>"
    "<p><a href=\"/\">&larr; Назад в панель</a></p>"
    "<script>"
    "(function(){"
    "var inp=document.getElementById('otaFile');"
    "var btn=document.getElementById('otaBtn');"
    "var bar=document.getElementById('otaBar');"
    "var txt=document.getElementById('otaTxt');"
    "var st=document.getElementById('otaStatus');"
    "function fmt(v){return (v/1024).toFixed(1);}"
    "function pct(l,t){return t?Math.round(100*l/t):0;}"
    "btn.onclick=function(){"
    "var f=inp.files[0];"
    "if(!f){st.innerHTML='<span class=err>Выберите файл .bin</span>';return;}"
    "if(!f.name.toLowerCase().endsWith('.bin')){st.innerHTML='<span class=err>Нужен файл .bin</span>';return;}"
    "var fd=new FormData();"
    "fd.append('firmware',f);"
    "var xhr=new XMLHttpRequest();"
    "btn.disabled=true;"
    "st.textContent='';"
    "bar.style.width='0%';"
    "txt.textContent='0 / '+fmt(f.size)+' KB (0%)';"
    "xhr.upload.onprogress=function(e){"
    "if(!e.lengthComputable)return;"
    "var p=e.loaded/e.total;"
    "bar.style.width=Math.round(p*100)+'%';"
    "txt.textContent=fmt(e.loaded)+' / '+fmt(e.total)+' KB ('+pct(e.loaded,e.total)+'%)';"
    "};"
    "xhr.onload=function(){"
    "btn.disabled=false;"
    "if(xhr.status===200){st.innerHTML='<span class=ok>Прошивка обновлена. Устройство перезагружается...</span>';}"
    "else{st.innerHTML='<span class=err>Ошибка '+xhr.status+'. Убедитесь, что файл .bin.</span>';}"
    "};"
    "xhr.onerror=function(){btn.disabled=false;st.innerHTML='<span class=err>Ошибка сети. Возможно, устройство перезагрузилось во время загрузки.</span>';};"
    "xhr.open('POST','/update');"
    "xhr.send(fd);"
    "};"
    "})();"
    "</script></body></html>");
}

void TenZillaWeb::handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    s_otaStarted = false;
    s_otaLastLogBytes = 0;
    s_otaLastHeapLog = 0;
    s_otaFirstChunk = true;
    markWebRequestActivity();
    const char* fn = upload.filename.c_str();
    size_t flen = strlen(fn);
    int nameOk = (flen >= 4 && strcmp(fn + flen - 4, ".bin") == 0) ? 1 : 0;
    if (strcmp(upload.name.c_str(), "firmware") != 0) nameOk = 0;
    if (nameOk) {
      Serial.println("OTA: --- start ---");
      Serial.print("OTA: file ");
      Serial.println(fn);
      Serial.print("OTA: free heap ");
      Serial.println(ESP.getFreeHeap());
      if (Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        s_otaStarted = true;
      } else {
        Update.printError(Serial);
      }
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && s_otaStarted) {
    markWebRequestActivity();
    size_t n = upload.currentSize;
    if (s_otaFirstChunk) {
      s_otaFirstChunk = false;
      Serial.print("OTA: first chunk ");
      Serial.print(n);
      Serial.print(" B, total ");
      Serial.println(upload.totalSize);
    }
    if (n == 0) { yield(); return; }
    const size_t kChunk = 512;
    size_t off = 0;
    bool ok = true;
    while (off < n && ok) {
      size_t piece = (n - off) > kChunk ? kChunk : (n - off);
      if (Update.write(upload.buf + off, piece) != piece) { ok = false; break; }
      off += piece;
      yield();
#ifdef ESP32
      vTaskDelay(0);
#endif
    }
    if (!ok) {
      Serial.print("OTA: write fail at ");
      Serial.print(upload.totalSize);
      Serial.println(" bytes");
      Update.printError(Serial);
      s_otaStarted = false;
    } else {
      uint32_t total = upload.totalSize;
      if (total - s_otaLastLogBytes >= 32768u) {
        s_otaLastLogBytes = total;
        Serial.print("OTA: ");
        Serial.print(total / 1024);
        Serial.println(" KB");
        if (total - s_otaLastHeapLog >= 131072u) {
          s_otaLastHeapLog = total;
          Serial.print("OTA: heap ");
          Serial.println(ESP.getFreeHeap());
        }
      }
      yield();
#ifdef ESP32
      vTaskDelay(0);
#endif
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    markWebRequestActivity();
    if (s_otaStarted) {
      Serial.print("OTA: end, total ");
      Serial.print(upload.totalSize);
      Serial.println(" bytes");
      if (Update.end(true)) {
        Serial.println("OTA: OK, restart pending");
      } else {
        Update.printError(Serial);
        s_otaStarted = false;
      }
    }
  }
}

void TenZillaWeb::handleUpdatePost() {
  markWebRequestActivity();
  bool ok = s_otaStarted && !Update.hasError();
  server.sendHeader("Connection", "close");
  if (ok) {
    server.send(200, "text/html; charset=utf-8",
      "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<title>TenZilla OTA</title><style>body{font-family:sans-serif;background:#1a1a1a;color:#fff;margin:20px;} .ok{color:#28a745;}</style></head><body>"
      "<p class=\"ok\">Прошивка обновлена. Устройство перезагрузится через 2 с...</p>"
      "<p><a href=\"/\">Перейти в панель</a></p></body></html>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(500, "text/html; charset=utf-8",
      "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>TenZilla OTA</title></head><body>"
      "<p style=\"color:#dc3545;\">Ошибка обновления. Убедитесь, что выбран файл .bin.</p>"
      "<p><a href=\"/update\">Повторить</a> | <a href=\"/\">Панель</a></p></body></html>");
  }
}
#endif

// HTML страница хранится в PROGMEM (flash памяти) вместо RAM для экономии памяти
// На ESP32 PROGMEM автоматически помещает const данные в flash, но явное указание улучшает читаемость
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>TenZilla Control Panel</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <style>
    :root {
      --bg-primary: #1a1a1a;
      --bg-secondary: #2d2d2d;
      --bg-tertiary: #3d3d3d;
      --text-primary: #ffffff;
      --text-secondary: #b0b0b0;
      --accent: #007bff;
      --accent-hover: #0056b3;
      --success: #28a745;
      --warning: #ffc107;
      --danger: #dc3545;
    }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: var(--bg-primary);
      color: var(--text-primary);
      line-height: 1.6;
    }
    
    .container {
      background: var(--bg-secondary);
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 8px 25px rgba(0,0,0,0.3);
      border: 1px solid #404040;
      max-width: 100%;
      box-sizing: border-box;
      overflow-x: hidden;
    }
    
    h1 {
      color: var(--text-primary);
      text-align: center;
      margin-bottom: 30px;
      font-weight: 300;
      font-size: 2.2em;
    }
    
    .tabs {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
      margin-bottom: 20px;
      border-bottom: 1px solid #404040;
    }
    
    .tab {
      padding: 10px 16px;
      background: var(--bg-tertiary);
      border: none;
      color: var(--text-secondary);
      cursor: pointer;
      font-size: 14px;
      border-radius: 8px 8px 0 0;
      transition: all 0.3s ease;
    }
    
    .tab.active {
      background: var(--accent);
      color: white;
    }
    
    .tab-content {
      display: none;
    }
    
    .tab-content.active {
      display: block;
    }
    
    .form-group {
      margin-bottom: 25px;
    }
    
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: var(--text-primary);
    }
    
    input[type="text"],
    input[type="password"],
    input[type="number"] {
      width: 100%;
      padding: 12px 15px;
      background: var(--bg-tertiary);
      border: 1px solid #404040;
      border-radius: 8px;
      font-size: 16px;
      box-sizing: border-box;
      color: var(--text-primary);
      transition: all 0.3s ease;
    }
    
    input[type="checkbox"] {
      width: 24px;
      height: 24px;
      cursor: pointer;
      margin-right: 10px;
      accent-color: var(--accent);
      flex-shrink: 0;
    }
    
    input:focus {
      outline: none;
      border-color: var(--accent);
      box-shadow: 0 0 0 2px rgba(0, 123, 255, 0.25);
    }
    
    .checkbox-label {
      display: flex;
      align-items: center;
      gap: 10px;
      cursor: pointer;
      padding: 10px;
      border-radius: 8px;
      transition: background 0.2s ease;
    }
    
    .checkbox-label:hover {
      background: var(--bg-tertiary);
    }
    
    button {
      background: linear-gradient(135deg, var(--accent), #0056b3);
      color: white;
      padding: 14px 30px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      font-size: 16px;
      font-weight: 600;
      transition: all 0.3s ease;
    }
    
    button:hover {
      background: linear-gradient(135deg, var(--accent-hover), #004085);
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0, 123, 255, 0.3);
    }
    
    button:active {
      transform: translateY(0);
    }
    
    .status {
      padding: 20px;
      border-radius: 10px;
      margin-bottom: 25px;
      text-align: center;
      font-weight: 600;
      border: 1px solid;
    }
    
    .connected { background: rgba(40, 167, 69, 0.1); color: var(--success); border-color: var(--success); }
    .disconnected { background: rgba(220, 53, 69, 0.1); color: var(--danger); border-color: var(--danger); }
    
    .info-panel {
      background: var(--bg-tertiary);
      padding: 20px;
      border-radius: 10px;
      margin-bottom: 25px;
      border-left: 4px solid var(--accent);
    }
    
    .ap-info {
      background: rgba(0, 123, 255, 0.1);
      padding: 15px;
      border-radius: 8px;
      margin-bottom: 20px;
      text-align: center;
      border: 1px solid var(--accent);
    }
    
    .data-value {
      font-size: 16px;
      font-weight: bold;
      color: var(--accent);
      padding: 12px 15px;
      background: var(--bg-tertiary);
      border: 1px solid #404040;
      border-radius: 8px;
      display: inline-block;
      min-width: 100px;
      text-align: center;
      box-sizing: border-box;
    }
    
    .reset-btn {
      background: var(--danger);
      color: white;
      padding: 12px 15px;
      border: 1px solid #404040;
      border-radius: 8px;
      cursor: pointer;
      font-size: 16px;
      font-weight: 600;
      transition: all 0.3s ease;
      height: auto;
    }
    
    .reset-btn:hover {
      background: #c82333;
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(220, 53, 69, 0.3);
    }
    
    .stability-stable {
      color: #28a745 !important;
      background: rgba(40, 167, 69, 0.1) !important;
    }
    
    .stability-unstable {
      color: #dc3545 !important;
      background: rgba(220, 53, 69, 0.1) !important;
    }
    
    .motor-controls {
      display: flex;
      justify-content: center;
      gap: 20px;
      margin: 30px 0;
      flex-wrap: wrap;
    }
    
    .motor-btn {
      width: 90px;
      height: 90px;
      border-radius: 50%;
      font-size: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      touch-action: manipulation;
      -webkit-tap-highlight-color: rgba(255, 255, 255, 0.3);
      -webkit-touch-callout: none;
      user-select: none;
      border: 2px solid rgba(255, 255, 255, 0.2);
      position: relative;
      overflow: hidden;
      transition: opacity 0.05s, transform 0.05s;
    }
    
    .motor-btn:active {
      opacity: 0.7;
      transform: scale(0.95);
    }
    
    /* Кнопка вверх - зеленая стрелка вверх (как на TFT) */
    .btn-up {
      background: #008000;
      border-color: #006600;
    }
    
    .btn-up::before {
      content: '';
      position: absolute;
      width: 0;
      height: 0;
      border-left: 18px solid transparent;
      border-right: 18px solid transparent;
      border-bottom: 26px solid #ffffff;
      top: 18px;
      left: 50%;
      margin-left: -18px;
    }
    
    .btn-up::after {
      content: '';
      position: absolute;
      width: 12px;
      height: 24px;
      background: #ffffff;
      top: 44px;
      left: 50%;
      margin-left: -6px;
      border-radius: 2px;
    }
    
    /* Кнопка вниз - синяя стрелка вниз (как на TFT) */
    .btn-down {
      background: #00cccc;
      border-color: #00aaaa;
    }
    
    .btn-down::before {
      content: '';
      position: absolute;
      width: 0;
      height: 0;
      border-left: 18px solid transparent;
      border-right: 18px solid transparent;
      border-top: 26px solid #ffffff;
      bottom: 18px;
      left: 50%;
      margin-left: -18px;
    }
    
    .btn-down::after {
      content: '';
      position: absolute;
      width: 12px;
      height: 24px;
      background: #ffffff;
      bottom: 44px;
      left: 50%;
      margin-left: -6px;
      border-radius: 2px;
    }
    
    /* Кнопка стоп - красный круг с белым прямоугольником (как на TFT) */
    .btn-stop {
      background: #cc0000;
      border-color: #990000;
    }
    
    .btn-stop::before {
      content: '';
      position: absolute;
      width: 68px;
      height: 68px;
      border-radius: 50%;
      background: #cc0000;
      border: 2px solid #990000;
      top: 50%;
      left: 50%;
      margin-top: -34px;
      margin-left: -34px;
    }
    
    .btn-stop::after {
      content: '';
      position: absolute;
      width: 46px;
      height: 23px;
      background: #ffffff;
      border-radius: 4px;
      border: 1px solid #cccccc;
      top: 50%;
      left: 50%;
      margin-top: -11.5px;
      margin-left: -23px;
    }
    
    /* Оптимизация для мобильных устройств */
    @media (max-width: 768px) {
      body {
        padding: 10px;
      }
      
      .container {
        padding: 15px;
      }
      
      h1 {
        font-size: 1.8em;
        margin-bottom: 15px;
      }
      
      .tabs {
        flex-wrap: wrap;
        gap: 5px;
      }
      
      .tab {
        padding: 10px 16px;
        font-size: 14px;
        flex: 1 1 auto;
        min-width: calc(50% - 4px);
      }
      
      .data-row {
        font-size: 1.2em;
        margin: 12px 0;
      }
      
      .data-value {
        font-size: 1.1em;
        min-width: 60px;
      }
      
      .btn-up::before {
        border-left-width: 14px;
        border-right-width: 14px;
        border-bottom-width: 20px;
        top: 14px;
        margin-left: -14px;
      }
      
      .btn-up::after {
        width: 9px;
        height: 17px;
        top: 33px;
        margin-left: -4.5px;
      }
      
      .btn-down::before {
        border-left-width: 14px;
        border-right-width: 14px;
        border-top-width: 20px;
        bottom: 14px;
        margin-left: -14px;
      }
      
      .btn-down::after {
        width: 9px;
        height: 17px;
        bottom: 33px;
        margin-left: -4.5px;
      }
      
      .btn-stop::before {
        width: 52px;
        height: 52px;
        margin-top: -26px;
        margin-left: -26px;
      }
      
      .btn-stop::after {
        width: 35px;
        height: 18px;
        margin-top: -9px;
        margin-left: -17.5px;
      }
      
      button {
        padding: 16px 24px;
        font-size: 16px;
        min-height: 44px;
        touch-action: manipulation;
        -webkit-tap-highlight-color: rgba(255, 255, 255, 0.3);
        -webkit-touch-callout: none;
        user-select: none;
        transition: opacity 0.05s, transform 0.05s;
      }
      
      button:active {
        opacity: 0.7;
        transform: scale(0.98);
      }
      
      input[type="text"],
      input[type="password"],
      input[type="number"] {
        padding: 14px 15px;
        font-size: 16px;
        min-height: 44px;
      }
      
      .form-group {
        margin-bottom: 20px;
      }
      
      .calibration-group {
        background: var(--bg-secondary);
        padding: 15px;
        border-radius: 8px;
        margin-bottom: 20px;
      }
      
      .info-panel {
        padding: 15px;
        margin-bottom: 15px;
      }
      
      .status {
        padding: 15px;
        margin-bottom: 15px;
      }
    }
    
    @media (max-width: 480px) {
      h1 {
        font-size: 1.5em;
      }
      
      .tab {
        font-size: 12px;
        padding: 8px 12px;
        min-width: calc(50% - 3px);
      }
      
      .weight-display {
        font-size: 1.8em;
      }
      
      .max-weight {
        font-size: 1.1em;
      }
      
      .motor-btn {
        width: 100px;
        height: 100px;
      }
      
      .btn-up::before {
        border-left-width: 20px;
        border-right-width: 20px;
        border-bottom-width: 30px;
        top: 20px;
        margin-left: -20px;
      }
      
      .btn-up::after {
        width: 14px;
        height: 26px;
        top: 50px;
        margin-left: -7px;
      }
      
      .btn-down::before {
        border-left-width: 20px;
        border-right-width: 20px;
        border-top-width: 30px;
        bottom: 20px;
        margin-left: -20px;
      }
      
      .btn-down::after {
        width: 14px;
        height: 26px;
        bottom: 50px;
        margin-left: -7px;
      }
      
      .btn-stop::before {
        width: 76px;
        height: 76px;
        margin-top: -38px;
        margin-left: -38px;
      }
      
      .btn-stop::after {
        width: 52px;
        height: 26px;
        margin-top: -13px;
        margin-left: -26px;
      }
      
      .counter {
        font-size: 1.1em;
      }
    }
    
    /* Предотвращение выделения текста при касании */
    * {
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
    }
    
    /* Улучшенная чувствительность для всех кнопок */
    button {
      -webkit-tap-highlight-color: rgba(255, 255, 255, 0.3);
      -webkit-touch-callout: none;
      touch-action: manipulation;
    }
    
    input, textarea {
      -webkit-user-select: text;
      -moz-user-select: text;
      -ms-user-select: text;
      user-select: text;
    }
    
    /* Предотвращение zoom при двойном касании */
    * {
      touch-action: manipulation;
    }
    
    .calibration-group {
      background: var(--bg-tertiary);
      padding: 20px;
      border-radius: 10px;
      margin-bottom: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🦎 TenZilla Control Panel</h1>
    <div id="headerBar" style="display: flex; justify-content: flex-end; align-items: center; gap: 20px; margin-bottom: 12px; padding: 8px 12px; background: var(--bg-secondary); border-radius: 8px; font-size: 0.95em;">
      <span id="headerTime">—</span>
      <span id="headerUptime">—</span>
    </div>
    <div class="tabs">
      <button class="tab active" data-tab="compression" onclick="showTab('compression'); return false;" ontouchend="showTab('compression'); return false;">💪 СЖАТИЕ</button>
      <button class="tab" data-tab="break" onclick="showTab('break'); return false;" ontouchend="showTab('break'); return false;">💥 РАЗРЫВ</button>
      <button class="tab" data-tab="wifi" onclick="showTab('wifi'); return false;" ontouchend="showTab('wifi'); return false;">📡 WiFi</button>
      <button class="tab" data-tab="scaleSettings" onclick="showTab('scaleSettings'); return false;" ontouchend="showTab('scaleSettings'); return false;">⚖️ Тензодатчик</button>
      <button class="tab" data-tab="motorSettings" onclick="showTab('motorSettings'); return false;" ontouchend="showTab('motorSettings'); return false;">🔧 Двигатель</button>
      <button class="tab" data-tab="otherSettings" onclick="showTab('otherSettings'); return false;" ontouchend="showTab('otherSettings'); return false;">⚙️ Прочие</button>
      <button class="tab" data-tab="history" onclick="showTab('history'); return false;" ontouchend="showTab('history'); return false;">📋 История</button>
    </div>
    
    <!-- Вкладка СЖАТИЕ -->
    <div id="compression" class="tab-content active">
      <div class="ap-info">
        <strong>Экран:</strong> <span>СЖАТИЕ</span>
      </div>
      
      <div class="form-group">
        <label><strong>Текущее значение:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="currentWeightCompression" style="flex: 0 0 auto;">0.0</span> <span>Н</span>
        </div>
      </div>
      
      <div class="form-group">
        <label><strong>Максимум:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="maxWeightCompression" style="flex: 0 0 auto;">0.0</span> <span>Н</span>
          <button onclick="resetCompressionMax()" class="reset-btn">🔄 Сброс</button>
        </div>
      </div>
      
      <div class="form-group">
        <label><strong>Перемещение:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="displacementCompressionWork" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
          <span style="margin: 0 6px;">/</span>
          <span class="data-value" id="displacementCompressionAbs" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
        </div>
      </div>
      
      <div class="form-group" style="margin-top: 20px;">
        <label><strong>Статус программы:</strong></label>
        <div style="background: var(--bg-tertiary); padding: 15px; border-radius: 8px; min-height: 50px;">
          <div id="programStatusCompression" style="font-size: 18px; font-weight: bold; color: var(--text-secondary);">Готово</div>
        </div>
      </div>
      
      <div class="form-group" style="margin-top: 20px; text-align: center;">
        <button onclick="startCompressionProgram()" id="startCompressionBtn" style="width: 100%; padding: 20px; font-size: 20px; font-weight: bold; background: var(--success);">▶️ Запустить программу СЖАТИЕ</button>
      </div>
      
      <div class="motor-controls">
        <button class="motor-btn btn-up" onmousedown="startMotor('up')" onmouseup="stopMotor()" onmouseleave="stopMotor()" ontouchstart="startMotor('up')" ontouchend="stopMotor()" ontouchcancel="stopMotor()"></button>
        <button class="motor-btn btn-stop" onclick="stopProgramAndMotor()" ontouchend="stopProgramAndMotor()" ontouchstart="stopProgramAndMotor()"></button>
        <button class="motor-btn btn-down" onmousedown="startMotor('down')" onmouseup="stopMotor()" onmouseleave="stopMotor()" ontouchstart="startMotor('down')" ontouchend="stopMotor()" ontouchcancel="stopMotor()"></button>
      </div>
    </div>
    
    <!-- Вкладка РАЗРЫВ -->
    <div id="break" class="tab-content">
      <div class="ap-info">
        <strong>Экран:</strong> <span>РАЗРЫВ</span>
      </div>
      
      <div class="form-group">
        <label><strong>Текущее значение:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="currentWeightBreak" style="flex: 0 0 auto;">0.0</span> <span>Н</span>
        </div>
      </div>
      
      <div class="form-group">
        <label><strong>Максимум:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="maxWeightBreak" style="flex: 0 0 auto;">0.0</span> <span>Н</span>
          <button onclick="resetBreakMax()" class="reset-btn">🔄 Сброс</button>
        </div>
      </div>
      
      <div class="form-group">
        <label><strong>Перемещение:</strong></label>
        <div style="display: flex; gap: 10px; align-items: center;">
          <span class="data-value" id="displacementBreakWork" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
          <span style="margin: 0 6px;">/</span>
          <span class="data-value" id="displacementBreakAbs" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
        </div>
      </div>
      
      <div class="form-group" style="margin-top: 20px;">
        <label><strong>Статус программы:</strong></label>
        <div style="background: var(--bg-tertiary); padding: 15px; border-radius: 8px; min-height: 50px;">
          <div id="programStatusBreak" style="font-size: 18px; font-weight: bold; color: var(--text-secondary);">Готово</div>
        </div>
      </div>
      
      <div class="form-group" style="margin-top: 20px; text-align: center;">
        <button onclick="startBreakProgram()" id="startBreakBtn" style="width: 100%; padding: 20px; font-size: 20px; font-weight: bold; background: var(--danger);">▶️ Запустить программу РАЗРЫВ</button>
      </div>
      
      <div class="motor-controls">
        <button class="motor-btn btn-up" onmousedown="startMotor('up')" onmouseup="stopMotor()" onmouseleave="stopMotor()" ontouchstart="startMotor('up')" ontouchend="stopMotor()" ontouchcancel="stopMotor()"></button>
        <button class="motor-btn btn-stop" onclick="stopProgramAndMotor()" ontouchend="stopProgramAndMotor()" ontouchstart="stopProgramAndMotor()"></button>
        <button class="motor-btn btn-down" onmousedown="startMotor('down')" onmouseup="stopMotor()" onmouseleave="stopMotor()" ontouchstart="startMotor('down')" ontouchend="stopMotor()" ontouchcancel="stopMotor()"></button>
      </div>
    </div>
    
    <!-- Вкладка WiFi -->
    <div id="wifi" class="tab-content">
      <div class="ap-info">
        <strong>AP Name:</strong> <span id="apName">TenZilla_XXXX</span><br>
        <small>Подключитесь к этой сети для настройки WiFi</small>
      </div>
      
      <div id="wifiStatus" class="status disconnected">
        <span id="wifiStatusText">Не подключено к WiFi</span>
      </div>
      
      <div class="info-panel" id="wifiInfoPanel" style="display:none;">
        <h3>📶 Информация о подключении</h3>
        <p><strong>SSID:</strong> <span id="currentSSID"></span></p>
        <p><strong>IP адрес:</strong> <span id="ipAddress"></span></p>
        <p><strong>Уровень сигнала:</strong> <span id="rssi"></span> dBm</p>
      </div>

      <form id="wifiForm">
        <div class="form-group">
          <label for="ssid">📡 WiFi SSID:</label>
          <input type="text" id="ssid" name="ssid" placeholder="Введите имя сети WiFi" required>
        </div>
        <div class="form-group">
          <label for="password">🔑 Пароль WiFi:</label>
          <input type="password" id="password" name="password" placeholder="Введите пароль WiFi">
        </div>
        <button type="submit">💾 Сохранить и подключить</button>
      </form>

      <div class="info-panel" style="margin-top: 24px; padding: 20px;">
        <h3>📱 Telegram бот</h3>
        <p style="color: var(--text-secondary); font-size: 0.95em; margin-bottom: 16px;">Уведомления о результатах программ, включении системы, ошибках перегрузки и принудительной остановке.</p>
        <div class="form-group">
          <label class="checkbox-label">
            <input type="checkbox" id="tgEnabled" name="tgEnabled">
            <span>Включить уведомления</span>
          </label>
        </div>
        <div class="form-group">
          <label for="tgToken">Токен бота:</label>
          <input type="password" id="tgToken" name="tgToken" placeholder="123456:ABC-DEF..." autocomplete="off">
        </div>
        <div class="form-group">
          <label for="tgChatId">Chat ID:</label>
          <input type="text" id="tgChatId" name="tgChatId" placeholder="123456789 или -1001234567890">
          <small style="display:block;color:var(--muted);margin-top:4px;">Для личного чата сначала напишите боту /start в Telegram. Узнать свой Chat ID: @userinfobot или getUpdates API.</small>
        </div>
        <div class="form-group" style="margin-top: 12px;">
          <span style="display: block; margin-bottom: 8px; font-weight: 600;">Отправлять:</span>
          <label class="checkbox-label"><input type="checkbox" id="tgNotifyResults" name="tgNotifyResults"> Результаты программ (завершение СЖАТИЕ/РАЗРЫВ)</label>
          <label class="checkbox-label"><input type="checkbox" id="tgNotifyStartup" name="tgNotifyStartup"> Включение системы</label>
          <label class="checkbox-label"><input type="checkbox" id="tgNotifyOverload" name="tgNotifyOverload"> Ошибки перегрузки и лимитов</label>
          <label class="checkbox-label"><input type="checkbox" id="tgNotifyStopped" name="tgNotifyStopped"> Принудительная остановка</label>
        </div>
        <button type="button" onclick="saveTelegramSettings()" id="tgSaveBtn">💾 Сохранить Telegram</button>
        <button type="button" onclick="sendTelegramTest()" id="tgTestBtn" style="margin-left: 8px;">📤 Отправить тест</button>
        <span id="tgStatus" style="margin-left: 10px; font-size: 0.9em;"></span>
      </div>
      
      <div style="margin-top: 20px; text-align: center;">
        <button onclick="resetConfig()" style="background: var(--danger);">🔄 Сбросить настройки</button>
      </div>
      <div style="margin-top: 20px; text-align: center;">
        <button onclick="restartDevice()" style="background: var(--warning); width: 100%; padding: 15px; font-size: 18px; font-weight: bold;">
          🔄 Перезапустить устройство
        </button>
        <small style="color: var(--text-secondary); display: block; margin-top: 10px;">
          Перезапуск устройства для применения всех изменений настроек
        </small>
      </div>
    </div>
    
    <!-- Вкладка Настройки тензодатчика -->
    <div id="scaleSettings" class="tab-content">
      <div class="calibration-group">
        <h3>⚖️ Настройки тензодатчика</h3>
        
        <!-- Отображение значений -->
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group" style="margin-bottom: 15px;">
            <label><strong>Текущее значение:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="currentWeightN" step="0.1" placeholder="0.0" readonly style="width: auto; flex: 0 0 auto; background: var(--bg-tertiary); cursor: not-allowed;"> <span>Н</span>
              <input type="number" id="currentRaw" step="1" placeholder="0" readonly style="width: auto; flex: 0 0 auto; background: var(--bg-tertiary); cursor: not-allowed;"> <span>RAW</span>
            </div>
          </div>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label><strong>Нулевая точка:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="zeroWeightNInput" value="0" step="0.1" readonly style="width: auto; flex: 0 0 auto; background: var(--bg-tertiary); cursor: not-allowed;"> <span>Н</span>
              <input type="number" id="zeroRawInput" step="1" placeholder="-" readonly style="width: auto; flex: 0 0 auto; background: var(--bg-tertiary); cursor: not-allowed;"> <span>RAW</span>
              <button onclick="saveZeroPoint()">🔄 Обнулить</button>
            </div>
            <span id="zeroPointStatus" style="margin-top: 5px; display: block; font-size: 0.9em; color: var(--text-secondary);"></span>
          </div>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label><strong>Вторая точка:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="calibrationWeightNInput" step="0.1" min="0.1" placeholder="-" style="width: auto; flex: 0 0 auto;"> <span>Н</span>
              <input type="number" id="calibrationRawInput" step="1" placeholder="-" style="width: auto; flex: 0 0 auto;"> <span>RAW</span>
              <button onclick="saveCalibrationPoint()">💾 Сохранить</button>
            </div>
            <span id="calibrationPointStatus" style="margin-top: 5px; display: block; font-size: 0.9em; color: var(--text-secondary);"></span>
          </div>
        </div>
        
        <!-- Пошаговая калибровка -->
        <div id="calibrationSteps" style="display: none;">
          <div class="form-group">
            <label>Шаг 1: Разгрузите платформу и нажмите "Записать ноль"</label>
            <button onclick="recordZero()" id="zeroBtn">📝 Записать ноль</button>
            <span id="zeroStatus" style="margin-left: 10px;"></span>
          </div>
          
          <div class="form-group" id="weightInputGroup" style="display: none;">
            <label for="calibrationWeight">Шаг 2: Укажите калибровочный вес (Ньютоны):</label>
            <input type="number" id="calibrationWeight" step="0.1" min="0.1" placeholder="Например: 10.0">
            <button onclick="recordCalibrationPoint()" id="calBtn">📝 Записать калибровку</button>
            <span id="calStatus" style="margin-left: 10px;"></span>
          </div>
        </div>
        
        <div class="form-group" style="margin-top: 10px;">
          <label><strong>Абсолютное перемещение:</strong></label>
          <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <span class="data-value" id="displacementCal" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
            <button onclick="resetCounter()" class="reset-btn">🔄 Обнулить</button>
          </div>
        </div>
        
        <button onclick="startCalibration()" id="startCalBtn" style="width: 100%; margin-bottom: 20px;">🎯 Начать калибровку</button>
        
        <!-- Шум и стабильность -->
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group" style="margin-bottom: 15px;">
            <label><strong>Шум:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <span class="data-value" id="noiseLevel">0.00</span> <span>%</span>
            </div>
          </div>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label><strong>Стабильность:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <span class="data-value" id="stabilityStatus" style="flex: 0 0 auto;">-</span>
            </div>
          </div>
        </div>
        
        <!-- Порог шума и максимальный вес -->
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group">
            <label for="noiseThresholdInput">Порог шума (0.1-50.0%):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="noiseThresholdInput" step="0.1" min="0.1" max="50.0" placeholder="Например: 5.0">
              <span>%</span>
              <button onclick="setNoiseThreshold()">💾 Сохранить</button>
            </div>
            <span id="noiseThresholdStatus" style="margin-top: 5px; display: block; font-size: 0.9em;"></span>
          </div>
          
          <div class="form-group">
            <label for="maxWeightInput">Предел перегрузки:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="maxWeightInput" step="0.1" min="0.1" placeholder="В ньютонах">
              <button onclick="setMaxWeight()">💾 Установить</button>
            </div>
          </div>
          <div class="form-group">
            <label for="negativeWeightLimitInput">Порог отрицательного веса (Н):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="negativeWeightLimitInput" step="1" placeholder="-50">
              <button onclick="setNegativeWeightLimit()">💾 Установить</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 4px;">При весе ниже этого значения — аварийный стоп и гудок (по умолчанию -50 Н).</small>
          </div>
        </div>
        
        <!-- Настройки производительности чтения -->
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <h4 style="margin-top: 0; margin-bottom: 15px;">⚡ Настройки производительности чтения</h4>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label for="minReadIntervalSelect">Минимальный интервал между чтениями (мс):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <select id="minReadIntervalSelect" style="padding: 12px 15px; background: var(--bg-tertiary); border: 1px solid #404040; border-radius: 8px; font-size: 16px; color: var(--text-primary); flex: 0 0 auto;">
                <option value="10">10 мс (~100 Гц)</option>
                <option value="15">15 мс (~66 Гц)</option>
                <option value="20">20 мс (~50 Гц)</option>
                <option value="25" selected>25 мс (~40 Гц) - по умолчанию</option>
                <option value="30">30 мс (~33 Гц)</option>
                <option value="40">40 мс (~25 Гц)</option>
                <option value="50">50 мс (~20 Гц)</option>
                <option value="100">100 мс (~10 Гц)</option>
              </select>
              <button onclick="saveMinReadInterval()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 5px;">
              Влияние: Меньше значение = выше частота обновления веса, но больше нагрузка на CPU и I2C шину.
              Рекомендуется: 25 мс (40 Гц) для баланса производительности и стабильности.
            </small>
            <p id="minReadIntervalStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label for="nau7802WaitMsSelect">Таймаут ожидания готовности NAU7802 (мс):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <select id="nau7802WaitMsSelect" style="padding: 12px 15px; background: var(--bg-tertiary); border: 1px solid #404040; border-radius: 8px; font-size: 16px; color: var(--text-primary); flex: 0 0 auto;">
                <option value="5">5 мс</option>
                <option value="8">8 мс</option>
                <option value="10">10 мс</option>
                <option value="12" selected>12 мс - по умолчанию</option>
                <option value="15">15 мс</option>
                <option value="20">20 мс</option>
                <option value="30">30 мс</option>
                <option value="50">50 мс</option>
              </select>
              <button onclick="saveNAU7802WaitMs()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 5px;">
              Влияние: Меньше значение = быстрее отклик, но больше риск пропустить данные если NAU7802 не успел подготовить.
              Больше значение = надежнее чтение, но больше задержка в цикле. Рекомендуется: 12 мс.
            </small>
            <p id="nau7802WaitMsStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
          
          <div class="form-group" style="margin-bottom: 15px;">
            <label for="i2cSpeedSelect">Скорость шины I2C:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <select id="i2cSpeedSelect" style="padding: 12px 15px; background: var(--bg-tertiary); border: 1px solid #404040; border-radius: 8px; font-size: 16px; color: var(--text-primary); flex: 0 0 auto;">
                <option value="100000">100 kHz (стандартная) - по умолчанию</option>
                <option value="200000">200 kHz (средняя)</option>
                <option value="300000">300 kHz (высокая)</option>
                <option value="400000">400 kHz (быстрая)</option>
              </select>
              <button onclick="saveI2CSpeed()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 5px;">
              Влияние: Выше скорость = быстрее передача данных, но требует качественной разводки и может быть менее стабильной на длинных проводах.
              100 kHz = наиболее стабильная работа, рекомендуется для большинства случаев.
              400 kHz = максимальная скорость, но требует коротких проводов и хорошего экранирования.
              <strong>Внимание:</strong> Изменение скорости I2C применяется немедленно, но может потребовать перезагрузки для полной стабильности.
            </small>
            <p id="i2cSpeedStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
        </div>
        
        <!-- Кнопка перезапуска -->
        <div style="margin-top: 30px; text-align: center;">
          <button onclick="restartDevice()" style="background: var(--warning); width: 100%; padding: 15px; font-size: 18px; font-weight: bold;">
            🔄 Перезапустить устройство
          </button>
          <small style="color: var(--text-secondary); display: block; margin-top: 10px;">
            Перезапуск устройства для применения всех изменений настроек
          </small>
        </div>
      </div>
    </div>
    
    <!-- Вкладка Настройки двигателя -->
    <div id="motorSettings" class="tab-content">
      <div class="calibration-group">
        <h3>🔧 Настройки двигателя</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group">
            <label><strong>Абсолютное перемещение:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <span class="data-value" id="absoluteDisplacement" style="flex: 0 0 auto;">0.0</span> <span>мм</span>
              <button onclick="resetEncoderDisplacement()" class="reset-btn">🔄 Обнулить</button>
            </div>
          </div>
          <div class="form-group">
            <label for="encoderStepMm">Шаг в мм на импульс:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="encoderStepMm" min="0.0001" step="0.0001" placeholder="0.01">
              <button onclick="saveEncoderSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Пример: 0.01 мм = 10 мкм на импульс</small>
          </div>
          <div class="form-group">
            <label for="encoderMax">Максимальное значение:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="encoderMax" step="0.01" min="0" placeholder="2000.00">
              <span>мм</span>
              <button onclick="saveEncoderSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Верхний предел перемещения для управления двигателем (в миллиметрах)</small>
          </div>
          <div class="form-group" style="margin-top: 16px;">
            <label class="checkbox-label">
              <input type="checkbox" id="encoderTestOnlyB" name="encoderTestOnlyB">
              <span>Тестовый режим энкодера (только B)</span>
            </label>
            <button onclick="saveEncoderTestOnlyB()" style="margin-left: 12px;">💾 Сохранить</button>
            <span id="encoderTestOnlyBStatus" style="margin-left: 8px; font-size: 0.9em;"></span>
            <small style="display: block; color: var(--text-secondary); margin-top: 6px;">Вкл: только канал B, направление от кнопок мотора. Выкл: A+B, направление по энкодеру.</small>
          </div>
        </div>
        
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-top: 20px; border: 2px solid #ff6b00;">
          <h4 style="color: #ff6b00; margin-top: 0;">⚠️ Управление ограничениями перемещения</h4>
          <div class="form-group">
            <label><strong>Статус ограничений:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap; margin-bottom: 15px;">
              <span id="limitsStatus" class="data-value" style="flex: 0 0 auto;">Проверка...</span>
            </div>
          </div>
          <div class="form-group">
            <label for="limitsPinCode">Пин-код для отключения ограничений:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="limitsPinCode" placeholder="" style="width: 150px;">
              <button onclick="disableLimits()" class="reset-btn" style="background: #ff6b00;">🔓 Отключить ограничения</button>
              <button onclick="enableLimits()" class="reset-btn" style="background: #00aa00;">🔒 Включить ограничения</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 10px;">
              ⚠️ <strong>ВНИМАНИЕ:</strong> Отключение ограничений временное и не сохраняется при перезагрузке.<br>
              При отключенных ограничениях:<br>
              • Двигатель не блокируется по перемещению<br>
              • Запуск программ и калибровки невозможен<br>
              • На экранах отображается мигающее предупреждение
            </small>
            <p id="limitsStatusMessage" style="margin-top: 10px; font-size: 0.9em; color: var(--text-secondary);"></p>
          </div>
        </div>
        <div style="margin-top: 30px; text-align: center;">
          <button onclick="restartDevice()" style="background: var(--warning); width: 100%; padding: 15px; font-size: 18px; font-weight: bold;">
            🔄 Перезапустить устройство
          </button>
          <small style="color: var(--text-secondary); display: block; margin-top: 10px;">
            Перезапуск устройства для применения всех изменений настроек
          </small>
        </div>
      </div>
    </div>
    
    <!-- Вкладка Прочие настройки -->
    <div id="otherSettings" class="tab-content">
      <div class="calibration-group">
        <h3>📦 Информация о сборке</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <p style="margin: 0 0 8px 0;"><strong>Версия:</strong> <span id="buildRelease" class="data-value">-</span></p>
          <p style="margin: 0;"><strong>Дата сборки:</strong> <span id="buildDate" class="data-value">-</span></p>
        </div>
      </div>
      <div class="calibration-group">
        <h3>🎯 Настройки программы</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group">
            <label for="compressionStartThreshold">Порог начала накопления для сжатия (Н):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="compressionStartThreshold" step="0.1" min="0.1" placeholder="1.0">
              <button onclick="saveProgramSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Значение в Н, при достижении которого начинается накопление рабочего перемещения</small>
          </div>
          
          <div class="form-group">
            <label for="compressionTargetDisplacement">Целевое рабочее перемещение для сжатия (мм):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="compressionTargetDisplacement" step="0.01" min="0.01" placeholder="5.0">
              <button onclick="saveProgramSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Программа остановится при достижении этого рабочего перемещения</small>
          </div>
          
          <div class="form-group">
            <label for="compressionUnloadRetractMm">Расстояние после разгрузки (мм):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="compressionUnloadRetractMm" step="0.01" min="0" max="100" placeholder="5.0">
              <span>мм</span>
              <button onclick="saveProgramSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">После достижения целевого перемещения весы разгружаются до 0 Н, затем откат на это расстояние (не ниже нуля энкодера)</small>
          </div>
          
          <div class="form-group">
            <label for="breakDropThreshold">Порог падения для разрыва (%):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="breakDropThreshold" step="0.1" min="0.1" max="100.0" placeholder="20.0">
              <button onclick="saveProgramSettings()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Процент падения от максимума, при котором программа завершится</small>
          </div>
        </div>
      </div>
      
      <div class="calibration-group">
        <h3>🕐 Настройки NTP (синхронизация времени)</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group">
            <label for="ntpServer">NTP сервер:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="text" id="ntpServer" placeholder="pool.ntp.org" style="flex: 1; min-width: 200px;">
              <button onclick="saveNTPServer()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Сервер для синхронизации времени (например: pool.ntp.org, time.google.com)</small>
            <p id="ntpServerStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
          
          <div class="form-group">
            <label for="ntpInterval">Интервал обновления (секунды):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="ntpInterval" min="60" max="86400" step="60" placeholder="3600">
              <span>сек</span>
              <button onclick="saveNTPInterval()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Интервал синхронизации времени (60-86400 секунд, по умолчанию 3600 = 1 час)</small>
            <p id="ntpIntervalStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
          
          <div class="form-group">
            <label for="ntpTimezoneSelect">Часовой пояс:</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <select id="ntpTimezoneSelect" style="padding: 12px 15px; background: var(--bg-tertiary); border: 1px solid #404040; border-radius: 8px; font-size: 16px; color: var(--text-primary); flex: 1; min-width: 300px;">
                <option value="MSK-3">Москва, Россия (UTC+3)</option>
                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Киев, Украина (UTC+2/+3)</option>
                <option value="CET-1CEST,M3.5.0,M10.5.0/3">Берлин, Германия (UTC+1/+2)</option>
                <option value="GMT0BST,M3.5.0/1,M10.5.0">Лондон, Великобритания (UTC+0/+1)</option>
                <option value="EST5EDT,M3.2.0,M11.1.0">Нью-Йорк, США (UTC-5/-4)</option>
                <option value="CST6CDT,M3.2.0,M11.1.0">Чикаго, США (UTC-6/-5)</option>
                <option value="PST8PDT,M3.2.0,M11.1.0">Лос-Анджелес, США (UTC-8/-7)</option>
                <option value="JST-9">Токио, Япония (UTC+9)</option>
                <option value="CST-8">Пекин, Китай (UTC+8)</option>
                <option value="IST-5:30">Мумбаи, Индия (UTC+5:30)</option>
                <option value="MSK-3">Москва, Россия (UTC+3) - без перехода на летнее время</option>
                <option value="MSK-3MSK,M3.5.0,M10.5.0/3">Москва, Россия (UTC+3/+4) - с переходом на летнее время</option>
                <option value="GMT0">UTC (Гринвич)</option>
                <option value="EET-2">Восточная Европа (UTC+2) - без перехода</option>
                <option value="CET-1">Центральная Европа (UTC+1) - без перехода</option>
                <option value="WET0">Западная Европа (UTC+0) - без перехода</option>
              </select>
              <button onclick="saveNTPTimezone()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary); display: block; margin-top: 5px;">
              Выберите часовой пояс из списка. Время будет автоматически скорректировано после сохранения.
            </small>
            <p id="ntpTimezoneStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
          
          <div class="form-group">
            <label><strong>Статус синхронизации:</strong></label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <span class="data-value" id="ntpStatus" style="flex: 0 0 auto;">-</span>
            </div>
          </div>
        </div>
      </div>
      
      <div class="calibration-group">
        <h3>🌐 Настройки веб-сервера</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <div class="form-group">
            <label for="maxConnections">Максимальное количество одновременных подключений (1-10):</label>
            <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
              <input type="number" id="maxConnections" min="1" max="10" step="1" placeholder="1">
              <button onclick="saveMaxConnections()">💾 Сохранить</button>
            </div>
            <small style="color: var(--text-secondary);">Ограничение количества клиентов, которые могут одновременно управлять устройством</small>
            <p id="maxConnectionsStatus" style="margin-top: 5px; font-size: 0.9em;"></p>
          </div>
        </div>
      </div>

      <div class="calibration-group">
        <h3>📤 Обновление прошивки (OTA)</h3>
        <div style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <p style="color: var(--text-secondary); margin-bottom: 12px;">Загрузите файл <strong>.bin</strong> прошивки (Arduino IDE: Sketch &rarr; Export compiled Binary). Устройство перезагрузится после обновления.</p>
          <p><a href="/update" target="_blank" style="color: var(--accent); font-weight: 600;">Открыть страницу обновления прошивки &rarr;</a></p>
        </div>
      </div>
      <div style="margin-top: 30px; text-align: center;">
        <button onclick="restartDevice()" style="background: var(--warning); width: 100%; padding: 15px; font-size: 18px; font-weight: bold;">
          🔄 Перезапустить устройство
        </button>
        <small style="color: var(--text-secondary); display: block; margin-top: 10px;">
          Перезапуск устройства для применения всех изменений настроек
        </small>
      </div>
    </div>
    
    <!-- Вкладка История измерений -->
    <div id="history" class="tab-content">
      <div class="calibration-group">
        <h3>📋 История измерений</h3>
        <p style="color: var(--text-secondary); margin-bottom: 15px;">Последние результаты программ СЖАТИЕ и РАЗРЫВ.</p>
        <button onclick="updateHistory()" style="margin-bottom: 15px;">🔄 Обновить</button>
        <div id="historyList" style="background: var(--bg-secondary); padding: 15px; border-radius: 8px; min-height: 80px; overflow-x: auto;"></div>
      </div>
    </div>
  </div>

  <script>
    // Управление вкладками
    function setActiveTabUI(tabName) {
      // Скрыть все вкладки
      var tabs = document.querySelectorAll('.tab-content');
      for (var i = 0; i < tabs.length; i++) {
        tabs[i].classList.remove('active');
      }
      var buttons = document.querySelectorAll('.tab');
      for (var i = 0; i < buttons.length; i++) {
        buttons[i].classList.remove('active');
      }
      
      var contentElement = document.getElementById(tabName);
      if (contentElement) {
        contentElement.classList.add('active');
      }
      var buttonElement = document.querySelector('.tab[data-tab="' + tabName + '"]');
      if (buttonElement) {
        buttonElement.classList.add('active');
      }
    }
    
    var currentTab = 'compression';
    function showTab(tabName) {
      currentTab = tabName;
      setActiveTabUI(tabName);
      
      if (tabName === 'wifi') { updateWifiStatus(); updateTelegramForm(); }
      else if (tabName === 'motorSettings') { updateLimitsStatus(); updateMainData(); }
      else if (tabName === 'history') updateHistory();
      
      var screenNumber = 1;
      if (tabName === 'break') screenNumber = 2;
      else if (tabName === 'wifi') screenNumber = 3;
      else if (tabName === 'scaleSettings') screenNumber = 4;
      else if (tabName === 'motorSettings') screenNumber = 5;
      else if (tabName === 'otherSettings') screenNumber = 6;
      else if (tabName === 'history') screenNumber = 7;
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/display/screen', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.send(JSON.stringify({ screen: screenNumber }));
    }
    
    // Инициализация обработчиков закладок - максимально простая и надежная версия
    function setupTabs() {
      var allTabs = document.getElementsByClassName('tab');
      for (var i = 0; i < allTabs.length; i++) {
        var btn = allTabs[i];
        var tabName = btn.getAttribute('data-tab');
        if (!tabName) continue;
        
        // Сохраняем tabName в замыкании для избежания проблем с this
        (function(name) {
          btn.onclick = function() {
            showTab(name);
            return false;
          };
          btn.ontouchstart = function() {
            showTab(name);
            return false;
          };
          btn.ontouchend = function() {
            showTab(name);
            return false;
          };
        })(tabName);
      }
    }
    
    // Инициализируем обработчики - несколько попыток для надежности
    function initTabs() {
      setupTabs();
    }
    
    // Пробуем инициализировать сразу, если DOM готов
    if (document.body) {
      initTabs();
    }
    
    // Также инициализируем при событиях загрузки
    if (document.addEventListener) {
      document.addEventListener('DOMContentLoaded', initTabs, false);
      window.addEventListener('load', initTabs, false);
    } else if (document.attachEvent) {
      document.attachEvent('onreadystatechange', function() {
        if (document.readyState === 'complete') {
          initTabs();
        }
      });
      window.attachEvent('onload', initTabs);
    } else {
      window.onload = initTabs;
    }
    
    // Минимальное сглаживание для очень быстрого отклика
    var smoothWeight = 0;
    var smoothMaxWeightCompression = 0;
    var smoothMaxWeightBreak = 0;
    var smoothCalWeight = 0;
    var smoothingFactor = 0.95; // Очень высокий базовый коэффициент - почти мгновенный отклик
    
    var scaleRequestInFlight = false;
    function updateHeaderBar(data) {
      var timeEl = document.getElementById('headerTime');
      var uptimeEl = document.getElementById('headerUptime');
      if (!timeEl || !uptimeEl) return;
      if (data.currentTimeEpoch !== undefined && data.currentTimeEpoch > 0) {
        var d = new Date(data.currentTimeEpoch * 1000);
        var day = ('0' + d.getDate()).slice(-2);
        var month = ('0' + (d.getMonth() + 1)).slice(-2);
        var year = d.getFullYear();
        var h = ('0' + d.getHours()).slice(-2);
        var min = ('0' + d.getMinutes()).slice(-2);
        var sec = ('0' + d.getSeconds()).slice(-2);
        timeEl.textContent = day + '.' + month + '.' + year + ' ' + h + ':' + min + ':' + sec;
      } else {
        timeEl.textContent = '—';
      }
      if (data.uptimeMs !== undefined && data.uptimeMs >= 0) {
        var ms = parseInt(data.uptimeMs, 10) || 0;
        var s = Math.floor(ms / 1000);
        var m = Math.floor(s / 60);
        var h = Math.floor(m / 60);
        var d = Math.floor(h / 24);
        s = s % 60;
        m = m % 60;
        h = h % 24;
        var parts = [];
        if (d > 0) parts.push(d + '\u0434');
        if (h > 0 || parts.length) parts.push(h + '\u0447');
        parts.push(m + '\u043c');
        parts.push(s + '\u0441');
        uptimeEl.textContent = parts.join(' ');
      } else {
        uptimeEl.textContent = '—';
      }
    }
    setInterval(function() {
      var t = currentTab;
      if (t === 'compression' || t === 'break' || t === 'scaleSettings' || t === 'motorSettings') return;
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/api/scale', true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4 && xhr.status === 200) {
          try { updateHeaderBar(JSON.parse(xhr.responseText)); } catch(e) {}
        }
      };
      xhr.send();
    }, 1000);
    function updateMainData() {
      if (scaleRequestInFlight) return;
      var t = currentTab;
      if (t !== 'compression' && t !== 'break' && t !== 'scaleSettings' && t !== 'motorSettings') return;
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/api/scale', true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          scaleRequestInFlight = false;
        }
        if (xhr.readyState === 4 && xhr.status === 200) {
          var data = JSON.parse(xhr.responseText);
          updateHeaderBar(data);
          // Минимальное сглаживание для почти мгновенного отклика
          var targetWeight = data.currentWeight || 0;
          var weightDiff = Math.abs(targetWeight - smoothWeight);
          
          // Адаптивный коэффициент: почти мгновенный отклик
          var factor = weightDiff > 0.2 ? 1.0 : (weightDiff > 0.05 ? 0.98 : 0.95);
          smoothWeight += (targetWeight - smoothWeight) * factor;
          var currentN = parseFloat(targetWeight).toFixed(1);
          
          // Обновление экрана СЖАТИЕ
          var compressionMaxEl = document.getElementById('maxWeightCompression');
          if (compressionMaxEl) {
            if (data.compressionMax !== undefined) {
              smoothMaxWeightCompression = parseFloat(data.compressionMax);
            } else {
              var currentNval = parseFloat(currentN);
              if (currentNval > smoothMaxWeightCompression) {
                smoothMaxWeightCompression = currentNval;
              }
            }
            document.getElementById('currentWeightCompression').textContent = currentN;
            document.getElementById('maxWeightCompression').textContent = smoothMaxWeightCompression.toFixed(1);
            if (data.displacement !== undefined) {
              var absDisp = data.displacement.toFixed(1); // Абсолютное перемещение
              var workDisp = data.workingDisplacement !== undefined ? data.workingDisplacement.toFixed(1) : '0.0';
              var workEl = document.getElementById('displacementCompressionWork');
              var absEl = document.getElementById('displacementCompressionAbs');
              if (workEl) workEl.textContent = workDisp;
              if (absEl) absEl.textContent = absDisp;
              var calEl = document.getElementById('displacementCal');
              if (calEl) calEl.textContent = absDisp;
              // Обновляем абсолютное перемещение в настройках энкодера
              var absDispEl = document.getElementById('absoluteDisplacement');
              if (absDispEl) absDispEl.textContent = absDisp;
            }
          }
          
          // Обновление экрана РАЗРЫВ
          var breakMaxEl = document.getElementById('maxWeightBreak');
          if (breakMaxEl) {
            if (data.breakMax !== undefined) {
              smoothMaxWeightBreak = parseFloat(data.breakMax);
            } else {
              var currentNval = parseFloat(currentN);
              if (currentNval > smoothMaxWeightBreak) {
                smoothMaxWeightBreak = currentNval;
              }
            }
            document.getElementById('currentWeightBreak').textContent = currentN;
            document.getElementById('maxWeightBreak').textContent = smoothMaxWeightBreak.toFixed(1);
            if (data.displacement !== undefined) {
              var absDispB = data.displacement.toFixed(1);
              var workDispB = data.workingDisplacement !== undefined ? data.workingDisplacement.toFixed(1) : '0.0';
              var workElB = document.getElementById('displacementBreakWork');
              var absElB = document.getElementById('displacementBreakAbs');
              if (workElB) workElB.textContent = workDispB;
              if (absElB) absElB.textContent = absDispB;
            }
          }
          
          // Обновление RAW значений
          document.getElementById('currentRaw').textContent = data.rawValue || '0';
          
          // Нулевая точка: вес всегда 0, RAW — текущее усреднённое (как при калибровке)
          var zeroWeightInput = document.getElementById('zeroWeightNInput');
          var zeroRawInput = document.getElementById('zeroRawInput');
          if (zeroWeightInput) zeroWeightInput.value = '0';
          if (zeroRawInput && data.averagedRaw !== undefined) zeroRawInput.value = Math.round(data.averagedRaw);
          
          // Обновление полей ввода второй точки
          if (data.calibrationRaw !== undefined) {
            var calRawInput = document.getElementById('calibrationRawInput');
            if (calRawInput && calRawInput.value === '') {
              calRawInput.value = data.calibrationRaw;
            }
          }
          
          // Обновление информации о стабильности
          var noiseLevel = data.noiseLevel || 0;
          var noiseThreshold = data.noiseThreshold || 5.0;
          var isStable = data.isStable || false;
          document.getElementById('noiseLevel').textContent = noiseLevel.toFixed(2);
          var noiseThresholdInput = document.getElementById('noiseThresholdInput');
          if (noiseThresholdInput) {
            // Обновляем только если поле не в фокусе (пользователь не вводит данные)
            if (document.activeElement !== noiseThresholdInput) {
              var currentValue = parseFloat(noiseThresholdInput.value) || 0;
              var newValue = parseFloat(noiseThreshold.toFixed(2));
              // Обновляем только если значение изменилось
              if (Math.abs(currentValue - newValue) > 0.01) {
                noiseThresholdInput.value = noiseThreshold.toFixed(2);
              }
            }
          }
          
          var stabilityStatusEl = document.getElementById('stabilityStatus');
          if (stabilityStatusEl) {
            stabilityStatusEl.className = 'data-value';
            if (isStable) {
              stabilityStatusEl.textContent = 'стабильно';
              stabilityStatusEl.className += ' stability-stable';
            } else {
              stabilityStatusEl.textContent = 'не стабильно';
              stabilityStatusEl.className += ' stability-unstable';
            }
          }
          
          // Обновление значений калибровки (N)
          var currentNcal = parseFloat(targetWeight).toFixed(1);
          var currentWeightNEl = document.getElementById('currentWeightN');
          if (currentWeightNEl) {
            currentWeightNEl.value = currentNcal;
          }
          var currentRawEl = document.getElementById('currentRaw');
          if (currentRawEl && data.rawValue !== undefined) {
            currentRawEl.value = Math.round(data.rawValue);
          }
          
          // Обновляем поля второй точки только если они пустые И поле не в фокусе (пользователь не вводит данные)
          if (data.calibrationRaw && data.calibrationRaw !== 0 && data.calibrationFactor) {
            var calWeightInput = document.getElementById('calibrationWeightNInput');
            var calRawInput = document.getElementById('calibrationRawInput');
            // Обновляем только если поле пустое И не в фокусе (пользователь не вводит данные)
            if (calWeightInput && calWeightInput.value === '' && document.activeElement !== calWeightInput) {
              var zeroRawValue = data.zeroRaw || 0;
              var rawDiff = data.calibrationRaw - zeroRawValue;
              if (rawDiff !== 0) {
                // calibrationFactor переводит RAW напрямую в ньютоны
                var calWeightN = (rawDiff / data.calibrationFactor).toFixed(1);
                calWeightInput.value = calWeightN;
              }
            }
            // Обновляем RAW только если поле пустое И не в фокусе
            if (calRawInput && calRawInput.value === '' && document.activeElement !== calRawInput) {
              calRawInput.value = data.calibrationRaw;
            }
          }
          
          // Обновление настроек веб-сервера (только при первой загрузке)
          if (data.maxWebConnections !== undefined) {
            var maxConnInput = document.getElementById('maxConnections');
            if (maxConnInput && maxConnInput.value === '') {
              maxConnInput.value = data.maxWebConnections;
            }
          }
          
          // Обновление предела перегрузки (только если поле не в фокусе)
          if (data.maxWeight !== undefined) {
            var maxWeightInput = document.getElementById('maxWeightInput');
            if (maxWeightInput) {
              // Обновляем только если поле не в фокусе (пользователь не вводит данные)
              if (document.activeElement !== maxWeightInput) {
                var currentValue = parseFloat(maxWeightInput.value) || 0;
                var newValue = parseFloat(data.maxWeight.toFixed(1));
                // Обновляем только если значение изменилось или поле пустое
                if (maxWeightInput.value === '' || Math.abs(currentValue - newValue) > 0.1) {
                  maxWeightInput.value = data.maxWeight.toFixed(1);
                }
              }
            }
          }
          if (data.negativeWeightLimit !== undefined) {
            var negLimInput = document.getElementById('negativeWeightLimitInput');
            if (negLimInput && document.activeElement !== negLimInput) {
              var cur = parseFloat(negLimInput.value);
              var newVal = parseFloat(data.negativeWeightLimit);
              if (negLimInput.value === '' || (cur !== cur) || Math.abs(cur - newVal) > 0.1) {
                negLimInput.value = data.negativeWeightLimit.toFixed(0);
              }
            }
          }
          
          // Шаг энкодера (мм на импульс) — не перезаписываем при фокусе; подставляем при пустом или при неверном значении (например 200)
          if (data.encoderStepMm !== undefined) {
            var stepInput = document.getElementById('encoderStepMm');
            if (stepInput && document.activeElement !== stepInput) {
              var cur = parseFloat(stepInput.value);
              var newVal = parseFloat(data.encoderStepMm);
              var validRange = (newVal >= 0.0001 && newVal <= 10);
              if (validRange && (stepInput.value === '' || (cur !== cur) || cur < 0.0001 || cur > 10 || Math.abs(cur - newVal) > 0.0001)) {
                stepInput.value = data.encoderStepMm.toFixed(4);
              }
            }
          }
          // Максимальное значение перемещения — обновляем только если поле не в фокусе (чтобы не затирать ввод)
          if (data.encoderMax !== undefined) {
            var maxInput = document.getElementById('encoderMax');
            if (maxInput && document.activeElement !== maxInput) {
              var currentValue = parseFloat(maxInput.value) || 0;
              var newValue = parseFloat(data.encoderMax);
              if (maxInput.value === '' || Math.abs(currentValue - newValue) > 0.01) {
                maxInput.value = newValue.toFixed(2);
              }
            }
          }
          
          // Обновление статуса ограничений перемещения
          if (data.limitsDisabled !== undefined) {
            updateLimitsStatus();
          }
          
          // Обновление настроек программы (только при первой загрузке)
          if (data.compressionStartThreshold !== undefined) {
            var compThreshInput = document.getElementById('compressionStartThreshold');
            if (compThreshInput && compThreshInput.value === '') {
              compThreshInput.value = data.compressionStartThreshold;
            }
          }
          if (data.compressionTargetDisplacement !== undefined) {
            var compTargetInput = document.getElementById('compressionTargetDisplacement');
            if (compTargetInput && document.activeElement !== compTargetInput) {
              var cur = parseFloat(compTargetInput.value) || 0;
              var next = parseFloat(data.compressionTargetDisplacement);
              if (compTargetInput.value === '' || Math.abs(cur - next) > 0.01) {
                compTargetInput.value = data.compressionTargetDisplacement;
              }
            }
          }
          if (data.compressionUnloadRetractMm !== undefined) {
            var unloadRetractInput = document.getElementById('compressionUnloadRetractMm');
            if (unloadRetractInput && document.activeElement !== unloadRetractInput) {
              var cur = parseFloat(unloadRetractInput.value) || 0;
              var next = parseFloat(data.compressionUnloadRetractMm);
              if (unloadRetractInput.value === '' || Math.abs(cur - next) > 0.01) {
                unloadRetractInput.value = data.compressionUnloadRetractMm.toFixed(2);
              }
            }
          }
          if (data.breakDropThreshold !== undefined) {
            var breakThreshInput = document.getElementById('breakDropThreshold');
            if (breakThreshInput && breakThreshInput.value === '') {
              breakThreshInput.value = data.breakDropThreshold;
            }
          }
          
          // Обновление настроек NTP (только при первой загрузке)
          if (data.ntpServer !== undefined) {
            var ntpServerInput = document.getElementById('ntpServer');
            if (ntpServerInput && ntpServerInput.value === '') {
              ntpServerInput.value = data.ntpServer;
            }
          }
          if (data.ntpInterval !== undefined) {
            var ntpIntervalInput = document.getElementById('ntpInterval');
            if (ntpIntervalInput && ntpIntervalInput.value === '') {
              ntpIntervalInput.value = data.ntpInterval;
            }
          }
          if (data.ntpTimezone !== undefined) {
            var ntpTimezoneSelect = document.getElementById('ntpTimezoneSelect');
            if (ntpTimezoneSelect && document.activeElement !== ntpTimezoneSelect) {
              // Устанавливаем значение из списка, если оно совпадает с одним из option
              var found = false;
              for (var i = 0; i < ntpTimezoneSelect.options.length; i++) {
                if (ntpTimezoneSelect.options[i].value === data.ntpTimezone) {
                  ntpTimezoneSelect.selectedIndex = i;
                  found = true;
                  break;
                }
              }
              // Если точного совпадения нет, но есть похожее (например MSK-3), выбираем его
              if (!found && data.ntpTimezone.indexOf('MSK-3') >= 0) {
                for (var i = 0; i < ntpTimezoneSelect.options.length; i++) {
                  if (ntpTimezoneSelect.options[i].value.indexOf('MSK-3') >= 0) {
                    ntpTimezoneSelect.selectedIndex = i;
                    break;
                  }
                }
              }
            }
          }
          
          // Обновление настроек производительности (только если элемент не в фокусе И значение изменилось И не было только что сохранено)
          if (data.minReadInterval !== undefined) {
            var minIntervalSelect = document.getElementById('minReadIntervalSelect');
            if (minIntervalSelect && document.activeElement !== minIntervalSelect && !justSavedMinInterval) {
              // Обновляем только если значение на сервере отличается от текущего выбранного
              var currentValue = parseInt(minIntervalSelect.value) || 0;
              if (currentValue === 0 || currentValue !== data.minReadInterval) {
                minIntervalSelect.value = data.minReadInterval;
              }
            }
          }
          if (data.nau7802WaitMs !== undefined) {
            var waitMsSelect = document.getElementById('nau7802WaitMsSelect');
            if (waitMsSelect && document.activeElement !== waitMsSelect && !justSavedWaitMs) {
              var currentValue = parseInt(waitMsSelect.value) || 0;
              if (currentValue === 0 || currentValue !== data.nau7802WaitMs) {
                waitMsSelect.value = data.nau7802WaitMs;
              }
            }
          }
          if (data.i2cSpeed !== undefined) {
            var i2cSpeedSelect = document.getElementById('i2cSpeedSelect');
            if (i2cSpeedSelect && document.activeElement !== i2cSpeedSelect && !justSavedI2CSpeed) {
              var currentValue = parseInt(i2cSpeedSelect.value) || 0;
              if (currentValue === 0 || currentValue !== data.i2cSpeed) {
                i2cSpeedSelect.value = data.i2cSpeed;
              }
            }
          }
          
          // Обновление статуса NTP
          var ntpStatusEl = document.getElementById('ntpStatus');
          if (ntpStatusEl && data.ntpSynced !== undefined) {
            if (data.ntpSynced) {
              ntpStatusEl.textContent = '✅ Синхронизировано';
              ntpStatusEl.className = 'data-value stability-stable';
            } else {
              ntpStatusEl.textContent = '❌ Не синхронизировано';
              ntpStatusEl.className = 'data-value stability-unstable';
            }
          }
          
          // Обновление информации о сборке (вкладка Прочие)
          var buildReleaseEl = document.getElementById('buildRelease');
          var buildDateEl = document.getElementById('buildDate');
          if (buildReleaseEl && data.release !== undefined) buildReleaseEl.textContent = data.release;
          if (buildDateEl && data.releaseDate !== undefined) buildDateEl.textContent = data.releaseDate;
          
          // Обновление статуса программы
          var programRunning = data.programRunning || false;
          var programCompleted = data.programCompleted || false;
          var programStatusMessage = data.programStatusMessage || '';
          
          // Обновление статуса на экране СЖАТИЕ
          var statusElCompression = document.getElementById('programStatusCompression');
          if (statusElCompression) {
            if (programCompleted) {
              statusElCompression.textContent = '✅ Программа завершена!';
              statusElCompression.style.color = '#28a745';
            } else if (programRunning) {
              // Мигание "ВНИМАНИЕ!!!" если статус Waiting
              var isWaiting = programStatusMessage.startsWith('Waiting');
              if (isWaiting) {
                var blink = (Math.floor(Date.now() / 500) % 2 === 0);
                statusElCompression.textContent = blink ? '⚠️ ВНИМАНИЕ!!!' : '';
                statusElCompression.style.color = '#ffc107';
              } else {
                statusElCompression.textContent = programStatusMessage || 'Выполняется...';
                statusElCompression.style.color = '#17a2b8';
              }
            } else {
              statusElCompression.textContent = 'Готово';
              statusElCompression.style.color = 'var(--text-secondary)';
            }
          }
          
          // Обновление статуса на экране РАЗРЫВ
          var statusElBreak = document.getElementById('programStatusBreak');
          if (statusElBreak) {
            if (programCompleted) {
              statusElBreak.textContent = '✅ Программа завершена!';
              statusElBreak.style.color = '#28a745';
            } else if (programRunning) {
              var isWaitingB = programStatusMessage.startsWith('Waiting');
              if (isWaitingB) {
                var blink = (Math.floor(Date.now() / 500) % 2 === 0);
                statusElBreak.textContent = blink ? '⚠️ ВНИМАНИЕ!!!' : '';
                statusElBreak.style.color = '#ffc107';
              } else {
                statusElBreak.textContent = programStatusMessage || 'Выполняется...';
                statusElBreak.style.color = '#17a2b8';
              }
            } else {
              statusElBreak.textContent = 'Готово';
              statusElBreak.style.color = 'var(--text-secondary)';
            }
          }
          
          // Блокировка/разблокировка кнопок запуска
          var startCompressionBtn = document.getElementById('startCompressionBtn');
          var startBreakBtn = document.getElementById('startBreakBtn');
          if (startCompressionBtn) {
            startCompressionBtn.disabled = programRunning;
            if (programRunning) {
              startCompressionBtn.style.opacity = '0.5';
              startCompressionBtn.style.cursor = 'not-allowed';
              startCompressionBtn.style.backgroundColor = '#6c757d';
            } else {
              startCompressionBtn.style.opacity = '1';
              startCompressionBtn.style.cursor = 'pointer';
              startCompressionBtn.style.backgroundColor = 'var(--success)';
            }
          }
          if (startBreakBtn) {
            startBreakBtn.disabled = programRunning;
            if (programRunning) {
              startBreakBtn.style.opacity = '0.5';
              startBreakBtn.style.cursor = 'not-allowed';
              startBreakBtn.style.backgroundColor = '#6c757d';
            } else {
              startBreakBtn.style.opacity = '1';
              startBreakBtn.style.cursor = 'pointer';
              startBreakBtn.style.backgroundColor = 'var(--danger)';
            }
          }
          
          // Синхронизация вкладок с текущим экраном TFT
          if (data.currentScreen !== undefined) {
            var targetTab = 'compression';
            if (data.currentScreen == 2) targetTab = 'break';
            else if (data.currentScreen == 3) targetTab = 'wifi';
            else if (data.currentScreen == 4) targetTab = 'scaleSettings';
            else if (data.currentScreen == 5) targetTab = 'motorSettings';
            else if (data.currentScreen == 6) targetTab = 'otherSettings';
            else if (data.currentScreen == 7) targetTab = 'history';
            var activeContent = document.querySelector('.tab-content.active');
            var activeId = activeContent ? activeContent.id : '';
            if (activeId !== targetTab) {
              setActiveTabUI(targetTab); // Не шлём команду на TFT, только меняем вкладку у клиента
            }
          }
          
        }
      };
      scaleRequestInFlight = true;
      xhr.send();
    }
    
    // Инициализация: загружаем параметры энкодера при первой загрузке
    updateMainData();
    
    // Функции сброса максимумов
    function resetCompressionMax() {
      fetch('/api/display/reset/main', { method: 'POST' })
        .then(function() {
          smoothMaxWeightCompression = 0;
          document.getElementById('maxWeightCompression').textContent = '0.0';
          // Обновляем данные для получения нового максимума с сервера
          updateMainData();
        });
    }
    
    function resetBreakMax() {
      fetch('/api/display/reset/break', { method: 'POST' })
        .then(function() {
          smoothMaxWeightBreak = 0;
          document.getElementById('maxWeightBreak').textContent = '0.0';
          // Обновляем данные для получения нового максимума с сервера
          updateMainData();
        });
    }
    
    function resetCounter() {
      fetch('/api/counter/reset', { method: 'POST' });
      document.getElementById('opticalCount').textContent = '0';
      var d1w = document.getElementById('displacementCompressionWork');
      var d1a = document.getElementById('displacementCompressionAbs');
      var d2w = document.getElementById('displacementBreakWork');
      var d2a = document.getElementById('displacementBreakAbs');
      var d3 = document.getElementById('displacementCal');
      var d4 = document.getElementById('absoluteDisplacement');
      if (d1w) d1w.textContent = '0.0';
      if (d1a) d1a.textContent = '0.0';
      if (d2w) d2w.textContent = '0.0';
      if (d2a) d2a.textContent = '0.0';
      if (d3) d3.textContent = '0.0';
      if (d4) d4.textContent = '0.0';
      updateMainData();
    }
    
    function resetEncoderDisplacement() {
      resetCounter();
    }
    
    // Пошаговая калибровка
    function startCalibration() {
      // Сбрасываем все статусы
      document.getElementById('zeroStatus').textContent = '';
      document.getElementById('calStatus').textContent = '';
      document.getElementById('zeroBtn').disabled = false;
      document.getElementById('calBtn').disabled = false;
      document.getElementById('calibrationWeight').value = '';
      
      fetch('/api/calibration/start', { method: 'POST' })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          document.getElementById('calibrationSteps').style.display = 'block';
          document.getElementById('startCalBtn').style.display = 'none';
          document.getElementById('zeroBtn').disabled = false;
          document.getElementById('zeroStatus').textContent = '';
          document.getElementById('weightInputGroup').style.display = 'none';
          // Сбрасываем отображение второй точки
            document.getElementById('calibrationWeightNInput').value = '';
            document.getElementById('calibrationRawInput').value = '';
          alert('Пожалуйста, разгрузите платформу и нажмите "Записать ноль"');
        })
        .catch(function(error) {
          console.error('Error starting calibration:', error);
          alert('Ошибка при запуске калибровки');
        });
    }
    
    function recordZero() {
      var btn = document.getElementById('zeroBtn');
      var status = document.getElementById('zeroStatus');
      btn.disabled = true;
      status.textContent = '⏳ Запись...';
      
      fetch('/api/calibration/zero', { method: 'POST' })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          if (data.status === 'success') {
            status.textContent = '✅ Записано: ' + data.zeroRaw;
            document.getElementById('weightInputGroup').style.display = 'block';
            document.getElementById('zeroRawInput').value = data.zeroRaw;
          } else {
            status.textContent = '❌ Ошибка: ' + (data.message || 'Нестабильное значение');
            btn.disabled = false;
          }
        })
        .catch(function(error) {
          status.textContent = '❌ Ошибка соединения';
          btn.disabled = false;
        });
    }
    
    function recordCalibrationPoint() {
      var weight = parseFloat(document.getElementById('calibrationWeight').value);
      if (!weight || weight <= 0) {
        alert('Пожалуйста, укажите калибровочный вес в ньютонах');
        return;
      }
      
      var btn = document.getElementById('calBtn');
      var status = document.getElementById('calStatus');
      btn.disabled = true;
      status.textContent = '⏳ Запись...';
      
      fetch('/api/calibration/point', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ weight: weight })
      })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          if (data.status === 'success') {
            status.textContent = '✅ Калибровка завершена!';
            
            // Обновляем поля ввода второй точки
            document.getElementById('calibrationWeightNInput').value = document.getElementById('calibrationWeight').value;
            document.getElementById('calibrationRawInput').value = data.calibrationRaw;
            
            // Обновляем поле ввода нулевой точки
            document.getElementById('zeroRawInput').value = data.zeroRaw;
            
            document.getElementById('calibrationSteps').style.display = 'none';
            document.getElementById('startCalBtn').style.display = 'block';
            alert('Калибровка успешно завершена!\n\n' +
                  'RAW при 0 Н: ' + data.zeroRaw + '\n' +
                  'RAW по второй точке: ' + data.calibrationRaw + '\n' +
                  'Вес второй точки: ' + document.getElementById('calibrationWeight').value + ' Н\n' +
                  'Коэффициент: ' + data.calibrationFactor.toFixed(2));
          } else {
            status.textContent = '❌ Ошибка: ' + (data.message || 'Калибровка не удалась');
            btn.disabled = false;
          }
        })
        .catch(function(error) {
          status.textContent = '❌ Ошибка соединения';
          btn.disabled = false;
        });
    }
    
    function setNoiseThreshold() {
      var threshold = parseFloat(document.getElementById('noiseThresholdInput').value);
      if (!threshold || threshold < 0.1 || threshold > 50.0) {
        alert('Пожалуйста, укажите порог шума от 0.1 до 50.0%');
        return;
      }
      
      var status = document.getElementById('noiseThresholdStatus');
      status.textContent = '⏳ Сохранение...';
      status.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/calibration/noise-threshold', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            try {
              var data = JSON.parse(xhr.responseText);
              if (data.status === 'ok') {
                status.textContent = '✅ Сохранено: ' + data.threshold + '%';
                status.style.color = '#28a745';
                var noiseThresholdEl = document.getElementById('noiseThreshold');
                if (noiseThresholdEl) {
                  noiseThresholdEl.textContent = data.threshold.toFixed(2);
                }
                document.getElementById('noiseThresholdInput').value = '';
                setTimeout(function() {
                  status.textContent = '';
                }, 3000);
              } else {
                var errorMsg = data.message || 'Не удалось сохранить';
                status.textContent = '❌ Ошибка: ' + errorMsg;
                status.style.color = '#dc3545';
              }
            } catch (e) {
              status.textContent = '❌ Ошибка парсинга ответа';
              status.style.color = '#dc3545';
            }
          } else {
            status.textContent = '❌ Ошибка сервера: ' + xhr.status;
            status.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ threshold: threshold }));
    }
    
    // Старая функция калибровки (для совместимости)
    function calibrateScale() {
      startCalibration();
    }
    
    function setMaxWeight() {
      var maxWeight = parseFloat(document.getElementById('maxWeightInput').value);
      fetch('/api/scale/max', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ weight: maxWeight })
      }).then(function() { alert('Предел перегрузки установлен!'); });
    }
    function setNegativeWeightLimit() {
      var el = document.getElementById('negativeWeightLimitInput');
      var limit = el ? parseFloat(el.value) : -50;
      if (limit > 0) limit = -limit;
      fetch('/api/scale/negative-weight-limit', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ limit: limit })
      }).then(function() { alert('Порог отрицательного веса: ' + limit + ' Н'); });
    }
    
    function saveZeroPoint() {
      var weightN = parseFloat(document.getElementById('zeroWeightNInput').value);
      var rawValue = parseInt(document.getElementById('zeroRawInput').value);
      
      if (isNaN(weightN) || isNaN(rawValue)) {
        alert('Пожалуйста, заполните оба поля');
        return;
      }
      
      var statusEl = document.getElementById('zeroPointStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/calibration/set-zero', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Обнулено';
            statusEl.style.color = '#28a745';
            setTimeout(function() { statusEl.textContent = ''; }, 3000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ weight: weightN, raw: rawValue }));
    }
    
    function saveCalibrationPoint() {
      var weightN = parseFloat(document.getElementById('calibrationWeightNInput').value);
      var rawValue = parseInt(document.getElementById('calibrationRawInput').value);
      
      if (isNaN(weightN) || isNaN(rawValue)) {
        alert('Пожалуйста, заполните оба поля');
        return;
      }
      
      var statusEl = document.getElementById('calibrationPointStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/calibration/set-point', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено';
            statusEl.style.color = '#28a745';
            setTimeout(function() { statusEl.textContent = ''; }, 3000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ weight: weightN, raw: rawValue }));
    }
    
    function saveMaxConnections() {
      var maxConn = parseInt(document.getElementById('maxConnections').value);
      if (!maxConn || maxConn < 1 || maxConn > 10) {
        alert('Введите корректное значение (1-10)!');
        return;
      }
      
      var statusEl = document.getElementById('maxConnectionsStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/web/max-connections', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: максимум ' + data.maxConnections + ' подключений';
            statusEl.style.color = '#28a745';
            setTimeout(function() {
              statusEl.textContent = '';
            }, 3000);
          } else {
            statusEl.textContent = '❌ Ошибка сохранения';
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ maxConnections: maxConn }));
    }
    
    function startMotor(direction) {
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/motor/' + direction, true);
      xhr.send();
    }
    
    function stopMotor() {
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/motor/stop', true);
      xhr.send();
    }

    function stopProgramAndMotor() {
      // Останавливаем двигатель
      stopMotor();
      // Останавливаем программу (сжатие/разрыв)
      fetch('/api/program/stop', { method: 'POST' });
    }
    
    function startCompressionProgram() {
      fetch('/api/program/start/compression', { method: 'POST' })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          if (data.status === 'started') {
            console.log('Compression program started');
          } else {
            alert('Ошибка запуска программы: ' + (data.message || 'Unknown error'));
          }
        })
        .catch(function(error) {
          console.error('Error starting compression program:', error);
          alert('Ошибка запуска программы');
        });
    }
    
    function startBreakProgram() {
      fetch('/api/program/start/break', { method: 'POST' })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          if (data.status === 'started') {
            console.log('Break program started');
          } else {
            alert('Ошибка запуска программы: ' + (data.message || 'Unknown error'));
          }
        })
        .catch(function(error) {
          console.error('Error starting break program:', error);
          alert('Ошибка запуска программы');
        });
    }
    
    
    function saveEncoderSettings() {
      var stepMm = parseFloat(document.getElementById('encoderStepMm').value);
      var maxValueMm = parseFloat(document.getElementById('encoderMax').value);
      
      if (!stepMm || stepMm <= 0 || stepMm > 100.0) {
        alert('Введите корректный шаг в мм (0.0001-100.0)!');
        return;
      }
      
      if (!maxValueMm || maxValueMm <= 0) {
        alert('Введите корректное максимальное значение в мм!');
        return;
      }
      
      Promise.all([
        fetch('/api/encoder/step', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ stepMm: stepMm })
        }),
        fetch('/api/encoder/max', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ max: maxValueMm })
        })
      ]).then(function() { alert('Настройки энкодера сохранены!'); });
    }
    
    function saveEncoderTestOnlyB() {
      var cb = document.getElementById('encoderTestOnlyB');
      var st = document.getElementById('encoderTestOnlyBStatus');
      if (!cb || !st) return;
      st.textContent = '';
      fetch('/api/encoder/test-only-b', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ testOnlyB: cb.checked })
      })
      .then(function(r) { return r.json(); })
      .then(function(data) {
        if (data.status === 'ok') {
          st.textContent = 'Сохранено';
          st.style.color = 'var(--success)';
        } else {
          st.textContent = data.message || 'Ошибка';
          st.style.color = 'var(--danger)';
        }
      })
      .catch(function() {
        st.textContent = 'Ошибка запроса';
        st.style.color = 'var(--danger)';
      });
    }
    
    function disableLimits() {
      var pinCode = parseInt(document.getElementById('limitsPinCode').value);
      if (!pinCode || pinCode <= 0) {
        alert('Введите пин-код!');
        return;
      }
      
      fetch('/api/motor/limits/disable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pinCode: pinCode })
      })
      .then(function(response) { return response.json(); })
      .then(function(data) {
        if (data.status === 'ok') {
          document.getElementById('limitsStatusMessage').textContent = '✅ Ограничения отключены (временно, не сохраняется)';
          document.getElementById('limitsStatusMessage').style.color = '#ff6b00';
          updateLimitsStatus();
        } else {
          alert('Ошибка: ' + (data.message || 'Неверный пин-код'));
        }
      })
      .catch(function(error) {
        alert('Ошибка при отключении ограничений: ' + error);
      });
    }
    
    function enableLimits() {
      fetch('/api/motor/limits/enable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
      })
      .then(function(response) { return response.json(); })
      .then(function(data) {
        if (data.status === 'ok') {
          document.getElementById('limitsStatusMessage').textContent = '✅ Ограничения включены';
          document.getElementById('limitsStatusMessage').style.color = '#00aa00';
          updateLimitsStatus();
        } else {
          alert('Ошибка: ' + (data.message || 'Не удалось включить ограничения'));
        }
      })
      .catch(function(error) {
        alert('Ошибка при включении ограничений: ' + error);
      });
    }
    
    function updateLimitsStatus() {
      if (currentTab !== 'motorSettings') return;
      fetch('/api/motor/limits/status')
        .then(function(response) { return response.json(); })
        .then(function(data) {
          var statusEl = document.getElementById('limitsStatus');
          if (statusEl) {
            if (data.limitsDisabled) {
              statusEl.textContent = '🔓 ОТКЛЮЧЕНЫ';
              statusEl.style.color = '#ff6b00';
            } else {
              statusEl.textContent = '🔒 ВКЛЮЧЕНЫ';
              statusEl.style.color = '#00aa00';
            }
          }
        })
        .catch(function(error) {
          console.error('Ошибка при получении статуса ограничений:', error);
        });
    }
    
    function updateHistory() {
      var el = document.getElementById('historyList');
      if (!el) return;
      el.innerHTML = '<span style="color: var(--text-secondary);">Загрузка...</span>';
      fetch('/api/measurements')
        .then(function(response) { return response.json(); })
        .then(function(arr) {
          if (!Array.isArray(arr) || arr.length === 0) {
            el.innerHTML = '<span style="color: var(--text-secondary);">Нет записей</span>';
            return;
          }
          var typeStr = function(t) { return t === 1 ? 'СЖАТИЕ' : (t === 2 ? 'РАЗРЫВ' : '-'); };
          var outcomeStr = function(o) {
            if (o === 0) return '<span style="color: var(--success);">Выполнено</span>';
            if (o === 1) return '<span style="color: var(--warning);">Остановлено</span>';
            return '<span style="color: var(--danger);">Ошибка</span>';
          };
          var html = '<table style="width:100%; border-collapse: collapse; color: var(--text-primary); font-size: 14px;">';
          html += '<tr style="border-bottom: 1px solid #404040;"><th style="text-align:left; padding:8px;">Дата</th><th style="text-align:left; padding:8px;">Тип</th><th style="text-align:left; padding:8px;">Результат</th><th style="text-align:right; padding:8px;">Вес, Н</th></tr>';
          for (var i = 0; i < arr.length; i++) {
            var r = arr[i];
            var d = r.ts ? new Date(r.ts * 1000).toLocaleString('ru-RU', { dateStyle: 'short', timeStyle: 'short' }) : '—';
            html += '<tr style="border-bottom: 1px solid #404040;">';
            html += '<td style="padding:8px;">' + d + '</td>';
            html += '<td style="padding:8px;">' + typeStr(r.type) + '</td>';
            html += '<td style="padding:8px;">' + outcomeStr(r.outcome) + '</td>';
            html += '<td style="padding:8px; text-align:right;">' + (r.weight != null ? parseFloat(r.weight).toFixed(1) : '—') + '</td>';
            html += '</tr>';
          }
          html += '</table>';
          el.innerHTML = html;
        })
        .catch(function(err) {
          el.innerHTML = '<span style="color: var(--danger);">Ошибка загрузки</span>';
          console.error('History fetch error:', err);
        });
    }
    
    function saveProgramSettings() {
      var compThreshold = parseFloat(document.getElementById('compressionStartThreshold').value);
      var compTarget = parseFloat(document.getElementById('compressionTargetDisplacement').value);
      var unloadRetract = parseFloat(document.getElementById('compressionUnloadRetractMm').value);
      var breakThreshold = parseFloat(document.getElementById('breakDropThreshold').value);
      
      if (!compThreshold || compThreshold <= 0 || compThreshold > 1000.0) {
        alert('Введите корректный порог начала накопления (0.1-1000.0 Н)!');
        return;
      }
      if (!compTarget || compTarget <= 0 || compTarget > 1000.0) {
        alert('Введите корректное целевое перемещение (0.01-1000.0 мм)!');
        return;
      }
      if (isNaN(unloadRetract) || unloadRetract < 0 || unloadRetract > 100.0) {
        alert('Введите корректное расстояние после разгрузки (0-100.0 мм)!');
        return;
      }
      if (!breakThreshold || breakThreshold <= 0 || breakThreshold > 100.0) {
        alert('Введите корректный порог падения (0.1-100.0%)!');
        return;
      }
      
      Promise.all([
        fetch('/api/program/compression-threshold', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ threshold: compThreshold })
        }),
        fetch('/api/program/compression-target', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ target: compTarget })
        }),
        fetch('/api/program/compression-unload-retract', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mm: unloadRetract })
        }),
        fetch('/api/program/break-drop-threshold', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ threshold: breakThreshold })
        })
      ]).then(function() { alert('Настройки программы сохранены!'); });
    }
    
    // NTP функции
    function saveNTPServer() {
      var server = document.getElementById('ntpServer').value.trim();
      if (!server || server.length === 0 || server.length > 63) {
        alert('Введите корректное имя сервера (1-63 символа)!');
        return;
      }
      
      var statusEl = document.getElementById('ntpServerStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/ntp/server', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: ' + data.ntpServer;
            statusEl.style.color = '#28a745';
            setTimeout(function() { statusEl.textContent = ''; }, 3000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ server: server }));
    }
    
    function saveNTPInterval() {
      var interval = parseInt(document.getElementById('ntpInterval').value);
      if (!interval || interval < 60 || interval > 86400) {
        alert('Введите корректный интервал (60-86400 секунд)!');
        return;
      }
      
      var statusEl = document.getElementById('ntpIntervalStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/ntp/interval', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: ' + data.ntpInterval + ' секунд';
            statusEl.style.color = '#28a745';
            setTimeout(function() { statusEl.textContent = ''; }, 3000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ interval: interval }));
    }
    
    function saveNTPTimezone() {
      var selectEl = document.getElementById('ntpTimezoneSelect');
      if (!selectEl) {
        alert('Элемент выбора часового пояса не найден!');
        return;
      }
      var tz = selectEl.value.trim();
      if (!tz || tz.length === 0 || tz.length >= 64) {
        alert('Выберите корректный часовой пояс из списка!');
        return;
      }
      
      var statusEl = document.getElementById('ntpTimezoneStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/ntp/timezone', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: ' + data.ntpTimezone;
            statusEl.style.color = '#28a745';
            setTimeout(function() { statusEl.textContent = ''; }, 3000);
          } else {
            try {
              var error = JSON.parse(xhr.responseText);
              statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            } catch (e) {
              statusEl.textContent = '❌ Ошибка сохранения';
            }
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send(JSON.stringify({ timezone: tz }));
    }
    
    // Флаги для предотвращения перезаписи только что сохраненных значений
    var justSavedMinInterval = false;
    var justSavedWaitMs = false;
    var justSavedI2CSpeed = false;
    var saveTimestamp = 0;
    
    // Функции для настроек производительности чтения тензодатчика
    function saveMinReadInterval() {
      var selectEl = document.getElementById('minReadIntervalSelect');
      var interval = parseInt(selectEl.value, 10);
      if (isNaN(interval) || interval < 10 || interval > 100) {
        alert('Выберите корректный интервал (10-100 мс)! Выбрано: ' + selectEl.value);
        return;
      }
      
      var statusEl = document.getElementById('minReadIntervalStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/scale/performance/min-interval?interval=' + encodeURIComponent(interval), true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: ' + data.minReadInterval + ' мс (~' + Math.round(1000 / data.minReadInterval) + ' Гц)';
            statusEl.style.color = '#28a745';
            // Устанавливаем флаг, чтобы не перезаписывать значение в течение 5 секунд
            justSavedMinInterval = true;
            saveTimestamp = Date.now();
            setTimeout(function() { 
              statusEl.textContent = '';
              justSavedMinInterval = false;
            }, 5000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send();
    }
    
    function saveNAU7802WaitMs() {
      var selectEl = document.getElementById('nau7802WaitMsSelect');
      var waitMs = parseInt(selectEl.value, 10);
      if (isNaN(waitMs) || waitMs < 5 || waitMs > 50) {
        alert('Выберите корректный таймаут (5-50 мс)! Выбрано: ' + selectEl.value);
        return;
      }
      
      var statusEl = document.getElementById('nau7802WaitMsStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/scale/performance/wait-ms?waitMs=' + encodeURIComponent(waitMs), true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            statusEl.textContent = '✅ Сохранено: ' + data.nau7802WaitMs + ' мс';
            statusEl.style.color = '#28a745';
            // Устанавливаем флаг, чтобы не перезаписывать значение в течение 5 секунд
            justSavedWaitMs = true;
            saveTimestamp = Date.now();
            setTimeout(function() { 
              statusEl.textContent = '';
              justSavedWaitMs = false;
            }, 5000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send();
    }
    
    function saveI2CSpeed() {
      var selectEl = document.getElementById('i2cSpeedSelect');
      var speed = parseInt(selectEl.value, 10);
      if (isNaN(speed) || (speed !== 100000 && speed !== 200000 && speed !== 300000 && speed !== 400000)) {
        alert('Выберите корректную скорость (100000, 200000, 300000 или 400000 Гц)! Выбрано: ' + selectEl.value);
        return;
      }
      
      var statusEl = document.getElementById('i2cSpeedStatus');
      statusEl.textContent = '⏳ Сохранение...';
      statusEl.style.color = '#b0b0b0';
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/scale/performance/i2c-speed?speed=' + encodeURIComponent(speed), true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            var data = JSON.parse(xhr.responseText);
            var speedLabels = {
              100000: '100 kHz (стандартная)',
              200000: '200 kHz (средняя)',
              300000: '300 kHz (высокая)',
              400000: '400 kHz (быстрая)'
            };
            var speedLabel = speedLabels[data.i2cSpeed] || data.i2cSpeed + ' Гц';
            statusEl.textContent = '✅ Сохранено: ' + speedLabel + '. Изменение применено немедленно.';
            statusEl.style.color = '#28a745';
            // Устанавливаем флаг, чтобы не перезаписывать значение в течение 5 секунд
            justSavedI2CSpeed = true;
            saveTimestamp = Date.now();
            setTimeout(function() { 
              statusEl.textContent = '';
              justSavedI2CSpeed = false;
            }, 5000);
          } else {
            var error = JSON.parse(xhr.responseText);
            statusEl.textContent = '❌ ' + (error.message || 'Ошибка');
            statusEl.style.color = '#dc3545';
          }
        }
      };
      xhr.send();
    }
    
    // Функция перезапуска устройства
    function restartDevice() {
      if (!confirm('Вы уверены, что хотите перезапустить устройство?')) {
        return;
      }
      
      fetch('/api/restart', { method: 'POST' })
        .then(function(response) { return response.json(); })
        .then(function(data) {
          alert('Устройство перезапускается...');
          // Страница автоматически перезагрузится после перезапуска устройства
        })
        .catch(function(error) {
          console.error('Ошибка перезапуска:', error);
          alert('Ошибка при перезапуске устройства');
        });
    }
    
    // WiFi функции (опрос только на вкладке WiFi)
    function updateWifiStatus() {
      if (currentTab !== 'wifi') return;
      fetch('/status')
        .then(function(response) { return response.json(); })
        .then(function(data) {
          var statusDiv = document.getElementById('wifiStatus');
          var statusText = document.getElementById('wifiStatusText');
          var infoPanel = document.getElementById('wifiInfoPanel');
          
          if (data.connected) {
            statusDiv.className = 'status connected';
            infoPanel.style.display = 'block';
            document.getElementById('currentSSID').textContent = data.ssid;
            document.getElementById('ipAddress').textContent = data.ip;
            document.getElementById('rssi').textContent = data.rssi;
            statusText.textContent = 'Подключено к ' + data.ssid;
          } else {
            statusDiv.className = 'status disconnected';
            statusText.textContent = 'Режим точки доступа: ' + data.ap_name;
            infoPanel.style.display = 'none';
          }
        });
    }
    
    function updateTelegramForm() {
      fetch('/api/telegram')
        .then(function(response) { return response.json(); })
        .then(function(data) {
          var cb = function(id, v) { var e = document.getElementById(id); if (e) e.checked = !!v; };
          cb('tgEnabled', data.enabled);
          cb('tgNotifyResults', data.notifyProgramResults);
          cb('tgNotifyStartup', data.notifyStartup);
          cb('tgNotifyOverload', data.notifyOverload);
          cb('tgNotifyStopped', data.notifyStopped);
          var tok = document.getElementById('tgToken');
          if (tok) tok.value = data.token || '';
          var cid = document.getElementById('tgChatId');
          if (cid) cid.value = data.chatId || '';
        });
    }
    function updateMotorEncoderForm() {
      fetch('/api/scale')
        .then(function(response) { return response.json(); })
        .then(function(data) {
          var e = document.getElementById('encoderTestOnlyB');
          if (e && data.encoderTestOnlyB !== undefined) e.checked = !!data.encoderTestOnlyB;
        });
    }
    
    function saveTelegramSettings() {
      var btn = document.getElementById('tgSaveBtn');
      var st = document.getElementById('tgStatus');
      if (!btn || !st) return;
      var payload = {
        enabled: document.getElementById('tgEnabled') ? document.getElementById('tgEnabled').checked : false,
        token: document.getElementById('tgToken') ? document.getElementById('tgToken').value : '',
        chatId: document.getElementById('tgChatId') ? document.getElementById('tgChatId').value : '',
        notifyProgramResults: document.getElementById('tgNotifyResults') ? document.getElementById('tgNotifyResults').checked : true,
        notifyStartup: document.getElementById('tgNotifyStartup') ? document.getElementById('tgNotifyStartup').checked : false,
        notifyOverload: document.getElementById('tgNotifyOverload') ? document.getElementById('tgNotifyOverload').checked : true,
        notifyStopped: document.getElementById('tgNotifyStopped') ? document.getElementById('tgNotifyStopped').checked : true
      };
      btn.disabled = true;
      st.textContent = '';
      fetch('/api/telegram', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      .then(function(r) { return r.json(); })
      .then(function(data) {
        btn.disabled = false;
        if (data.status === 'ok') { st.textContent = 'Сохранено'; st.style.color = 'var(--success)'; }
        else { st.textContent = data.message || 'Ошибка'; st.style.color = 'var(--danger)'; }
      })
      .catch(function() {
        btn.disabled = false;
        st.textContent = 'Ошибка запроса';
        st.style.color = 'var(--danger)';
      });
    }
    
    function sendTelegramTest() {
      var btn = document.getElementById('tgTestBtn');
      var st = document.getElementById('tgStatus');
      if (!btn || !st) return;
      btn.disabled = true;
      st.textContent = 'Отправка…';
      st.style.color = '';
      fetch('/api/telegram/test', { method: 'POST' })
        .then(function(r) {
          if (r.status === 202) {
            function poll() {
              fetch('/api/telegram/test/status').then(function(re) { return re.json(); })
                .then(function(d) {
                  if (d.status === 'pending') {
                    setTimeout(poll, 400);
                    return;
                  }
                  btn.disabled = false;
                  st.textContent = d.message || (d.ok ? 'Отправлено' : 'Ошибка');
                  st.style.color = d.ok ? 'var(--success)' : 'var(--danger)';
                })
                .catch(function() {
                  btn.disabled = false;
                  st.textContent = 'Ошибка запроса';
                  st.style.color = 'var(--danger)';
                });
            }
            poll();
            return;
          }
          return r.json();
        })
        .then(function(data) {
          if (!data) return;
          btn.disabled = false;
          if (data.status === 'ok') {
            st.textContent = data.message || 'Отправлено';
            st.style.color = 'var(--success)';
          } else {
            st.textContent = data.message || 'Ошибка';
            st.style.color = 'var(--danger)';
          }
        })
        .catch(function() {
          btn.disabled = false;
          st.textContent = 'Ошибка запроса';
          st.style.color = 'var(--danger)';
        });
    }
    
    var wifiForm = document.getElementById('wifiForm');
    if (wifiForm) {
      wifiForm.addEventListener('submit', function(e) {
        e.preventDefault();
        var formData = new FormData(this);
        var button = this.querySelector('button');
        var originalText = button.textContent;
        
        button.textContent = 'Сохранение...';
        button.disabled = true;
        
        fetch('/config', {
          method: 'POST',
          body: formData
        })
        .then(function(response) { return response.text(); })
        .then(function(data) {
          button.textContent = '✓ Сохранено! Перезапуск...';
        })
        .catch(function(error) {
          button.textContent = '✗ Ошибка!';
          setTimeout(function() {
            button.textContent = originalText;
            button.disabled = false;
          }, 2000);
        });
      });
    }
    
    function resetConfig() {
      if (confirm('Вы уверены, что хотите сбросить все настройки?')) {
        fetch('/reset', { method: 'POST' })
          .then(function(response) { return response.text(); })
          .then(function(data) {
            alert('Настройки сброшены. Устройство перезапустится.');
          });
      }
    }
    
    // Инициализация
    fetch('/apname')
      .then(function(response) { return response.text(); })
      .then(function(apName) {
        var apNameEl = document.getElementById('apName');
        if (apNameEl) {
          apNameEl.textContent = apName;
        }
      });
    
    // Обновление данных: таб-зависимый опрос, реже запросы, без дублей при незавершённом предыдущем
    setInterval(function() { updateMainData(); }, 350);
    setInterval(updateWifiStatus, 3000);
    setInterval(updateLimitsStatus, 2000);
    updateMainData();
    updateWifiStatus();
    updateLimitsStatus();  // Первоначальное обновление при загрузке
  </script>
</body>
</html>
)rawliteral";

const char* TenZillaWeb::getHTMLPage() {
  // Возвращаем указатель на строку в PROGMEM
  // ESP32 автоматически обрабатывает чтение из flash памяти
  return html_page;
}