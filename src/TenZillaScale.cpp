#include "TenZillaScale.h"
#include "TenZillaEncoder.h"
#include "TenZillaProgram.h"
#include "TenZillaWeb.h"
#include "TenZillaDisplay.h"
#include <Preferences.h>
#include <cmath>

// ============================================
// ИНИЦИАЛИЗАЦИЯ СТАТИЧЕСКИХ ЧЛЕНОВ
// ============================================

// Объект NAU7802 (I2C)
Adafruit_NAU7802 TenZillaScale::scale;

// Константа по умолчанию: RAW -> N (ранее было для граммов, разделено на 100). Единственное место задания.
const float TenZillaScale::DEFAULT_CALIBRATION_FACTOR = -70.5f;

// Основные переменные
bool TenZillaScale::initialized = false;
bool TenZillaScale::calibrated = false;
float TenZillaScale::calibration_factor = TenZillaScale::DEFAULT_CALIBRATION_FACTOR;
float TenZillaScale::maxWeight = 500.0f;  // N (предел перегрузки), загружается из NVS в readConfig()
float TenZillaScale::negativeWeightLimitN = -50.0f;  // Н, порог стопа при отрицательном весе, загружается из NVS
int TenZillaScale::opticalCount = 0;
bool TenZillaScale::motorRunning = false;
int TenZillaScale::motorDirection = 0; // 1 = вверх, -1 = вниз, 0 = стоп
bool TenZillaScale::relayActiveHigh = false; // Активный уровень реле: false = LOW активен (инвертировано)

// Переменные для пошаговой калибровки
bool TenZillaScale::calibrationInProgress = false;
long TenZillaScale::zeroRaw = 0;
long TenZillaScale::calibrationRaw = 0;

// Параметры энкодера (по умолчанию)
float TenZillaScale::encoderStepMm = 0.01f;     // Шаг в мм на импульс энкодера (0.01 мм = 10 мкм)
int TenZillaScale::encoderMin = 0;               // Минимальное значение (всегда 0, не хранится)
int TenZillaScale::encoderMax = 200000;          // Максимальное значение: 2000 мм / 0.01 мм = 200000 импульсов
bool TenZillaScale::encoderTestOnlyB = false;    // По умолчанию: A+B, направление по энкодеру

// Состояние отключения ограничений (временное, не сохраняется при перезагрузке)
bool TenZillaScale::limitsDisabled = false;
unsigned long TenZillaScale::limitsDisabledTime = 0;  // Время отключения (для таймаута 10 минут)
bool TenZillaScale::encoderFault = false;
unsigned long TenZillaScale::encoderFaultCheckStart = 0;
int TenZillaScale::encoderFaultLastCount = 0;

// Диагностические переменные
float TenZillaScale::noiseLevel = 0.0f;
float TenZillaScale::noiseThreshold = 5.0f; // Порог шума по умолчанию: 5.0%
unsigned long TenZillaScale::lastReadTime = 0;
int TenZillaScale::readErrorCount = 0;
long TenZillaScale::lastRawReadings[10] = {0}; // История RAW значений для анализа шума
int TenZillaScale::readingIndex = 0;

// Для периодического сохранения opticalCount
static unsigned long lastOpticalCountSave = 0;
static const unsigned long OPTICAL_COUNT_SAVE_INTERVAL = 5000; // Сохраняем каждые 5 секунд
static int lastSavedOpticalCount = 0;

// Кэш для защиты от слишком частых чтений
static float cachedWeight = 0.0f;
static long cachedRawValue = 0;  // Кэш последнего RAW значения для getRawValue/getRawReading
static unsigned long lastSuccessfulRead = 0;

static int s_pendingMenuAction = -1;  // 1 = RESET MOV, 2 = RESET ZERO; выполняем в update()

// Настройки производительности чтения (инициализируются значениями по умолчанию)
unsigned long TenZillaScale::minReadInterval = 25;   // Минимальный интервал между чтениями (мс) - 40 Гц обновления
unsigned long TenZillaScale::nau7802WaitMs = 12;     // Таймаут ожидания готовности NAU7802 (мс)
unsigned long TenZillaScale::i2cSpeed = 100000;      // Скорость I2C шины (Гц) - 100kHz по умолчанию

// NVS для сохранения настроек
Preferences preferences;

// ============================================
// ОСНОВНЫЕ МЕТОДЫ
// ============================================

// Вспомогательные: ожидание готовности и усреднённое чтение NAU7802
static bool nau7802_wait_ready_timeout(Adafruit_NAU7802& nau, unsigned long timeout_ms) {
  unsigned long start = millis();
  while (millis() - start < timeout_ms) {
    if (nau.available()) return true;
    delay(1);
  }
  return false;
}

static long nau7802_read_average(Adafruit_NAU7802& nau, int times) {
  int32_t sum = 0;
  int n = 0;
  unsigned long waitMs = TenZillaScale::getNAU7802WaitMs();  // Используем геттер для доступа к private переменной
  for (int i = 0; i < times; i++) {
    if (nau7802_wait_ready_timeout(nau, waitMs) && nau.available()) {
      sum += nau.read();
      n++;
    }
    if (i + 1 < times) delay(1);
  }
  return (n > 0) ? (long)(sum / n) : 0;
}

void TenZillaScale::begin() {
  Serial.println("NAU7802: Initializing...");
  delay(10);

  Serial.println("📖 [TenZillaScale] Starting readConfig()...");
  Serial.flush();
  readConfig();
  Serial.flush();
  Serial.println("✅ Calibration data read from NVS");

  Serial.println("⏳ Initializing NAU7802 (I2C)...");
  Serial.print("   SDA: GPIO");
  Serial.print(NAU7802_I2C_SDA);
  Serial.print(", SCL: GPIO");
  Serial.println(NAU7802_I2C_SCL);
  delay(100);

  Wire.begin(NAU7802_I2C_SDA, NAU7802_I2C_SCL);
  Wire.setClock(i2cSpeed);  // Устанавливаем скорость I2C из настроек
  delay(50);

  if (!scale.begin()) {
    Serial.println("❌ NAU7802 not found. Check I2C wiring (SDA/SCL) and address 0x2A.");
    initialized = false;
    return;
  }
  Serial.println("✅ NAU7802 begun");

  // Gain 64 (как у HX711) — баланс скорость/точность. При смене gain нужна перекалибровка.
  scale.setGain(NAU7802_GAIN_64);
  scale.setLDO(NAU7802_3V3);
  if (!scale.calibrate(NAU7802_CALMOD_INTERNAL)) {
    Serial.println("⚠️ NAU7802 internal calibrate failed (continue anyway)");
  }
  delay(50);
  Serial.println("✅ NAU7802 gain 64, LDO 3V3");
  
  // Инициализация энкодера
  TenZillaEncoder::begin(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_SW);
#if ENCODER_HALL
  TenZillaEncoder::setHallMode(true);
#endif
#if ENCODER_PULSE_DIR
  TenZillaEncoder::setPulseDirMode(true);
  #if ENCODER_PULSE_DIR_INVERT
  TenZillaEncoder::setInverted(true);
  #endif
#endif
  TenZillaEncoder::setPulseOnlyMode(getEncoderTestOnlyB());
#if ENCODER_INVERT_A
  TenZillaEncoder::setInvertA(true);
#endif
#if ENCODER_INVERT_B
  TenZillaEncoder::setInvertB(true);
#endif
  
  // ВАЖНО: Устанавливаем загруженное значение opticalCount в энкодер
  // Это нужно делать ПОСЛЕ инициализации энкодера, но ДО первого update()
  if (opticalCount != 0) {
    TenZillaEncoder::setCount(opticalCount);
    Serial.print("🔄 [begin] Encoder count set to loaded value: ");
    Serial.println(opticalCount);
  }
  
  delay(50);
  delay(10);
  
  // Инициализация реле для управления двигателем
  Serial.println("⏳ Initializing relay...");
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  // Устанавливаем неактивный уровень (зависит от типа реле)
  // Загружаем настройку из конфига (если была сохранена)
  // relayActiveHigh уже загружен через readConfig() выше
  int inactiveLevel = relayActiveHigh ? (int)LOW : (int)HIGH;
  digitalWrite(RELAY_PIN_1, inactiveLevel);
  digitalWrite(RELAY_PIN_2, inactiveLevel);
  Serial.println("🔌 Relay initialized:");
  Serial.println("   Relay Channel 1: GPIO " + String(RELAY_PIN_1));
  Serial.println("   Relay Channel 2: GPIO " + String(RELAY_PIN_2));
  Serial.println("   Active level: " + String(relayActiveHigh ? "HIGH" : "LOW"));

#if MOTOR_BTN_UP_PIN >= 0
  pinMode(MOTOR_BTN_UP_PIN, INPUT_PULLUP);
  Serial.println("   Motor UP button: GPIO " + String(MOTOR_BTN_UP_PIN));
#endif
#if MOTOR_BTN_DOWN_PIN >= 0
  pinMode(MOTOR_BTN_DOWN_PIN, INPUT_PULLUP);
  Serial.println("   Motor DOWN button: GPIO " + String(MOTOR_BTN_DOWN_PIN));
#endif
  
  // NAU7802 не хранит scale/offset — используем zeroRaw и calibration_factor в коде
  if (calibrated && zeroRaw != 0) {
    Serial.println("OK: Using saved calibration:");
    Serial.println("   Factor: " + String(calibration_factor));
    Serial.println("   ZeroRaw: " + String(zeroRaw));
    Serial.println("   CalRaw: " + String(calibrationRaw));
  } else {
    Serial.println("WARNING: Using default calibration factor: " + String(DEFAULT_CALIBRATION_FACTOR));
    Serial.println("   Note: Scale is not calibrated. Please calibrate before use.");
  }

  Serial.println("⏳ Checking NAU7802 readiness...");
  if (nau7802_wait_ready_timeout(scale, 1000)) {
    Serial.println("✅ NAU7802 is ready");
    initialized = true;
    Serial.println("⏳ Performing initial weight reading...");
    getCurrentWeight();
    Serial.println("✅ Initial reading complete");
    Serial.println("OK: NAU7802 initialized");
  } else {
    Serial.println("ERROR: NAU7802 not ready. Check I2C wiring.");
    initialized = false;
  }
  Serial.println("✅ TenZillaScale::begin() completed");
}

void TenZillaScale::update() {
  if (!initialized) return;

  TenZillaWeb::handleClient();

  if (s_pendingMenuAction >= 0) {
    int a = s_pendingMenuAction;
    s_pendingMenuAction = -1;
    if (a == 1) TenZillaEncoder::resetCount();
    else if (a == 2) resetZeroFromCurrent();
  }

  TenZillaEncoder::update();
  int rawCount = TenZillaEncoder::getCount();
  unsigned long now = millis();

  // Режим «только B»: повторно применить после ~800 ms с Boot, чтобы lastPulseDirB
  // синхронизировался с пином (при старте пины могут быть нестабильны).
  {
    static bool s_pulseOnlyReapplied = false;
    if (!s_pulseOnlyReapplied && getEncoderTestOnlyB() && now >= 800) {
      s_pulseOnlyReapplied = true;
      TenZillaEncoder::setPulseOnlyMode(true);
    }
  }

  // Проверка таймаута для отключенных ограничений (10 минут)
  if (limitsDisabled && limitsDisabledTime > 0) {
    const unsigned long LIMITS_DISABLE_TIMEOUT_MS = 10 * 60 * 1000;  // 10 минут
    if (now - limitsDisabledTime >= LIMITS_DISABLE_TIMEOUT_MS) {
      limitsDisabled = false;
      limitsDisabledTime = 0;
      Serial.println("⏰ Displacement limits automatically ENABLED (10 min timeout expired)");
    }
  }
  
  getCurrentWeight();

  bool wasMotorRunning = motorRunning;
  updateMotorPanelButtons();

  if (motorRunning) {
    // Проверка: мотор работает, но энкодер не меняется — неисправность энкодера
    if (!wasMotorRunning) {
      // Синхронизация после motorUp/motorDown в том же кадре (now мог быть раньше millis() в motorUp)
      encoderFaultCheckStart = millis();
      encoderFaultLastCount = rawCount;
    }
    unsigned long faultNow = millis();
    if (rawCount != encoderFaultLastCount) {
      encoderFaultLastCount = rawCount;
      encoderFaultCheckStart = faultNow;
    } else {
      const unsigned long ENCODER_FAULT_MS = 1500;
      if (faultNow >= encoderFaultCheckStart &&
          faultNow - encoderFaultCheckStart >= ENCODER_FAULT_MS) {
        encoderFault = true;
        motorStop();
        TenZillaProgram::beepError();
        if (TenZillaProgram::isRunning()) {
          TenZillaProgram::stopProgram(TenZillaProgram::STOP_REASON_ENCODER_FAULT);
        }
        return;
      }
    }
    // Проверка ограничений по весу (всегда активна, даже при отключенных ограничениях перемещения)
    float currentWeightN = getCachedWeight();
    float maxWeightN = getMaxWeight();
    if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
      motorStop();
      TenZillaProgram::beepError();
      return;
    }
    // Аварийный стоп при сильном отрицательном весе (сбой датчика / растяжение)
    if (currentWeightN < getNegativeWeightLimit()) {
      motorStop();
      TenZillaProgram::beepError();
      return;
    }
    // Проверка ограничений по перемещению (отключается при limitsDisabled)
    if (!limitsDisabled) {
      int encoderMin = getEncoderMin();
      int encoderMax = getEncoderMax();
      if (motorDirection == 1 && rawCount <= encoderMin) {
        motorStop();
        TenZillaProgram::beepError();
        return;
      }
      if (motorDirection == -1 && rawCount >= encoderMax) {
        motorStop();
        TenZillaProgram::beepError();
        return;
      }
    }
  }

  int oldCount = opticalCount;
  opticalCount = rawCount;

  bool shouldSave = false;
  if (opticalCount != lastSavedOpticalCount) {
    if (lastOpticalCountSave == 0)
      shouldSave = true;
    else if (now - lastOpticalCountSave >= OPTICAL_COUNT_SAVE_INTERVAL)
      shouldSave = true;
    else if (abs(opticalCount - lastSavedOpticalCount) > 10)
      shouldSave = true;
    else if (!motorRunning && oldCount != opticalCount)
      shouldSave = true;

    if (shouldSave) {
      preferences.begin("tenzilla-scale", false);
      preferences.putInt("optical_count", opticalCount);
      preferences.end();
      lastOpticalCountSave = now;
      lastSavedOpticalCount = opticalCount;
    }
  }

  updateNoiseLevel();
}

float TenZillaScale::getCurrentWeight() {
  if (!initialized) {
    readErrorCount++;
    return cachedWeight; // Возвращаем кэшированное значение вместо 0
  }
  
  unsigned long now = millis();
  if (lastSuccessfulRead != 0 && (now - lastSuccessfulRead) < getMinReadInterval())
    return cachedWeight;

  if (nau7802_wait_ready_timeout(scale, getNAU7802WaitMs())) {
    long currentRaw = nau7802_read_average(scale, 1);
    if (currentRaw == 0) {
      readErrorCount++;
      return (lastSuccessfulRead != 0) ? cachedWeight : 0.0f;
    }

    // Все измерения и хранения исключительно в ньютонах
    float weightN;
    if (calibrated && zeroRaw != 0 && calibration_factor != 0)
      weightN = (float)(currentRaw - zeroRaw) / calibration_factor;
    else
      weightN = (float)currentRaw / DEFAULT_CALIBRATION_FACTOR;

    // Сохраняем RAW значение для расчета шума (независимо от калибровки)
    lastRawReadings[readingIndex] = currentRaw;
    readingIndex = (readingIndex + 1) % 10;
    lastReadTime = millis();

    if (abs(weightN) < 1000.0f) {
      cachedWeight = weightN;
      cachedRawValue = currentRaw;
      lastSuccessfulRead = now;
    }
    return weightN;
  }
  readErrorCount++;
  return cachedWeight;  // N
}

float TenZillaScale::getCachedWeight() {
  return cachedWeight;
}

void TenZillaScale::setPendingMenuAction(int action) {
  s_pendingMenuAction = action;
}

float TenZillaScale::getRawValue() {
  if (!initialized) return 0.0f;
  unsigned long now = millis();
  if (now - lastSuccessfulRead < 200 && lastSuccessfulRead != 0 && cachedRawValue != 0)
    return (float)cachedRawValue;
  if (nau7802_wait_ready_timeout(scale, 200)) {
    long rawValue = nau7802_read_average(scale, 1);
    if (rawValue == 0) {
      readErrorCount++;
      return (cachedRawValue != 0) ? (float)cachedRawValue : 0.0f;
    }
    cachedRawValue = rawValue;
    lastSuccessfulRead = now;
    return (float)rawValue;
  }
  readErrorCount++;
  return (cachedRawValue != 0) ? (float)cachedRawValue : 0.0f;
}

long TenZillaScale::getRawReading() {
  if (!initialized) return 0;
  unsigned long now = millis();
  if (lastSuccessfulRead != 0 && (now - lastSuccessfulRead) < 200 && cachedRawValue != 0)
    return cachedRawValue;
  if (nau7802_wait_ready_timeout(scale, 350) && scale.available()) {
    long rawValue = (long)scale.read();
    if (rawValue == 0) {
      readErrorCount++;
      return 0;
    }
    cachedRawValue = rawValue;
    lastSuccessfulRead = now;
    return rawValue;
  }
  readErrorCount++;
  return 0;
}

long TenZillaScale::getAveragedRawForZero() {
  if (!initialized) return 0;
  if (!nau7802_wait_ready_timeout(scale, 50)) return (long)cachedRawValue;  /* 50 ms — не блокировать UI */
  return nau7802_read_average(scale, 5);
}

String TenZillaScale::getStatus() {
  if (!initialized) return "NOT_INITIALIZED";
  if (!isReady()) return "NOT_READY";
  if (!calibrated) return "NEEDS_CALIBRATION";
  if (!isStable()) return "UNSTABLE";
  return "READY";
}

bool TenZillaScale::isReady() {
  return initialized && nau7802_wait_ready_timeout(scale, 300);
}

// ============================================
// МЕТОДЫ КАЛИБРОВКИ
// ============================================

void TenZillaScale::tare() {
  if (!initialized) return;
  long raw = nau7802_read_average(scale, 5);
  zeroRaw = raw;
  calibrated = false;
  calibrationRaw = 0;
}

void TenZillaScale::calibrateScale(float knownWeight) {
  if (!initialized) return;
  long rawZero = nau7802_read_average(scale, 5);
  zeroRaw = rawZero;
  delay(500);
  delay(3000);
  long rawCal = nau7802_read_average(scale, 10);
  calibrationRaw = rawCal;
  long diff = rawCal - rawZero;
  if (diff == 0 || knownWeight <= 0) return;
  // knownWeight в N; calibration_factor переводит RAW напрямую в ньютоны
  calibration_factor = (float)diff / knownWeight;
  calibrated = true;
  saveConfig();
}

// ============================================
// ПОШАГОВАЯ КАЛИБРОВКА
// ============================================

void TenZillaScale::startCalibration() {
  if (!initialized) return;
  
  // Блокируем запуск калибровки при отключенных ограничениях
  if (limitsDisabled) {
    Serial.println("❌ Calibration blocked: displacement limits are disabled");
    return;
  }
  
  calibrationInProgress = true;
  zeroRaw = 0;
  calibrationRaw = 0;
  calibrated = false;
  preferences.begin("tenzilla-scale", false);
  preferences.remove("zero_raw");
  preferences.remove("cal_raw");
  preferences.end();
}

bool TenZillaScale::recordZeroPoint() {
  if (!initialized || !calibrationInProgress) return false;
  delay(300);
  long raw1 = nau7802_read_average(scale, 3);
  delay(300);
  long raw2 = nau7802_read_average(scale, 3);
  if (abs(raw1 - raw2) > 100) return false;
  zeroRaw = (raw1 + raw2) / 2;
  preferences.begin("tenzilla-scale", false);
  preferences.putLong("zero_raw", zeroRaw);
  preferences.remove("cal_raw");
  preferences.end();
  return true;
}

bool TenZillaScale::recordCalibrationPoint(float knownWeightNewtons) {
  if (!initialized || !calibrationInProgress || zeroRaw == 0) return false;
  
  // ВАЖНО: При записи калибровочной точки offset еще НЕ установлен в scale
  // Поэтому мы читаем RAW значение напрямую (без учета offset)
  // Это правильно, так как offset будет установлен только после вычисления factor
  
  delay(500);
  long raw1 = nau7802_read_average(scale, 5);
  delay(500);
  long raw2 = nau7802_read_average(scale, 5);
  if (abs(raw1 - raw2) > 100) return false;
  calibrationRaw = (raw1 + raw2) / 2;
  long rawDifference = calibrationRaw - zeroRaw;
  if (rawDifference == 0 || knownWeightNewtons <= 0) return false;
  
  // ВАЖНО: Порядок установки критичен для правильной работы!
  // Сначала устанавливаем scale (временный), потом offset, потом правильный scale
  //
  // calibration_factor переводит RAW напрямую в ньютоны: weight_N = (raw - zeroRaw) / factor.
  // knownWeightNewtons в N; factor = rawDifference / knownWeightNewtons.
  calibration_factor = (float)rawDifference / knownWeightNewtons;
  calibrated = true;
  calibrationInProgress = false;
  saveConfig();
  return true;
}

long TenZillaScale::getZeroRaw() {
  return zeroRaw;
}

long TenZillaScale::getCalibrationRaw() {
  return calibrationRaw;
}

void TenZillaScale::setZeroRaw(long raw) {
  zeroRaw = raw;
  saveConfig(); // Сохраняем изменения
}

void TenZillaScale::setCalibrationRaw(long raw) {
  calibrationRaw = raw;
  saveConfig(); // Сохраняем изменения
}

bool TenZillaScale::isCalibrationInProgress() {
  return calibrationInProgress;
}

bool TenZillaScale::isCalibrated() {
  return calibrated;
}

float TenZillaScale::getCalibrationFactor() {
  return calibration_factor;
}

void TenZillaScale::setCalibrationFactor(float factor) {
  calibration_factor = factor;
  calibrated = true;
  saveConfig();
}

bool TenZillaScale::resetZeroFromCurrent() {
  if (!initialized) return false;
  long newZero = getAveragedRawForZero();
  long oldZero = zeroRaw;
  setZeroRaw(newZero);
  if (calibrationRaw != 0 && calibration_factor > 0.0001f) {
    long rawDiff = calibrationRaw - oldZero;
    if (rawDiff != 0) {
      long newDiff = calibrationRaw - newZero;
      float newFactor = (float)newDiff * calibration_factor / (float)rawDiff;
      if (newFactor > 0.0001f && newFactor < 100000.0f) {
        calibration_factor = newFactor;
        calibrated = true;
        saveConfig();
      }
    }
  }
  return true;
}

// ============================================
// МЕТОДЫ ДИАГНОСТИКИ НАПРЯЖЕНИЯ
// ============================================

float TenZillaScale::getNoiseLevel() {
  return noiseLevel;
}

bool TenZillaScale::isStable() {
  // Стабильность оценивается по уровню шума < noiseThreshold (настраиваемый порог)
  return (noiseLevel < noiseThreshold);
}

float TenZillaScale::getNoiseThreshold() {
  return noiseThreshold;
}

void TenZillaScale::setNoiseThreshold(float threshold) {
  if (threshold < 0.1f) threshold = 0.1f;
  if (threshold > 50.0f) threshold = 50.0f;
  noiseThreshold = threshold;
  saveConfig();
}

// ============================================
// НАСТРОЙКИ ПРОИЗВОДИТЕЛЬНОСТИ ЧТЕНИЯ
// ============================================

unsigned long TenZillaScale::getMinReadInterval() {
  return minReadInterval;
}

void TenZillaScale::setMinReadInterval(unsigned long intervalMs) {
  if (intervalMs < 10) intervalMs = 10;
  if (intervalMs > 100) intervalMs = 100;
  minReadInterval = intervalMs;
  saveConfig();
}

unsigned long TenZillaScale::getNAU7802WaitMs() {
  return nau7802WaitMs;
}

void TenZillaScale::setNAU7802WaitMs(unsigned long waitMs) {
  if (waitMs < 5) waitMs = 5;
  if (waitMs > 50) waitMs = 50;
  nau7802WaitMs = waitMs;
  saveConfig();
}

unsigned long TenZillaScale::getI2CSpeed() {
  return i2cSpeed;
}

void TenZillaScale::setI2CSpeed(unsigned long speedHz) {
  // Разрешенные значения: 100000, 200000, 300000, 400000 (100kHz, 200kHz, 300kHz, 400kHz)
  // Округляем до ближайшего разрешенного значения
  if (speedHz < 150000) {
    speedHz = 100000;  // 100 kHz
  } else if (speedHz < 250000) {
    speedHz = 200000;  // 200 kHz
  } else if (speedHz < 350000) {
    speedHz = 300000;  // 300 kHz
  } else {
    speedHz = 400000;  // 400 kHz
  }
  
  i2cSpeed = speedHz;
  
  // Применяем новую скорость I2C немедленно, если шина уже инициализирована
  if (initialized) {
    Wire.setClock(i2cSpeed);
  }
  
  saveConfig();
}

float TenZillaScale::getLastReadTime() {
  if (lastReadTime == 0) return 999.9f;
  return (millis() - lastReadTime) / 1000.0f; // В секундах
}

int TenZillaScale::getErrorCount() {
  return readErrorCount;
}


void TenZillaScale::updateNoiseLevel() {
  static unsigned long lastNoiseUpdate = 0;
  unsigned long now = millis();
  
  // Обновляем каждые 2 секунды
  if (now - lastNoiseUpdate < 2000) return;
  lastNoiseUpdate = now;
  
  // Рассчитываем стандартное отклонение последних RAW измерений
  // Это дает объективную оценку стабильности датчика, независимо от калибровки
  float sum = 0.0f;
  float sumSq = 0.0f;
  int count = 0;
  
  for (int i = 0; i < 10; i++) {
    if (lastRawReadings[i] != 0) {
      float rawValue = (float)lastRawReadings[i];
      sum += rawValue;
      sumSq += rawValue * rawValue;
      count++;
    }
  }
  
  if (count > 1) {
    float mean = sum / count;
    // Используем минимальное значение для расчета процентов шума
    // Для RAW значений типичный диапазон: -100000 до +100000 (24-битный АЦП)
    // Минимум 1000 единиц АЦП предотвращает завышенные проценты при малых значениях
    float meanForPercent = (abs(mean) > 1000.0f) ? abs(mean) : 1000.0f;
    
    if (meanForPercent > 1.0f) { // Избегаем деления на ноль
      float variance = (sumSq / count) - (mean * mean);
      if (variance > 0) {
        float stdDev = sqrt(variance);
        // Рассчитываем процент шума относительно среднего RAW значения
        // Это отражает реальную стабильность АЦП, не зависящую от калибровки
        noiseLevel = stdDev / meanForPercent * 100.0f; // В процентах
      } else {
        noiseLevel = 0.0f;
      }
    } else {
      noiseLevel = 0.0f;
    }
  } else {
    noiseLevel = 0.0f;
  }
}


// ============================================
// ЗАГЛУШКИ ДЛЯ МОТОРА И СЧЕТЧИКА
// ============================================

void TenZillaScale::updateMotorPanelButtons() {
#if MOTOR_BTN_UP_PIN < 0 && MOTOR_BTN_DOWN_PIN < 0
  return;
#else
  static bool panelMotorActive = false;
  static bool upStable = false;
  static bool downStable = false;
  static unsigned long upChangeMs = 0;
  static unsigned long downChangeMs = 0;
  const unsigned long DEBOUNCE_MS = 40;

  unsigned long now = millis();

  auto debounceBtn = [&](int pin, bool& stable, unsigned long& changeMs) -> bool {
    if (pin < 0) return false;
    bool raw = (digitalRead(pin) == LOW);
    if (raw != stable) {
      if (changeMs == 0) changeMs = now;
      else if (now - changeMs >= DEBOUNCE_MS) {
        stable = raw;
        changeMs = 0;
      }
    } else {
      changeMs = 0;
    }
    return stable;
  };

  bool upPressed = debounceBtn(MOTOR_BTN_UP_PIN, upStable, upChangeMs);
  bool downPressed = debounceBtn(MOTOR_BTN_DOWN_PIN, downStable, downChangeMs);

  // During auto-program: UP button stops the program (motor is controlled by program only)
  if (TenZillaProgram::isRunning()) {
    static bool upProgramStopLatched = false;
    if (upPressed) {
      if (!upProgramStopLatched) {
        upProgramStopLatched = true;
        TenZillaProgram::stopProgram(TenZillaProgram::STOP_REASON_MANUAL);
        TenZillaProgram::beepShort();
      }
    } else {
      upProgramStopLatched = false;
    }
    return;
  }

  if (upPressed || downPressed) {
    if (!(upPressed && downPressed)) {
      clearEncoderFault();
    }
    panelMotorActive = true;
    if (upPressed && downPressed) {
      motorStop();
    } else if (upPressed) {
      if (getMotorDirection() != 1) {
        if (motorRunning) motorStop();
        motorUp();
      }
    } else {
      if (getMotorDirection() != -1) {
        if (motorRunning) motorStop();
        motorDown();
      }
    }
  } else if (panelMotorActive) {
    motorStop();
    panelMotorActive = false;
  }
#endif
}

void TenZillaScale::motorUp() {
  // Контроль лимитов и перегрузки перед запуском
  int encoderCount = getOpticalCount();
  float currentWeightN = getCurrentWeight();  // N
  int encoderMax = getEncoderMax();
  int encoderMin = getEncoderMin();
  
  // Проверка ограничений по перемещению (отключается при limitsDisabled)
  if (!limitsDisabled) {
    // Блокируем только направление выхода за пределы
    // ВАЖНО: Ноль вверху! Движение вниз УВЕЛИЧИВАЕТ count, движение вверх УМЕНЬШАЕТ count
    // После исправления энкодера: UP (direction=1) -> delta=-1 -> count уменьшается ✓
    // - Если count уже минимальный (0) - блокируем UP (нельзя идти выше нуля)
    // - Если count превысил максимум - разрешаем UP (это деблокирующее направление - уменьшает count)
    if (encoderCount <= encoderMin) {
      TenZillaProgram::beepError();
      // Статусное сообщение будет показано в updateLVGL
      return;
    }
    
    // Если превышен максимум - разрешаем UP (это деблокирующее направление - уменьшает count)
    // НЕ блокируем UP при count > max, так как это деблокирующее направление
  }
  
  // Проверка перегрузки по весу (maxWeight в N) - всегда активна
  float maxWeightN = getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    TenZillaProgram::beepError();
    return;
  }
  // Блокируем запуск при сильном отрицательном весе (сбой датчика)
  if (currentWeightN < getNegativeWeightLimit()) {
    TenZillaProgram::beepError();
    return;
  }
  
  clearEncoderFault();
  encoderFaultCheckStart = millis();
  encoderFaultLastCount = TenZillaEncoder::getCount();
  motorRunning = true;
  motorDirection = 1;
  TenZillaEncoder::setMotorDirection(1);
  int activeLevel = relayActiveHigh ? HIGH : LOW;
  int inactiveLevel = relayActiveHigh ? LOW : HIGH;
  digitalWrite(RELAY_PIN_1, activeLevel);
  digitalWrite(RELAY_PIN_2, inactiveLevel);
}

void TenZillaScale::motorDown() {
  // Контроль лимитов и перегрузки перед запуском
  int encoderCount = getOpticalCount();
  float currentWeightN = getCurrentWeight();  // N
  int encoderMin = getEncoderMin();
  int encoderMax = getEncoderMax();
  
  // Проверка ограничений по перемещению (отключается при limitsDisabled)
  if (!limitsDisabled) {
    // Блокируем только направление выхода за пределы
    // ВАЖНО: Ноль вверху! Движение вниз УВЕЛИЧИВАЕТ count, движение вверх УМЕНЬШАЕТ count
    // После исправления энкодера: DOWN (direction=-1) -> delta=1 -> count увеличивается ✓
    // - Если count уже максимальный или превысил - блокируем DOWN (нельзя идти ниже максимума)
    // - Если count ниже минимума - разрешаем DOWN (это деблокирующее направление - увеличивает count)
    if (encoderCount >= encoderMax) {
      TenZillaProgram::beepError();
      // Статусное сообщение будет показано в updateLVGL
      return;
    }
    
    // Если ниже минимума - разрешаем DOWN (это деблокирующее направление - увеличивает count)
    // НЕ блокируем DOWN при count < min, так как это деблокирующее направление
  }
  
  // Проверка перегрузки по весу (maxWeight в N) - всегда активна
  float maxWeightN = getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    TenZillaProgram::beepError();
    return;
  }
  // Блокируем запуск при сильном отрицательном весе (сбой датчика)
  if (currentWeightN < getNegativeWeightLimit()) {
    TenZillaProgram::beepError();
    return;
  }
  
  clearEncoderFault();
  encoderFaultCheckStart = millis();
  encoderFaultLastCount = TenZillaEncoder::getCount();
  motorRunning = true;
  motorDirection = -1;
  TenZillaEncoder::setMotorDirection(-1);
  int activeLevel = relayActiveHigh ? HIGH : LOW;
  int inactiveLevel = relayActiveHigh ? LOW : HIGH;
  digitalWrite(RELAY_PIN_1, inactiveLevel);
  digitalWrite(RELAY_PIN_2, activeLevel);
}

void TenZillaScale::motorStop() {
  motorRunning = false;
  motorDirection = 0;
  // ТЕСТОВАЯ КОНФИГУРАЦИЯ: Передаем направление в энкодер (когда нет канала A)
  TenZillaEncoder::setMotorDirection(0);
  
  // Выключаем оба канала реле (стоп) - используем неактивный уровень
  int inactiveLevel = relayActiveHigh ? LOW : HIGH;
  digitalWrite(RELAY_PIN_1, inactiveLevel);
  digitalWrite(RELAY_PIN_2, inactiveLevel);
  
  int currentCount = TenZillaEncoder::getCount();
  opticalCount = currentCount;
  preferences.begin("tenzilla-scale", false);
  preferences.putInt("optical_count", opticalCount);
  preferences.end();
  lastSavedOpticalCount = opticalCount;
  lastOpticalCountSave = millis();
  
  TenZillaDisplay::updateOpticalCount(getOpticalCount());
  TenZillaDisplay::updateMotorStatus(false, 0);
  TenZillaDisplay::forceUpdateScreen();
  // Это предотвращает задержку отображения изменений перемещения после остановки
  TenZillaDisplay::updateOpticalCount(getOpticalCount());
  TenZillaDisplay::updateMotorStatus(false, 0);
  TenZillaDisplay::forceUpdateScreen();
}

bool TenZillaScale::isMotorRunning() {
  return motorRunning;
}

int TenZillaScale::getMotorDirection() {
  return motorDirection; // 1 = вверх, -1 = вниз, 0 = стоп
}


void TenZillaScale::setRelayActiveHigh(bool activeHigh) {
  relayActiveHigh = activeHigh;
  // Если двигатель был запущен, останавливаем его и перезапускаем с новыми настройками
  // НО только если нет перегрузки
  if (motorRunning) {
    int savedDirection = motorDirection;
    motorStop();
    delay(50);
    
    // Проверяем перегрузку перед перезапуском
    float currentWeightN = getCurrentWeight();  // N
    float maxWeightN = getMaxWeight();
    if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
      int inactiveLevel = relayActiveHigh ? LOW : HIGH;
      digitalWrite(RELAY_PIN_1, inactiveLevel);
      digitalWrite(RELAY_PIN_2, inactiveLevel);
      saveConfig();
      return;
    }
    
    if (savedDirection == 1) {
      motorUp();
    } else if (savedDirection == -1) {
      motorDown();
    }
  } else {
    int inactiveLevel = relayActiveHigh ? LOW : HIGH;
    digitalWrite(RELAY_PIN_1, inactiveLevel);
    digitalWrite(RELAY_PIN_2, inactiveLevel);
  }
  saveConfig();
}

bool TenZillaScale::getRelayActiveHigh() {
  return relayActiveHigh;
}

int TenZillaScale::getOpticalCount() {
  return opticalCount;
}

void TenZillaScale::resetOpticalCount() {
  int oldCount = opticalCount;
  opticalCount = 0;
  TenZillaEncoder::resetCount(); // Также сбрасываем счетчик энкодера
  // Сохраняем сброс в NVS
  preferences.begin("tenzilla-scale", false);
  preferences.putInt("optical_count", 0);
  preferences.end();
  lastSavedOpticalCount = 0;
  lastOpticalCountSave = millis();
  (void)oldCount;
}

float TenZillaScale::getMaxWeight() {
  return maxWeight;
}

void TenZillaScale::setMaxWeight(float weight) {
  maxWeight = weight;
}

float TenZillaScale::getNegativeWeightLimit() {
  return negativeWeightLimitN;
}

void TenZillaScale::setNegativeWeightLimit(float limitN) {
  // Ограничиваем только отрицательными значениями (или 0 = отключить проверку не будем, храним как есть)
  negativeWeightLimitN = limitN;
}

void TenZillaScale::setEncoderStepMm(float stepMm) {
  encoderStepMm = stepMm;
  saveConfig();
}

float TenZillaScale::getEncoderStepMm() {
  return encoderStepMm;
}


void TenZillaScale::setEncoderMin(int minValue) {
  // encoderMin всегда 0, не сохраняется
  encoderMin = 0;
}

int TenZillaScale::getEncoderMin() {
  return encoderMin;
}

void TenZillaScale::setEncoderMax(int maxValue) {
  encoderMax = maxValue;
  saveConfig();
}

int TenZillaScale::getEncoderMax() {
  return encoderMax;
}

void TenZillaScale::setEncoderTestOnlyB(bool enable) {
  encoderTestOnlyB = enable;
  TenZillaEncoder::setPulseOnlyMode(enable);
  saveConfig();
}

bool TenZillaScale::getEncoderTestOnlyB() {
  return encoderTestOnlyB;
}

bool TenZillaScale::getEncoderFault() {
  return encoderFault;
}

void TenZillaScale::clearEncoderFault() {
  encoderFault = false;
}

float TenZillaScale::getDisplacement() {
  // Перемещение = количество импульсов * шаг в мм на импульс
  // Применяем инверсию если необходимо
  int count = opticalCount;
  return (float)count * encoderStepMm;
}

// ============================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// ============================================

void TenZillaScale::readConfig() {
  Serial.println("📖 [readConfig] Reading configuration from NVS...");
  Serial.print("📖 [readConfig] Opening preferences namespace 'tenzilla-scale'... ");
  preferences.begin("tenzilla-scale", true);
  Serial.println("OK");
  
  if (preferences.isKey("cal_factor")) {
    calibration_factor = preferences.getFloat("cal_factor", DEFAULT_CALIBRATION_FACTOR);
    calibrated = true;
  }
  
  // Предел перегрузки тензодатчика (N) — сохраняется между перезагрузками
  maxWeight = preferences.getFloat("max_weight", 500.0f);
  // Порог аварийного стопа при отрицательном весе (Н)
  negativeWeightLimitN = preferences.getFloat("negative_weight_limit", -50.0f);
  
  // Загружаем параметры энкодера
  if (preferences.isKey("encoder_step_mm")) {
    encoderStepMm = preferences.getFloat("encoder_step_mm", 0.01f);
  }
  // Корректируем неверные значения (например 200 из-за ошибочного ввода или старого бага)
  if (encoderStepMm < 0.0001f || encoderStepMm > 10.0f) {
    encoderStepMm = 0.01f;
    preferences.end();
    preferences.begin("tenzilla-scale", false);
    preferences.putFloat("encoder_step_mm", encoderStepMm);
    preferences.end();
    preferences.begin("tenzilla-scale", true);
    Serial.println("encoder_step_mm corrected to 0.01 (was out of range)");
  }
  // encoderMin всегда 0, не хранится
  encoderMin = 0;
  
  // encoderMax: 2000 мм по умолчанию (200000 импульсов при шаге 0.01 мм)
  encoderMax = preferences.getInt("encoder_max", 200000);  // 2000 мм / 0.01 мм = 200000 импульсов
  if (preferences.isKey("encoder_test_only_b")) {
    encoderTestOnlyB = preferences.getBool("encoder_test_only_b", false);
    Serial.print("ENC readConfig: encoder_test_only_b=");
    Serial.println(encoderTestOnlyB ? 1 : 0);
  }

  // Загружаем настройки двигателя
  if (preferences.isKey("motor_inverted")) {
  }
  if (preferences.isKey("relay_active_high")) {
    relayActiveHigh = preferences.getBool("relay_active_high", false);
  }
  
  // Загружаем порог шума
  if (preferences.isKey("noise_threshold")) {
    noiseThreshold = preferences.getFloat("noise_threshold", 5.0f);
  }
  
  // Загружаем настройки производительности чтения
  // Сначала пробуем новый ключ, потом старый для обратной совместимости
  bool hasMinInterval = preferences.isKey("min_read_int_ms");
  if (!hasMinInterval) {
    hasMinInterval = preferences.isKey("min_read_interval");
  }
  
  if (hasMinInterval) {
    unsigned long loadedValue = 25;
    
    // Пробуем новый ключ "min_read_int_ms"
    if (preferences.isKey("min_read_int_ms")) {
      int loadedValueInt = preferences.getInt("min_read_int_ms", -1);
      if (loadedValueInt >= 10 && loadedValueInt <= 100) {
        loadedValue = (unsigned long)loadedValueInt;
      } else {
        String loadedValueStr = preferences.getString("min_read_int_ms", "");
        if (loadedValueStr.length() > 0) {
          int parsed = loadedValueStr.toInt();
          if (parsed >= 10 && parsed <= 100) {
            loadedValue = (unsigned long)parsed;
          }
        }
      }
    } else if (preferences.isKey("min_read_interval")) {
      // Старый ключ для обратной совместимости
      int loadedValueInt = preferences.getInt("min_read_interval", -1);
      if (loadedValueInt >= 10 && loadedValueInt <= 100) {
        loadedValue = (unsigned long)loadedValueInt;
      } else {
        loadedValue = preferences.getULong("min_read_interval", 25);
      }
    }
    
    minReadInterval = loadedValue;
    if (minReadInterval < 10) minReadInterval = 10;
    if (minReadInterval > 100) minReadInterval = 100;
  }
  if (preferences.isKey("nau7802_wait_ms")) {
    nau7802WaitMs = preferences.getULong("nau7802_wait_ms", 12);
    if (nau7802WaitMs < 5) nau7802WaitMs = 5;
    if (nau7802WaitMs > 50) nau7802WaitMs = 50;
  }
  if (preferences.isKey("i2c_speed")) {
    i2cSpeed = preferences.getULong("i2c_speed", 100000);
    // Разрешенные значения: 100000, 200000, 300000, 400000 (100kHz, 200kHz, 300kHz, 400kHz)
    // Округляем до ближайшего разрешенного значения
    if (i2cSpeed < 150000) {
      i2cSpeed = 100000;  // 100 kHz
    } else if (i2cSpeed < 250000) {
      i2cSpeed = 200000;  // 200 kHz
    } else if (i2cSpeed < 350000) {
      i2cSpeed = 300000;  // 300 kHz
    } else {
      i2cSpeed = 400000;  // 400 kHz
    }
  }
  
  // Загружаем сохраненное абсолютное перемещение (opticalCount)
  Serial.println("🔍 [readConfig] Checking for optical_count in NVS...");
  bool hasOpticalCount = preferences.isKey("optical_count");
  Serial.print("🔍 [readConfig] optical_count key exists: ");
  Serial.println(hasOpticalCount ? "YES" : "NO");
  
  if (hasOpticalCount) {
    int savedCount = preferences.getInt("optical_count", 0);
    opticalCount = savedCount;
    lastSavedOpticalCount = opticalCount; // Инициализируем для периодического сохранения
    Serial.print("✅ [readConfig] Optical count LOADED from NVS: ");
    Serial.print(opticalCount);
    Serial.print(" (displacement: ");
    Serial.print(opticalCount * encoderStepMm, 2);
    Serial.println(" mm)");
  } else {
    opticalCount = 0; // По умолчанию 0, если не было сохранено
    lastSavedOpticalCount = 0;
    Serial.println("⚠️ [readConfig] Optical count NOT FOUND in NVS, using default: 0");
  }
  
  // Загружаем сохраненные RAW значения (только если калибровка была завершена)
  // ВАЖНО: Делаем это ДО закрытия preferences!
  if (calibrated) {
    if (preferences.isKey("zero_raw")) {
      zeroRaw = preferences.getLong("zero_raw", 0);
    }
    
    if (preferences.isKey("cal_raw")) {
      calibrationRaw = preferences.getLong("cal_raw", 0);
    }
    
    // Если есть сохраненные RAW значения, выводим информацию
    // ВАЖНО: НЕ устанавливаем offset и scale здесь, так как scale еще не инициализирован!
    // Установка произойдет в begin() после инициализации NAU7802
    if (zeroRaw != 0) {
      Serial.println("✅ Calibration loaded:");
      Serial.print("   Zero:");
      Serial.print(zeroRaw);
      Serial.print(", Cal:");
      Serial.print(calibrationRaw);
      Serial.print(", Factor:");
      Serial.println(calibration_factor);
      delay(50); // Задержка для вывода
    }
  } else {
    // Если калибровка не была завершена, не загружаем RAW значения
    zeroRaw = 0;
    calibrationRaw = 0;
  }
  
  // Инициализируем таймер для периодического сохранения
  // Используем 0 для первого сохранения (сохранится сразу при первом изменении)
  lastOpticalCountSave = 0;
  
  preferences.end();
}

void TenZillaScale::saveConfig() {
  preferences.begin("tenzilla-scale", false);
  
  if (calibrated) {
    preferences.putFloat("cal_factor", calibration_factor);
    if (zeroRaw != 0) preferences.putLong("zero_raw", zeroRaw);
    if (calibrationRaw != 0) preferences.putLong("cal_raw", calibrationRaw);
  } else {
    if (zeroRaw != 0) preferences.putLong("zero_raw", zeroRaw);
  }
  
  // Сохраняем параметры энкодера
  preferences.putFloat("encoder_step_mm", encoderStepMm);
  // encoderMin всегда 0, не сохраняется
  preferences.putInt("encoder_max", encoderMax);
  preferences.putBool("encoder_test_only_b", encoderTestOnlyB);

  // Сохраняем настройки двигателя
  preferences.putBool("relay_active_high", relayActiveHigh);
  
  // Сохраняем порог шума
  preferences.putFloat("noise_threshold", noiseThreshold);
  
  // Предел перегрузки тензодатчика (N)
  preferences.putFloat("max_weight", maxWeight);
  preferences.putFloat("negative_weight_limit", negativeWeightLimitN);
  
  // Сохраняем настройки производительности чтения
  // Удаляем старый ключ (если существует) и используем новый ключ для избежания конфликтов типов
  if (preferences.isKey("min_read_interval")) {
    preferences.remove("min_read_interval");
  }
  if (preferences.isKey("min_read_int_ms")) {
    preferences.remove("min_read_int_ms");
  }
  
  // Используем новый ключ "min_read_int_ms" для избежания конфликтов
  if (!preferences.putInt("min_read_int_ms", (int)minReadInterval)) {
    // Fallback на String, если putInt не сработал
    String valueStr = String((int)minReadInterval);
    preferences.putString("min_read_int_ms", valueStr);
  }
  
  preferences.putULong("nau7802_wait_ms", nau7802WaitMs);
  preferences.putULong("i2c_speed", i2cSpeed);
  
  // Сохраняем абсолютное перемещение (opticalCount)
  preferences.putInt("optical_count", opticalCount);
  
  // ВАЖНО: limitsDisabled НЕ сохраняется - это временное состояние
  // При перезагрузке ограничения всегда включены
  
  preferences.end();
}

// ============================================
// УПРАВЛЕНИЕ ОГРАНИЧЕНИЯМИ ПЕРЕМЕЩЕНИЯ
// ============================================

bool TenZillaScale::disableLimits(int pinCode) {
  // Пин-код для отключения ограничений: 1861
  const int REQUIRED_PIN_CODE = 1861;
  
  if (pinCode == REQUIRED_PIN_CODE) {
    limitsDisabled = true;
    limitsDisabledTime = millis();  // Сохраняем время отключения
    Serial.println("⚠️ WARNING: Displacement limits DISABLED (temporary, not saved, 10 min timeout)");
    return true;
  }
  
  Serial.println("❌ Invalid PIN code for disabling limits");
  return false;
}

void TenZillaScale::enableLimits() {
  limitsDisabled = false;
  limitsDisabledTime = 0;  // Сбрасываем время
  Serial.println("✅ Displacement limits ENABLED");
}

bool TenZillaScale::areLimitsDisabled() {
  return limitsDisabled;
}