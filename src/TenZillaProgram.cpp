#include "TenZillaProgram.h"
#include "TenZillaPins.h"
#include "TenZillaScale.h"
#include "TenZillaDisplay.h"
#include "TenZillaWeb.h"
#include "TenZillaMeasurements.h"
#include "TenZillaConfig.h"
#include "TenZillaTelegram.h"
#include <Preferences.h>

// Инициализация статических переменных
int TenZillaProgram::programType = 0;
bool TenZillaProgram::programRunning = false;
unsigned long TenZillaProgram::programStartTime = 0;
unsigned long TenZillaProgram::lastProgramDurationMs = 0;
bool TenZillaProgram::programCompletedSuccessfully = false;
bool TenZillaProgram::programStoppedManually = false;

bool TenZillaProgram::compressionWaiting = false;
bool TenZillaProgram::compressionMovingDown = false;
bool TenZillaProgram::compressionMeasuring = false;
bool TenZillaProgram::compressionHolding = false;
float TenZillaProgram::compressionStartWeight = 0.0f;
float TenZillaProgram::workingDisplacement = 0.0f;
float TenZillaProgram::absoluteDisplacementStart = 0.0f;

bool TenZillaProgram::breakWaiting = false;
bool TenZillaProgram::breakMovingDown = false;
bool TenZillaProgram::breakMeasuring = false;
float TenZillaProgram::breakMaxWeight = 0.0f;
bool TenZillaProgram::breakMaxReached = false;
float TenZillaProgram::breakAbsoluteDisplacementStart = 0.0f;

float TenZillaProgram::compressionStartThreshold = 1.0f;   // По умолчанию 1 Н
float TenZillaProgram::compressionTargetDisplacement = 5.0f;  // По умолчанию 5 мм
float TenZillaProgram::compressionUnloadRetractMm = 5.0f;    // По умолчанию 5 мм после разгрузки до 0
float TenZillaProgram::breakDropThreshold = 20.0f;        // По умолчанию 20% падение

bool TenZillaProgram::compressionUnloading = false;
int TenZillaProgram::compressionRetractTargetEncoder = -1;
unsigned long TenZillaProgram::compressionHoldStartTime = 0;

void TenZillaProgram::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("✅ Buzzer initialized on GPIO " + String(BUZZER_PIN));
  
  // Загружаем настройки программы из NVS
  Preferences preferences;
  preferences.begin("tenzilla-program", true);
  
  if (preferences.isKey("comp_start_thresh")) {
    compressionStartThreshold = preferences.getFloat("comp_start_thresh", 1.0f);
  }
  if (preferences.isKey("comp_target_disp")) {
    compressionTargetDisplacement = preferences.getFloat("comp_target_disp", 5.0f);
  }
  if (preferences.isKey("comp_unload_retract_mm")) {
    compressionUnloadRetractMm = preferences.getFloat("comp_unload_retract_mm", 5.0f);
  }
  if (preferences.isKey("break_drop_thresh")) {
    breakDropThreshold = preferences.getFloat("break_drop_thresh", 20.0f);
  }
  
  preferences.end();
  Serial.println("✅ Program settings loaded from NVS");
}

void TenZillaProgram::beep(int durationMs, int pauseMs) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
  if (pauseMs > 0) {
    delay(pauseMs);
  }
  // НЕ используем delay() в основном цикле - вызываем только из startCompressionProgram/startBreakProgram
  // где delay допустим, так как это происходит один раз при старте
}

void TenZillaProgram::beepLong() {
  beep(500); // 500мс протяжный гудок
}

void TenZillaProgram::beepShort() {
  beep(100); // 100мс короткий гудок
}

void TenZillaProgram::beepError() {
  // 5 коротких гудков
  for (int i = 0; i < 5; i++) {
    beep(100, 50); // 100мс гудок, 50мс пауза
  }
}

void TenZillaProgram::startCompressionProgram() {
  // Блокируем запуск программы при отключенных ограничениях
  if (TenZillaScale::areLimitsDisabled()) {
    beepError();
    return;
  }
  
  float currentWeightN = TenZillaScale::getCachedWeight();
  float maxWeightN = TenZillaScale::getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    beepError();
    return;
  }
  TenZillaScale::motorStop();
  TenZillaDisplay::resetMainScreenMax();
  programCompletedSuccessfully = false;
  programStoppedManually = false;
  programType = 1;
  programRunning = true;
  programStartTime = millis();
  compressionWaiting = true;
  compressionMovingDown = false;
  compressionMeasuring = false;
  compressionHolding = false;
  compressionUnloading = false;
  compressionRetractTargetEncoder = -1;
  workingDisplacement = 0.0f;
  absoluteDisplacementStart = TenZillaScale::getDisplacement();
  compressionStartWeight = 0.0f;
  beepLong();
  // НЕ используем delay() - гудки будут в update(), а фаза ожидания начнется сразу
}

void TenZillaProgram::startBreakProgram() {
  // Блокируем запуск программы при отключенных ограничениях
  if (TenZillaScale::areLimitsDisabled()) {
    beepError();
    return;
  }
  
  float currentWeightN = TenZillaScale::getCachedWeight();
  float maxWeightN = TenZillaScale::getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    beepError();
    return;
  }
  TenZillaScale::motorStop();
  TenZillaDisplay::resetBreakScreenMax();
  programCompletedSuccessfully = false;
  programStoppedManually = false;
  programType = 2;
  programRunning = true;
  programStartTime = millis();
  breakWaiting = true;
  breakMovingDown = false;
  breakMeasuring = false;
  breakMaxWeight = 0.0f;
  breakMaxReached = false;
  workingDisplacement = 0.0f;
  absoluteDisplacementStart = TenZillaScale::getDisplacement();
  breakAbsoluteDisplacementStart = 0.0f;
  beepLong();
  // НЕ используем delay() - гудки будут в update(), а фаза ожидания начнется сразу
}

void TenZillaProgram::stopProgram(StopReason reason) {
  TenZillaScale::motorStop();
  if (!programCompletedSuccessfully && programRunning) {
    programStoppedManually = (reason == STOP_REASON_MANUAL || reason == STOP_REASON_WEB);
    float w = (programType == 2) ? breakMaxWeight : TenZillaDisplay::getMainScreenMax();
    TenZillaMeasurements::record((uint8_t)programType, MEAS_OUTCOME_STOPPED, w);
    {
      TenZillaSettings s = TenZillaConfig::get();
      if (s.tgEnabled && s.tgNotifyStopped) {
        String msg;
        if (reason == STOP_REASON_ENCODER_FAULT) {
          msg = "TenZilla: программа остановлена (ошибка энкодера). Вес: ";
        } else if (reason == STOP_REASON_WEB) {
          msg = "TenZilla: программа остановлена из веб-интерфейса. Вес: ";
        } else {
          msg = "TenZilla: программа остановлена пользователем. Вес: ";
        }
        msg += String(w, 1);
        msg += " N";
        TenZillaTelegram::send(msg);
      }
    }
  }
  // Сбрасываем все флаги состояния программы
  // НЕ сбрасываем programCompletedSuccessfully здесь - он сбросится при следующем запуске
  // НЕ сбрасываем workingDisplacement - сохраняем до следующего запуска программы
  lastProgramDurationMs = (programStartTime > 0) ? (millis() - programStartTime) : 0;
  programRunning = false;
  programType = 0;
  compressionWaiting = false;
  compressionMovingDown = false;
  compressionMeasuring = false;
  compressionHolding = false;
  compressionUnloading = false;
  compressionRetractTargetEncoder = -1;
  breakWaiting = false;
  breakMovingDown = false;
  breakMeasuring = false;
}

bool TenZillaProgram::isRunning() {
  return programRunning;
}

int TenZillaProgram::getProgramType() {
  return programType;
}

float TenZillaProgram::getAbsoluteDisplacement() {
  return TenZillaScale::getDisplacement();
}

float TenZillaProgram::getWorkingDisplacement() {
  return workingDisplacement;
}

unsigned long TenZillaProgram::getProgramStartTimeMs() {
  return programStartTime;
}

unsigned long TenZillaProgram::getLastProgramDurationMs() {
  return lastProgramDurationMs;
}

String TenZillaProgram::getStatusMessage() {
  if (TenZillaScale::getEncoderFault()) {
    return "ENCODER FAULT";
  }
  // Если программа завершена успешно, показываем сообщение о завершении
  if (programCompletedSuccessfully) {
    return "COMPLETED";
  }
  if (programStoppedManually && !programRunning) {
    return "PROGRAM STOPPED";
  }
  
  // Если программа запущена, показываем текущий статус
  if (!programRunning) {
    return "";
  }
  
  if (programType == 1) {
    if (compressionWaiting) {
      unsigned long elapsed = millis() - programStartTime;
      unsigned long remaining = (elapsed < 5000) ? (5000 - elapsed) : 0;
      return "Waiting: " + String(remaining / 1000) + "s";
    } else if (compressionMovingDown) {
      return "Moving down...";
    } else if (compressionMeasuring) {
      return "Measuring...";
    } else if (compressionHolding) {
      return "Holding...";
    } else if (compressionUnloading) {
      return "Unloading...";
    }
  } else if (programType == 2) {
    if (breakWaiting) {
      unsigned long elapsed = millis() - programStartTime;
      unsigned long remaining = (elapsed < 5000) ? (5000 - elapsed) : 0;
      return "Waiting: " + String(remaining / 1000) + "s";
    } else if (breakMovingDown) {
      return "Moving down...";
    } else if (breakMeasuring) {
      return "Measuring...";
    }
  }
  
  return "";
}

bool TenZillaProgram::isCompletedSuccessfully() {
  return programCompletedSuccessfully;
}

bool TenZillaProgram::isStoppedManually() {
  return programStoppedManually;
}

void TenZillaProgram::setCompressionStartThreshold(float thresholdN) {
  compressionStartThreshold = thresholdN;
  Preferences preferences;
  preferences.begin("tenzilla-program", false);
  preferences.putFloat("comp_start_thresh", thresholdN);
  preferences.end();
}

float TenZillaProgram::getCompressionStartThreshold() {
  return compressionStartThreshold;
}

void TenZillaProgram::setCompressionTargetDisplacement(float targetMm) {
  compressionTargetDisplacement = targetMm;
  Preferences preferences;
  preferences.begin("tenzilla-program", false);
  preferences.putFloat("comp_target_disp", targetMm);
  preferences.end();
}

float TenZillaProgram::getCompressionTargetDisplacement() {
  return compressionTargetDisplacement;
}

void TenZillaProgram::setCompressionUnloadRetractMm(float mm) {
  if (mm < 0.0f) mm = 0.0f;
  compressionUnloadRetractMm = mm;
  Preferences preferences;
  preferences.begin("tenzilla-program", false);
  preferences.putFloat("comp_unload_retract_mm", compressionUnloadRetractMm);
  preferences.end();
}

float TenZillaProgram::getCompressionUnloadRetractMm() {
  return compressionUnloadRetractMm;
}

void TenZillaProgram::setBreakDropThreshold(float thresholdPercent) {
  breakDropThreshold = thresholdPercent;
  Preferences preferences;
  preferences.begin("tenzilla-program", false);
  preferences.putFloat("break_drop_thresh", thresholdPercent);
  preferences.end();
}

float TenZillaProgram::getBreakDropThreshold() {
  return breakDropThreshold;
}

void TenZillaProgram::update() {
  if (!programRunning) return;
  
  // Обработка веб-запросов для улучшения отзывчивости
  TenZillaWeb::handleClient();
  
  unsigned long now = millis();
  float currentWeightN = TenZillaScale::getCachedWeight();
  float absoluteDisplacement = TenZillaScale::getDisplacement();
  int encoderCount = TenZillaScale::getOpticalCount();
  int encoderMin = TenZillaScale::getEncoderMin();
  int encoderMax = TenZillaScale::getEncoderMax();
  
  bool limitExceeded = false;
  // Проверка ограничений по перемещению (отключается при limitsDisabled)
  if (!TenZillaScale::areLimitsDisabled()) {
    if (encoderCount < encoderMin || encoderCount > encoderMax) {
      limitExceeded = true;
    }
  }
  // Проверка перегрузки по весу - всегда активна
  float maxWeightN = TenZillaScale::getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    limitExceeded = true;
  }
  if (limitExceeded) {
    float w = (programType == 2) ? breakMaxWeight : TenZillaDisplay::getMainScreenMax();
    TenZillaMeasurements::record((uint8_t)programType, MEAS_OUTCOME_ERROR, w);
    {
      TenZillaSettings s = TenZillaConfig::get();
      if (s.tgEnabled && s.tgNotifyOverload) {
        String msg = "TenZilla: ошибка (перегрузка/лимиты). Вес: ";
        msg += String(w, 1);
        msg += " N";
        TenZillaTelegram::send(msg);
      }
    }
    TenZillaScale::motorStop();
    beepError();
    lastProgramDurationMs = (programStartTime > 0) ? (millis() - programStartTime) : 0;
    programRunning = false;
    programType = 0;
    compressionWaiting = false;
    compressionMovingDown = false;
    compressionMeasuring = false;
    compressionUnloading = false;
    compressionRetractTargetEncoder = -1;
    breakWaiting = false;
    breakMovingDown = false;
    breakMeasuring = false;
    return;
  }
  
  // Программа СЖАТИЕ
  if (programType == 1) {
    // Фаза 1: Ожидание 5 секунд
    if (compressionWaiting) {
      unsigned long elapsed = now - programStartTime;
      if (elapsed >= 5000) {
        compressionMovingDown = true;
        compressionWaiting = false;
        TenZillaScale::motorDown();
      }
      return;
    }
    
    // Фаза 2: Движение вниз до начала роста значения на тензодатчике
    if (compressionMovingDown) {
      if (currentWeightN >= compressionStartThreshold) {
        // Значение начало расти - переходим к измерению
        compressionMovingDown = false;
        compressionMeasuring = true;
        compressionStartWeight = currentWeightN;
        absoluteDisplacementStart = absoluteDisplacement; // Сохраняем абсолютное перемещение в момент начала роста
        workingDisplacement = 0.0f;
      }
      return;
    }
    
    // Фаза 3: Измерение рабочего перемещения
    if (compressionMeasuring) {
      // Накопление рабочего перемещения с момента начала роста
      // Рабочее перемещение = текущее абсолютное - абсолютное в момент начала роста
      workingDisplacement = absoluteDisplacement - absoluteDisplacementStart;
      
      // Проверка достижения целевого перемещения — переходим к удержанию для стабилизации веса
      if (workingDisplacement >= compressionTargetDisplacement) {
        compressionMeasuring = false;
        compressionHolding = true;
        compressionHoldStartTime = now;
        TenZillaScale::motorStop();  // Останавливаем движение, держим позицию
      }
    }
    
    // Фаза 3.5: Удержание позиции перед разгрузкой (для точной установки веса)
    if (compressionHolding) {
      const unsigned long HOLD_DURATION_MS = 3000;
      if (now - compressionHoldStartTime >= HOLD_DURATION_MS) {
        compressionHolding = false;
        compressionUnloading = true;
        compressionRetractTargetEncoder = -1;  // Установится при достижении веса 0
        TenZillaScale::motorUp();  // Разгружаем весы (движение вверх, счётчик уменьшается)
      }
    }
    
    // Фаза 4: Разгрузка до 0 Н, затем откат на compressionUnloadRetractMm мм (не ниже нуля энкодера)
    if (compressionUnloading) {
      const float unloadWeightThreshold = 0.5f;  // Считаем «ноль» при весе <= 0.5 Н
      if (currentWeightN <= unloadWeightThreshold && compressionRetractTargetEncoder < 0) {
        float stepMm = TenZillaScale::getEncoderStepMm();
        int pulses = (stepMm > 0.0f) ? (int)(compressionUnloadRetractMm / stepMm) : 0;
        compressionRetractTargetEncoder = encoderCount - pulses;
        if (compressionRetractTargetEncoder < encoderMin) {
          compressionRetractTargetEncoder = encoderMin;
        }
      }
      // Завершение: достигли целевой позиции отката или уже на нуле энкодера при разгруженных весах
      bool reachedTarget = (compressionRetractTargetEncoder >= 0 && encoderCount <= compressionRetractTargetEncoder);
      bool atZeroUnloaded = (encoderCount <= encoderMin && currentWeightN <= unloadWeightThreshold);
      if (reachedTarget || atZeroUnloaded) {
        float w = TenZillaDisplay::getMainScreenMax();
        TenZillaMeasurements::record(MEAS_TYPE_COMPRESSION, MEAS_OUTCOME_COMPLETED, w);
        {
          TenZillaSettings s = TenZillaConfig::get();
          if (s.tgEnabled && s.tgNotifyProgramResults) {
            String msg = "TenZilla: СЖАТИЕ завершено. Вес: ";
            msg += String(w, 1);
            msg += " N";
            TenZillaTelegram::send(msg);
          }
        }
        TenZillaScale::motorStop();
        beepLong();
        programCompletedSuccessfully = true;
        stopProgram();
      }
    }
  }
  
  if (programType == 2) {
    // Фаза 1: Ожидание 5 секунд
    if (breakWaiting) {
      if (now - programStartTime >= 5000) {
        breakWaiting = false;
        breakMovingDown = true;
        TenZillaScale::motorDown();
      }
      return;
    }
    
    // Фаза 2: Движение вниз до начала роста
    if (breakMovingDown) {
      if (currentWeightN >= compressionStartThreshold) {
        breakMovingDown = false;
        breakMeasuring = true;
        breakMaxWeight = currentWeightN;
        breakMaxReached = false;
        breakAbsoluteDisplacementStart = absoluteDisplacement; // Сохраняем для рабочего перемещения
        workingDisplacement = 0.0f;
      }
      return;
    }
    
    // Фаза 3: Измерение до падения
    if (breakMeasuring) {
      // Накопление рабочего перемещения
      workingDisplacement = absoluteDisplacement - breakAbsoluteDisplacementStart;
      
      // Отслеживание максимума
      if (currentWeightN > breakMaxWeight) {
        breakMaxWeight = currentWeightN;
      }
      
      // Проверка достижения максимума
      if (!breakMaxReached && currentWeightN >= breakMaxWeight * 0.95f) {
        breakMaxReached = true;
      }
      
      // Проверка резкого падения после максимума
      if (breakMaxReached) {
        float dropPercent = ((breakMaxWeight - currentWeightN) / breakMaxWeight) * 100.0f;
        if (dropPercent >= breakDropThreshold) {
          TenZillaMeasurements::record(MEAS_TYPE_BREAK, MEAS_OUTCOME_COMPLETED, breakMaxWeight);
          {
            TenZillaSettings s = TenZillaConfig::get();
            if (s.tgEnabled && s.tgNotifyProgramResults) {
              String msg = "TenZilla: РАЗРЫВ завершён. Макс: ";
              msg += String(breakMaxWeight, 1);
              msg += " N";
              TenZillaTelegram::send(msg);
            }
          }
          TenZillaScale::motorStop();
          beepLong();
          programCompletedSuccessfully = true;
          stopProgram();
        }
      }
    }
  }
}



