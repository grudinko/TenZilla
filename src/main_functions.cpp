// ВАЖНО: Порядок включения имеет значение!
// Сначала подключаем WebServer (TenZillaWeb), потом LVGL (TenZillaDisplay)
// Это предотвращает конфликт между FS из библиотек
#include "TenZillaPins.h"
#include "TenZillaWeb.h"      // Подключаем первым (WebServer должен быть до LVGL)
#include "TenZillaScale.h"
#include "TenZillaWiFi.h"
#include "TenZillaConfig.h"
#include "TenZillaProgram.h"
#include "TenZillaNTP.h"      // NTP клиент для синхронизации времени
#include "TenZillaMeasurements.h"
#include "TenZillaTelegram.h"
#include "TenZillaDisplay.h"  // Подключаем последним (LVGL после WebServer)

// Глобальные функции проекта

// Объявления функций
void testScaleMain();
void updateWiFiDisplayStatusMain();

void tenzilla_setup() {
  // Serial уже инициализирован в setup()
  // Принудительный вывод для проверки работы Serial
  Serial.println();
  Serial.println("=== TenZilla Control System v1.0 ===");
  Serial.println("Modules: Display + NAU7802 Scale + WiFi + Web");
  Serial.print("Setup time: ");
  Serial.println(millis());
  // НЕ используем Serial.flush() - может блокировать при отключенном USB
  delay(100);

  // Инициализация компонентов в правильном порядке
  // ВАЖНО: Если возникает StoreProhibited, попробуйте изменить порядок:
  // Дисплей должен инициализироваться ПОСЛЕ WiFi для избежания конфликтов SPI
  
  Serial.println("⏳ Initializing TenZillaConfig...");
  TenZillaConfig::begin();
  Serial.println("✅ TenZillaConfig initialized");
  
  Serial.println("⏳ Initializing TenZillaScale...");
  TenZillaScale::begin();        // Весы
  Serial.println("✅ TenZillaScale initialized");
  
  Serial.println("⏳ Initializing TenZillaProgram...");
  TenZillaProgram::begin();      // Программа (бипер)
  Serial.println("✅ TenZillaProgram initialized");
  
  Serial.println("⏳ Initializing TenZillaWiFi...");
  TenZillaWiFi::begin();         // WiFi первым
  Serial.println("✅ TenZillaWiFi initialized");
  
  Serial.println("⏳ Initializing TenZillaNTP...");
  TenZillaNTP::begin();          // NTP после WiFi
  Serial.println("✅ TenZillaNTP initialized");

  Serial.println("⏳ Initializing TenZillaMeasurements...");
  TenZillaMeasurements::begin();
  Serial.println("✅ TenZillaMeasurements initialized");
  
  Serial.println("⏳ Initializing TenZillaDisplay...");
  TenZillaDisplay::begin();      // Дисплей после WiFi (может помочь избежать конфликт SPI)
  Serial.println("✅ TenZillaDisplay initialized");
  
  Serial.println("⏳ Initializing TenZillaWeb...");
  TenZillaWeb::begin();
  Serial.println("✅ TenZillaWeb initialized");

  TenZillaTelegram::begin();

  // Отображаем стартовый экран
  TenZillaDisplay::showSplashScreen();

  // Тестируем весы
  testScaleMain();

  // Тест диагностики стабильности
  Serial.println("\nNAU7802 Scale Diagnostics:");
  Serial.println("Noise: " + String(TenZillaScale::getNoiseLevel(), 1) + "%");
  Serial.println("Stable: " + String(TenZillaScale::isStable() ? "YES" : "NO"));
  Serial.println("Errors: " + String(TenZillaScale::getErrorCount()));
  // НЕ используем Serial.flush() - может блокировать при отключенном USB

  Serial.println("OK: TenZilla initialization complete");

  // Сообщение о включении отправляется отложенно из loop после появления сети (см. tenzilla_loop)
}

void updateWiFiDisplayStatusMain() {
  static unsigned long lastWiFiCheck = 0;
  unsigned long now = millis();

  // Обновляем статус WiFi каждые 5 секунд
  if (now - lastWiFiCheck < 5000) return;
  lastWiFiCheck = now;

  bool isConnected = TenZillaWiFi::isConnected();
  String ssid = "";
  String ip = "";
  int rssi = -100;
  int clients = 0;

  if (isConnected) {
    // Подключены к внешней WiFi сети
    ssid = WiFi.SSID();
    ip = WiFi.localIP().toString();
    rssi = WiFi.RSSI();
  } else {
    // Работаем в режиме точки доступа
    ssid = TenZillaWiFi::getAPName();
    ip = WiFi.softAPIP().toString();
    clients = WiFi.softAPgetStationNum();
  }

  // Обновляем дисплей
  TenZillaDisplay::updateWiFiStatus(isConnected, ssid);
  TenZillaDisplay::updateWiFiIP(ip);
  TenZillaDisplay::updateRSSI(rssi);
  TenZillaDisplay::updateWiFiClients(clients);
}

void testScaleMain() {
  Serial.println("\nTesting NAU7802 scale...");
  // НЕ используем Serial.flush() - может блокировать при отключенном USB

  // Даем время на стабилизацию
  delay(1000);

  // Проверяем готовность
  if (TenZillaScale::isReady()) {
    Serial.println("OK: Scale is ready");
    // НЕ используем Serial.flush() - может блокировать при отключенном USB

    // Показываем статус калибровки
    if (TenZillaScale::isCalibrated()) {
      Serial.println("OK: Scale is calibrated");
    } else {
      Serial.println("WARNING: Scale needs calibration!");
    }
    // НЕ используем Serial.flush() - может блокировать при отключенном USB

    // Показываем текущий вес
    float weight = TenZillaScale::getCurrentWeight();
    Serial.println("Current weight: " + String(weight, 1) + " N");
    // НЕ используем Serial.flush() - может блокировать при отключенном USB

  } else {
    Serial.println("ERROR: Scale not ready. Check wiring!");
    // НЕ используем Serial.flush() - может блокировать при отключенном USB
  }
}

void tenzilla_loop() {
  // Обновляем весы
  TenZillaScale::update();
  
  // Обновляем программу (автоматическое управление двигателем)
  TenZillaProgram::update();
  
  // Обновляем NTP (синхронизация времени)
  TenZillaNTP::update();

  float currentWeight = TenZillaScale::getCachedWeight();
  
  // Получаем текущее перемещение (для обновления на экране)
  int opticalCount = TenZillaScale::getOpticalCount();
  bool motorRunning = TenZillaScale::isMotorRunning();
  int motorDirection = TenZillaScale::getMotorDirection();
  
  // Получаем максимальный вес с экрана (обновляется автоматически в updateLVGL при каждом вызове)
  // Обновляем в главном цикле для синхронизации данных
  float maxWeight = TenZillaDisplay::getMainScreenMax();
  
  // Получаем рабочее перемещение (накопленное перемещение при сжатии)
  // Обновляется в updateLVGL, но получаем здесь для синхронизации данных
  float workingDisplacement = TenZillaProgram::isRunning() ? TenZillaProgram::getWorkingDisplacement() : 0.0f;

  // Обновляем статус WiFi для дисплея
  updateWiFiDisplayStatusMain();

  // Обновляем дисплей
  TenZillaDisplay::updateWeight(currentWeight);
  TenZillaDisplay::updateMaxWeight(maxWeight);
  TenZillaDisplay::updateOpticalCount(opticalCount);
  TenZillaDisplay::updateMotorStatus(motorRunning, motorDirection);
  TenZillaDisplay::update();

  // Отложенная отправка теста Telegram (выполняется вне веб-обработчика, чтобы освободить память перед SSL)
  TenZillaWeb::processDeferredTelegramTest();

  // Неблокирующая отправка обычных Telegram-уведомлений из очереди
  TenZillaTelegram::processQueue();

  // Отложенная отправка уведомления о включении системы (одна попытка после появления сети)
  {
    static bool startupNotifyDone = false;
    if (!startupNotifyDone) {
      TenZillaSettings cfg = TenZillaConfig::get();
      if (!cfg.tgEnabled || !cfg.tgNotifyStartup) {
        startupNotifyDone = true;
      } else if (TenZillaWiFi::isConnected()) {
        unsigned long now = millis();
        // Одна попытка после 15 секунд работы и наличия сети
        if (now >= 15000) {
          TenZillaTelegram::send("TenZilla: система включена");
          startupNotifyDone = true;
        }
      }
    }
  }

  // Остальные компоненты
  // Веб-сервер теперь обрабатывается в отдельной задаче FreeRTOS на втором ядре
  // Не нужно вызывать handleClient() здесь - это делается автоматически в webServerTask
  TenZillaWiFi::updateStatus();
}


