#ifndef TENZILLA_WEB_H
#define TENZILLA_WEB_H

// Исправление конфликта FS между TFT_eSPI и WebServer
// WebServer ожидает FS в глобальном пространстве имен, но TFT_eSPI использует fs::FS
// Включаем FS.h явно и создаем alias для совместимости
#include <FS.h>
// Создаем alias для совместимости с WebServer
typedef fs::FS FS;

#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// #include <ArduinoJson.h>  // Временно отключено
#include "TenZillaWiFi.h"
#include "TenZillaConfig.h"

class TenZillaWeb {
public:
  static void begin();
  static void handleClient();  // Оставлено для обратной совместимости, но теперь вызывается из задачи
  static bool isClientAllowed();  // Проверка разрешения подключения (максимум 1 клиент)
  
  // Методы для управления подключениями
  static int getMaxConnections();
  static void setMaxConnections(int maxConn);

  static void processDeferredTelegramTest();

private:
  static WebServer server;
  static TaskHandle_t webServerTaskHandle;  // Handle задачи веб-сервера на втором ядре
  static IPAddress allowedClientIP;  // IP разрешенного клиента (для обратной совместимости)
  static IPAddress allowedClients[10];  // Список разрешенных IP (до 10)
  static int allowedClientsCount;  // Количество разрешенных IP
  
  // Задача FreeRTOS для веб-сервера на втором ядре
  static void webServerTask(void* parameter);
  
  // Основные обработчики
  static void handleRoot();
  static void handleAPName();
  static void handleStatus();
  static void handleConfig();
  static void handleReset();
  static void handleRestart();
  
  // Новые обработчики API
  static void handleScaleData();
  static void handleMotorUp();
  static void handleMotorDown();
  static void handleMotorStop();
  static void handleCounterReset();
  static void handleScaleCalibrate();
  static void handleScaleMax();
  static void handleScaleNegativeWeightLimit();
  
  // Пошаговая калибровка
  static void handleCalibrationStart();
  static void handleCalibrationZero();
  static void handleCalibrationPoint();
  static void handleSetNoiseThreshold();
  static void handleSetZeroPoint();
  static void handleSetCalibrationPoint();
  
  // Управление TFT дисплеем
  static void handleDisplayScreen();
  
  // Управление максимумами экранов
  static void handleResetMainMax();
  static void handleResetBreakMax();
  
  // Настройки энкодера
  static void handleEncoderStep();
  static void handleEncoderMin();
  static void handleEncoderMax();
  static void handleEncoderTestOnlyB();

  // Управление ограничениями перемещения
  static void handleMotorLimitsDisable();
  static void handleMotorLimitsEnable();
  static void handleMotorLimitsStatus();
  
  // Настройки веб-сервера
  static void handleMaxConnections();
  
  // Настройки программы
  static void handleProgramCompressionThreshold();
  static void handleProgramCompressionTarget();
  static void handleProgramCompressionUnloadRetract();
  static void handleProgramBreakDropThreshold();
  static void handleProgramStop();           // Остановить программу и двигатель
  static void handleProgramStartCompression();  // Запустить программу СЖАТИЕ
  static void handleProgramStartBreak();        // Запустить программу РАЗРЫВ
  
  // Настройки NTP
  static void handleNTPGet();
  static void handleNTPSetServer();
  static void handleNTPSetInterval();
  static void handleNTPSetTimezone();
  
  // Настройки производительности чтения тензодатчика
  static void handleScalePerformanceGet();
  static void handleScalePerformanceSetMinInterval();
  static void handleScalePerformanceSetWaitMs();
  static void handleScalePerformanceSetI2CSpeed();

  static void handleMeasurements();

  static void handleTelegramGet();
  static void handleTelegramSet();
  static void handleTelegramTest();
  static void handleTelegramTestStatus();

#if defined(ESP32)
  static void handleUpdateGet();
  static void handleUpdatePost();
  static void handleUpdateUpload();
#endif

  static const char* getHTMLPage();
  static String getWiFiNetworks();
};

#endif