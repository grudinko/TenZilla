#ifndef TENZILLA_SCALE_H
#define TENZILLA_SCALE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NAU7802.h>
#include "TenZillaPins.h"  // Централизованные определения пинов

// Forward declaration для избежания циклических зависимостей
class TenZillaProgram;

class TenZillaScale {
public:
  // Инициализация
  static void begin();
  static void update();
  
  // Основные данные весов
  static float getCurrentWeight();   // Чтение I2C (с троттлингом), обновляет кэш
  static float getCachedWeight();    // Только кэш, без I2C — для UI и проверок при уже обновлённом кэше
  static float getRawValue();
  static long getRawReading();  // Прямое чтение RAW без кэширования
  static long getAveragedRawForZero();  // Усреднённое RAW (5 сэмплов), как при калибровке нуля
  static String getStatus();
  static bool isReady();
  
  // Калибровка весов
  static void tare();
  static void calibrateScale(float knownWeight);
  static bool isCalibrated();
  static float getCalibrationFactor();
  static void setCalibrationFactor(float factor);
  
  // Пошаговая калибровка
  static void startCalibration();
  static bool recordZeroPoint();
  static bool recordCalibrationPoint(float knownWeightNewtons);
  static long getZeroRaw();
  static long getCalibrationRaw();
  static void setZeroRaw(long raw);
  static void setCalibrationRaw(long raw);
  static bool isCalibrationInProgress();
  /** Установить ноль по текущему RAW (усреднённому) и пересчитать фактор по второй точке, если есть */
  static bool resetZeroFromCurrent();
  
  // Диагностика стабильности
  static float getNoiseLevel();         // Уровень шума (стабильность)
  static bool isStable();               // Стабильно ли (только по шуму)
  static float getLastReadTime();       // Время последнего чтения (секунды)
  static int getErrorCount();           // Счетчик ошибок чтения
  
  // Настройка порога шума
  static float getNoiseThreshold();     // Получить порог шума (%)
  static void setNoiseThreshold(float threshold); // Установить порог шума (%)
  
  // Настройки производительности чтения тензодатчика
  static unsigned long getMinReadInterval();  // Получить минимальный интервал между чтениями (мс)
  static void setMinReadInterval(unsigned long intervalMs);  // Установить интервал (10-100 мс)
  static unsigned long getNAU7802WaitMs();  // Получить таймаут ожидания готовности NAU7802 (мс)
  static void setNAU7802WaitMs(unsigned long waitMs);  // Установить таймаут (5-50 мс)
  static unsigned long getI2CSpeed();  // Получить скорость I2C шины (Гц)
  static void setI2CSpeed(unsigned long speedHz);  // Установить скорость (100000, 400000 Гц)
  
  // Управление мотором (через реле)
  static void motorUp();
  static void motorDown();
  static void motorStop();
  static bool isMotorRunning();
  static int getMotorDirection(); // Возвращает: 1 (вверх), -1 (вниз), 0 (стоп)

  /** Кнопки «вверх/вниз» на панели (удержание). Вызывается из update(). */
  static void updateMotorPanelButtons();
  
  // Настройка активного уровня реле
  static void setRelayActiveHigh(bool activeHigh);
  static bool getRelayActiveHigh();
  
  // Оптический счетчик (заглушки)
  static int getOpticalCount();
  static void resetOpticalCount();
  static float getMaxWeight();
  static void setMaxWeight(float weight);
  /** Порог аварийного стопа при отрицательном весе (Н), по умолчанию -50. Сохраняется в NVS. */
  static float getNegativeWeightLimit();
  static void setNegativeWeightLimit(float limitN);
  /** Сохранить настройки весов в NVS (вызывать после setMaxWeight и др. для сохранения после перезагрузки). */
  static void saveConfig();
  
  // Настройки энкодера для расчета перемещения
  static void setEncoderStepMm(float stepMm);  // Шаг в мм на импульс энкодера
  static float getEncoderStepMm();
  static void setEncoderMin(int minValue);  // Минимальное значение энкодера
  static int getEncoderMin();
  static void setEncoderMax(int maxValue);  // Максимальное значение энкодера
  static int getEncoderMax();
  static float getDisplacement();  // Перемещение в мм (count * stepMm)
  /** Тестовый режим энкодера: только B, направление от мотора. false = A+B, направление по энкодеру. */
  static void setEncoderTestOnlyB(bool enable);
  static bool getEncoderTestOnlyB();
  /** Сбой энкодера: мотор работал, но счёт не менялся. Сбрасывается при новом запуске мотора. */
  static bool getEncoderFault();
  static void clearEncoderFault();
  /** Отложенное действие меню: 1 = RESET MOV, 2 = RESET ZERO. Выполняется в Scale::update(). */
  static void setPendingMenuAction(int action);
  
  // Управление ограничениями перемещения (временное отключение с пин-кодом)
  static bool disableLimits(int pinCode);  // Отключить ограничения (требует пин-код 1861)
  static void enableLimits();             // Включить ограничения обратно
  static bool areLimitsDisabled();        // Проверить, отключены ли ограничения
  
private:
  static Adafruit_NAU7802 scale;
  static bool initialized;
  static bool calibrated;
  static float calibration_factor;
  static float maxWeight;
  static float negativeWeightLimitN;  // Порог стопа при отрицательном весе (Н), по умолчанию -50
  static int opticalCount;
  static bool motorRunning;
  static int motorDirection; // 1 = вверх, -1 = вниз, 0 = стоп
  static bool relayActiveHigh; // Активный уровень реле: true = HIGH активен, false = LOW активен
  
  // Диагностические переменные
  static float noiseLevel;
  static float noiseThreshold;  // Порог шума для определения стабильности (%)
  static unsigned long lastReadTime;
  static int readErrorCount;
  static long lastRawReadings[10]; // История RAW значений для анализа шума (независимо от калибровки)
  static int readingIndex;
  
  // Переменные для пошаговой калибровки
  static bool calibrationInProgress;
  static long zeroRaw;
  static long calibrationRaw;
  
  // Параметры энкодера для расчета перемещения
  static float encoderStepMm;      // Шаг в мм на импульс энкодера
  static int encoderMin;           // Минимальное значение энкодера
  static int encoderMax;           // Максимальное значение энкодера
  static bool encoderTestOnlyB;    // true = только B (тест); false = A+B, направление по энкодеру (по умолчанию)
  
  // Состояние отключения ограничений (временное, не сохраняется)
  static bool limitsDisabled;      // Отключены ли ограничения по перемещению
  static unsigned long limitsDisabledTime;  // Время отключения ограничений (для таймаута 10 минут)
  static bool encoderFault;        // Сбой энкодера: мотор работал, счёт не менялся
  static unsigned long encoderFaultCheckStart;
  static int encoderFaultLastCount;
  
  // Пины определены в TenZillaPins.h
  // Используем макросы из TenZillaPins.h напрямую в коде

  // Константы
  static const float DEFAULT_CALIBRATION_FACTOR;
  
  // Настройки производительности чтения
  static unsigned long minReadInterval;  // Минимальный интервал между чтениями (мс)
  static unsigned long nau7802WaitMs;     // Таймаут ожидания готовности NAU7802 (мс)
  static unsigned long i2cSpeed;         // Скорость I2C шины (Гц)
  
  // Вспомогательные методы
  static void updateNoiseLevel();
  static void readConfig();
};

#endif