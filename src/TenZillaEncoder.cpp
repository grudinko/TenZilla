#include "TenZillaEncoder.h"

// Инициализация статических переменных
int TenZillaEncoder::encoderPinA = -1;
int TenZillaEncoder::encoderPinB = -1;
int TenZillaEncoder::encoderPinSW = -1;
volatile int TenZillaEncoder::encoderCount = 0;
volatile int TenZillaEncoder::lastEncoded = 0;
int TenZillaEncoder::lastMSB = 0;
int TenZillaEncoder::lastLSB = 0;

bool TenZillaEncoder::buttonPressed = false;
bool TenZillaEncoder::buttonClicked = false;
unsigned long TenZillaEncoder::lastButtonTime = 0;
bool TenZillaEncoder::lastButtonState = HIGH;

// Защита от дребезга и помех
volatile unsigned long TenZillaEncoder::lastISRTime = 0;
volatile int TenZillaEncoder::lastValidState = 0;
bool TenZillaEncoder::inverted = false;
unsigned long TenZillaEncoder::debounceUs = 1;
bool TenZillaEncoder::invertA = false;
bool TenZillaEncoder::invertB = false;
bool TenZillaEncoder::hallMode = false;
bool TenZillaEncoder::pulseDirMode = false;
bool TenZillaEncoder::pulseOnlyMode = false;
volatile int TenZillaEncoder::lastPulseDirA = -1;
volatile int TenZillaEncoder::lastPulseDirB = -1;  // ТЕСТОВАЯ КОНФИГУРАЦИЯ: для pulse-only используем B
volatile int TenZillaEncoder::motorDirectionForEncoder = 0;  // ТЕСТОВАЯ КОНФИГУРАЦИЯ

// Для оптического энкодера (аналоговые сигналы)
bool TenZillaEncoder::opticalMode = false;
int TenZillaEncoder::analogThresholdA = 200;
int TenZillaEncoder::analogThresholdB = 200;
bool TenZillaEncoder::autoThreshold = false;
int TenZillaEncoder::analogMinA = 4095;
int TenZillaEncoder::analogMaxA = 0;
int TenZillaEncoder::analogMinB = 4095;
int TenZillaEncoder::analogMaxB = 0;
unsigned long TenZillaEncoder::lastAnalogReadTime = 0;

void TenZillaEncoder::begin(int pinA, int pinB, int pinSW) {
  encoderPinA = pinA;
  encoderPinB = pinB;
  encoderPinSW = pinSW;
  
  encoderCount = 0;
  lastEncoded = 0;
  lastMSB = 0;
  lastLSB = 0;
  lastISRTime = 0;
  lastValidState = 0;
  lastPulseDirA = -1;
  lastPulseDirB = -1;
  inverted = false;
  invertA = false;
  invertB = false;
  pulseDirMode = false;
  pulseOnlyMode = false;
  hallMode = false;
  debounceUs = hallMode ? 100UL : 1UL;
  
  delay(10);
  // Настройка пинов как входы с подтяжкой
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  delay(10);
  
  // Настройка кнопки (если подключена)
  if (encoderPinSW >= 0) {
    pinMode(encoderPinSW, INPUT_PULLUP);
    lastButtonState = digitalRead(encoderPinSW);
    delay(10);
  }
  
  // Чтение начального состояния
  delay(10);
  // Читаем несколько раз для стабильности
  lastMSB = digitalRead(encoderPinA);
  lastLSB = digitalRead(encoderPinB);
  delay(1);
  int msb2 = digitalRead(encoderPinA);
  int lsb2 = digitalRead(encoderPinB);
  delay(1);
  int msb3 = digitalRead(encoderPinA);
  int lsb3 = digitalRead(encoderPinB);
  
  if (msb2 == msb3 && lsb2 == lsb3) {
    lastMSB = msb3;
    lastLSB = lsb3;
  }
  if (invertA) lastMSB ^= 1;
  if (invertB) lastLSB ^= 1;
  lastEncoded = (lastMSB << 1) | lastLSB;
  lastValidState = lastEncoded;
  lastPulseDirA = lastMSB;
  
  delay(10);
  
  // Настройка прерываний для обоих пинов (по изменению)
  delay(50); // Увеличена задержка перед attachInterrupt
  
  // Проверяем, что пины поддерживают прерывания
  int pinAInterrupt = digitalPinToInterrupt(encoderPinA);
  int pinBInterrupt = digitalPinToInterrupt(encoderPinB);
  
  delay(50);
  
  // Прикрепляем прерывания
  // ВАЖНО: На ESP32-S3 все GPIO поддерживают прерывания
  // Используем номер пина напрямую, так как digitalPinToInterrupt может вернуть -1
  delay(10);
  attachInterrupt(encoderPinA, encoderISR, CHANGE);
  delay(50);
  
  delay(10);
  attachInterrupt(encoderPinB, encoderISR, CHANGE);
  delay(50);
  
  delay(50); // Увеличена задержка после attachInterrupt
}

// Обработчик прерывания для энкодера
// Квадратура 90° или режим «импульс + направление» (Hall)
void IRAM_ATTR TenZillaEncoder::encoderISR() {
  unsigned long currentTime = micros();
  if (currentTime - lastISRTime < debounceUs) {
    return;
  }
  lastISRTime = currentTime;
  
  int MSB = digitalRead(encoderPinA);
  int LSB = digitalRead(encoderPinB);
  if (invertA) MSB ^= 1;
  if (invertB) LSB ^= 1;
  int encoded = (MSB << 1) | LSB;
  
  if (pulseDirMode || pulseOnlyMode) {
    if (pulseOnlyMode) {
      // ТЕСТОВАЯ КОНФИГУРАЦИЯ: Используем канал B для импульсов (данные меняются на B)
      if (lastPulseDirB < 0) {
        lastPulseDirB = LSB;
        return;
      }
      if (LSB == lastPulseDirB) {
        return;
      }
      lastPulseDirB = LSB;
      // Используем направление мотора для определения знака
      // ВАЖНО: Ноль вверху! Движение вниз УВЕЛИЧИВАЕТ count, движение вверх УМЕНЬШАЕТ count
      // Поэтому инвертируем знак: UP (1) -> delta = -1, DOWN (-1) -> delta = 1
      int delta;
      if (motorDirectionForEncoder != 0) {
        delta = -motorDirectionForEncoder;  // ИНВЕРТИРУЕМ: UP (1) -> -1, DOWN (-1) -> 1
      } else {
        // Если мотор не работает, используем стандартную логику
        delta = inverted ? -1 : 1;
      }
      encoderCount += delta;
      return;
    } else {
      // pulseDirMode: A = импульсы, B = направление
      if (lastPulseDirA < 0) {
        lastPulseDirA = MSB;
        return;
      }
      if (MSB == lastPulseDirA) {
        return;
      }
      lastPulseDirA = MSB;
      int delta = (LSB ? 1 : -1);
      if (inverted) delta = -delta;
      encoderCount += delta;
      return;
    }
  }
  
  if (encoded == lastEncoded) {
    return;
  }
  
  int sum = (lastEncoded << 2) | encoded;
  switch (sum) {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
      encoderCount += inverted ? -1 : 1;
      lastEncoded = encoded;
      break;
    case 0b0010:
    case 0b1011:
    case 0b1101:
    case 0b0100:
      encoderCount += inverted ? 1 : -1;
      lastEncoded = encoded;
      break;
    default:
      lastEncoded = encoded;
      break;
  }
}

void TenZillaEncoder::update() {
  // Если включен режим оптического энкодера, читаем аналоговые значения
  if (opticalMode) {
    unsigned long now = millis();
    
    // Читаем аналоговые значения с высокой частотой (каждые 1-2 мс)
    if (now - lastAnalogReadTime >= 2) {
      lastAnalogReadTime = now;
      
      // Читаем аналоговые значения
      int analogA = analogRead(encoderPinA);
      int analogB = analogRead(encoderPinB);
      
      // Обновляем min/max для автоматического определения порогов
      if (autoThreshold) {
        // Усиленная фильтрация выбросов
        // Ограничиваем максимальный span для предотвращения расширения диапазона
        const int MAX_SPAN = 200;  // Максимальный допустимый диапазон
        
        // Обновляем min только если:
        // 1. Это первая инициализация (analogMinA == 4095)
        // 2. ИЛИ новое значение близко к текущему min (в пределах 30 единиц)
        // 3. ИЛИ текущий span меньше MAX_SPAN
        if (analogA < analogMinA) {
          int currentSpan = analogMaxA - analogMinA;
          if (analogMinA == 4095 || 
              abs(analogA - analogMinA) < 30 || 
              (currentSpan < MAX_SPAN && analogA > analogMinA - 30)) {
            int newSpan = analogMaxA - analogA;
            if (newSpan <= MAX_SPAN || analogMinA == 4095) {
              analogMinA = analogA;
            }
          }
        }
        
        // Обновляем max только если:
        // 1. Это первая инициализация (analogMaxA == 0)
        // 2. ИЛИ новое значение близко к текущему max (в пределах 30 единиц)
        // 3. ИЛИ текущий span меньше MAX_SPAN
        if (analogA > analogMaxA) {
          int currentSpan = analogMaxA - analogMinA;
          if (analogMaxA == 0 || 
              abs(analogA - analogMaxA) < 30 || 
              (currentSpan < MAX_SPAN && analogA < analogMaxA + 30)) {
            int newSpan = analogA - analogMinA;
            if (newSpan <= MAX_SPAN || analogMaxA == 0) {
              analogMaxA = analogA;
            }
          }
        }
        
        // То же самое для канала B
        if (analogB < analogMinB) {
          int currentSpan = analogMaxB - analogMinB;
          if (analogMinB == 4095 || 
              abs(analogB - analogMinB) < 30 || 
              (currentSpan < MAX_SPAN && analogB > analogMinB - 30)) {
            int newSpan = analogMaxB - analogB;
            if (newSpan <= MAX_SPAN || analogMinB == 4095) {
              analogMinB = analogB;
            }
          }
        }
        if (analogB > analogMaxB) {
          int currentSpan = analogMaxB - analogMinB;
          if (analogMaxB == 0 || 
              abs(analogB - analogMaxB) < 30 || 
              (currentSpan < MAX_SPAN && analogB < analogMaxB + 30)) {
            int newSpan = analogB - analogMinB;
            if (newSpan <= MAX_SPAN || analogMaxB == 0) {
              analogMaxB = analogB;
            }
          }
        }
        
        // Обновляем пороги каждые 1000 мс (1 секунда) для стабильности
        static unsigned long lastThresholdUpdate = 0;
        static int lastThresholdA = -1;
        static int lastThresholdB = -1;
        static unsigned long learningStartTime = 0;
        
        // Первые 5 секунд - режим обучения (быстрое обновление)
        if (learningStartTime == 0) {
          learningStartTime = now;
        }
        bool learningMode = (now - learningStartTime < 5000);
        
        unsigned long updateInterval = learningMode ? 200 : 1000;  // 200 мс в режиме обучения, 1000 мс после
        
        if (now - lastThresholdUpdate >= updateInterval) {
          int newThresholdA = (analogMinA + analogMaxA) / 2;
          int newThresholdB = (analogMinB + analogMaxB) / 2;
          
          // Обновляем только если порог изменился более чем на 20 единиц (или в режиме обучения - 10)
          int threshold = learningMode ? 10 : 20;
          if (abs(newThresholdA - lastThresholdA) > threshold || 
              abs(newThresholdB - lastThresholdB) > threshold ||
              lastThresholdA == -1) {
            updateAnalogThresholds();
            lastThresholdA = analogThresholdA;
            lastThresholdB = analogThresholdB;
          }
          lastThresholdUpdate = now;
        }
      }
      
      int MSB = readAnalogDigital(encoderPinA, analogThresholdA);
      int LSB = readAnalogDigital(encoderPinB, analogThresholdB);
      if (invertA) MSB ^= 1;
      if (invertB) LSB ^= 1;
      int encoded = (MSB << 1) | LSB;
      
      if (encoded != lastEncoded) {
        unsigned long currentTime = micros();
        if (currentTime - lastISRTime >= debounceUs) {
          lastISRTime = currentTime;
          int sum = (lastEncoded << 2) | encoded;
          switch (sum) {
            case 0b0001:
            case 0b0111:
            case 0b1110:
            case 0b1000:
              encoderCount += inverted ? -1 : 1;
              lastEncoded = encoded;
              break;
            case 0b0010:
            case 0b1011:
            case 0b1101:
            case 0b0100:
              encoderCount += inverted ? 1 : -1;
              lastEncoded = encoded;
              break;
            default:
              lastEncoded = encoded;
              break;
          }
        }
      }
    }
  }

  // Обработка кнопки (если подключена)
  if (encoderPinSW >= 0) {
    bool currentState = digitalRead(encoderPinSW);
    unsigned long now = millis();
    
    // Обнаружение нажатия (HIGH -> LOW для INPUT_PULLUP)
    if (currentState == LOW && lastButtonState == HIGH) {
      // Антидребезг: проверяем через 50мс
      if (now - lastButtonTime > 50) {
        buttonPressed = true;
        buttonClicked = true;
        lastButtonTime = now;
      }
    }
    
    // Обнаружение отпускания
    if (currentState == HIGH && lastButtonState == LOW) {
      buttonPressed = false;
      lastButtonTime = now;
    }
    
    lastButtonState = currentState;
  }
}

int TenZillaEncoder::getCount() {
  return encoderCount;
}

void TenZillaEncoder::setCount(int count) {
  encoderCount = count;
}

void TenZillaEncoder::resetCount() {
  encoderCount = 0;
}

int TenZillaEncoder::getDelta() {
  // Возвращает изменение и сбрасывает его (если нужно)
  // Для простоты возвращаем текущий счетчик
  // В будущем можно добавить логику для отслеживания дельты
  return encoderCount;
}

bool TenZillaEncoder::isButtonPressed() {
  return buttonPressed;
}

bool TenZillaEncoder::wasButtonClicked() {
  if (buttonClicked) {
    buttonClicked = false; // Сбрасываем флаг после чтения
    return true;
  }
  return false;
}

void TenZillaEncoder::setInverted(bool inv) {
  inverted = inv;
}

bool TenZillaEncoder::isInverted() {
  return inverted;
}

void TenZillaEncoder::setHallMode(bool enable) {
  hallMode = enable;
  debounceUs = enable ? 100UL : 1UL;
}

bool TenZillaEncoder::isHallMode() {
  return hallMode;
}

void TenZillaEncoder::setDebounceUs(unsigned long us) {
  debounceUs = us;
}

void TenZillaEncoder::setInvertA(bool inv) {
  invertA = inv;
}

void TenZillaEncoder::setInvertB(bool inv) {
  invertB = inv;
}

void TenZillaEncoder::setPulseDirMode(bool enable) {
  pulseDirMode = enable;
  if (enable) pulseOnlyMode = false;
  lastPulseDirA = -1;
}

bool TenZillaEncoder::isPulseDirMode() {
  return pulseDirMode;
}

void TenZillaEncoder::setPulseOnlyMode(bool enable) {
  pulseOnlyMode = enable;
  if (enable) {
    pulseDirMode = false;
    lastPulseDirA = -1;
    // Sync to current B state so we count on next edge (no "skip first edge")
    int lsb = digitalRead(encoderPinB);
    if (invertB) lsb ^= 1;
    lastPulseDirB = lsb;
  } else {
    lastPulseDirA = -1;
    lastPulseDirB = -1;
    int msb = digitalRead(encoderPinA);
    int lsb = digitalRead(encoderPinB);
    if (invertA) msb ^= 1;
    if (invertB) lsb ^= 1;
    lastMSB = msb;
    lastLSB = lsb;
    lastEncoded = (msb << 1) | lsb;
  }
}

bool TenZillaEncoder::isPulseOnlyMode() {
  return pulseOnlyMode;
}

// ТЕСТОВАЯ КОНФИГУРАЦИЯ: Установка направления мотора для энкодера
// Используется когда нет канала A - направление определяется по нажатой кнопке (вверх/вниз)
void TenZillaEncoder::setMotorDirection(int direction) {
  motorDirectionForEncoder = direction;  // 1 = вверх, -1 = вниз, 0 = стоп
}

void TenZillaEncoder::setOpticalMode(bool enabled) {
  opticalMode = enabled;
  
  if (enabled) {
    // Безопасно отключаем прерывания (если они были прикреплены)
    // Используем try-catch подход через проверку валидности пинов
    if (encoderPinA >= 0 && encoderPinA < 48) {  // ESP32-S3 имеет GPIO 0-48
      noInterrupts();  // Отключаем все прерывания временно
      detachInterrupt(encoderPinA);
      interrupts();    // Включаем обратно
    }
    if (encoderPinB >= 0 && encoderPinB < 48) {
      noInterrupts();  // Отключаем все прерывания временно
      detachInterrupt(encoderPinB);
      interrupts();    // Включаем обратно
    }
    
    // Настраиваем пины как INPUT (без подтяжки) для аналогового чтения
    if (encoderPinA >= 0) {
      pinMode(encoderPinA, INPUT);
      delay(10);
    }
    if (encoderPinB >= 0) {
      pinMode(encoderPinB, INPUT);
    }
    
    // Инициализируем min/max
    analogMinA = 4095;
    analogMaxA = 0;
    analogMinB = 4095;
    analogMaxB = 0;
  }
}

void TenZillaEncoder::setAnalogThreshold(int pinAThreshold, int pinBThreshold) {
  analogThresholdA = pinAThreshold;
  analogThresholdB = pinBThreshold;
  autoThreshold = false;
}

void TenZillaEncoder::setAutoThreshold(bool enabled) {
  autoThreshold = enabled;
  if (enabled) {
    // Сбрасываем min/max для нового определения
    analogMinA = 4095;
    analogMaxA = 0;
    analogMinB = 4095;
    analogMaxB = 0;
  }
}

int TenZillaEncoder::readAnalogDigital(int pin, int threshold) {
  int analogValue = analogRead(pin);
  return (analogValue > threshold) ? 1 : 0;
}

void TenZillaEncoder::updateAnalogThresholds() {
  if (analogMaxA > analogMinA) {
    analogThresholdA = (analogMinA + analogMaxA) / 2;
  }
  if (analogMaxB > analogMinB) {
    analogThresholdB = (analogMinB + analogMaxB) / 2;
  }
}




