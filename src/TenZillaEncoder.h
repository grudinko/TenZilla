#ifndef TENZILLA_ENCODER_H
#define TENZILLA_ENCODER_H

#include <Arduino.h>

class TenZillaEncoder {
public:
  // Инициализация энкодера
  static void begin(int pinA, int pinB, int pinSW = -1);
  
  // Обновление состояния энкодера (вызывать в loop() или из прерывания)
  static void update();
  
  // Получить текущий счетчик импульсов
  static int getCount();
  
  // Установить счетчик (для восстановления из NVS)
  static void setCount(int count);
  
  // Сбросить счетчик
  static void resetCount();
  
  // Получить изменение с последнего вызова
  static int getDelta();
  
  // Настройка направления (если энкодер работает в обратную сторону)
  static void setInverted(bool inverted);
  static bool isInverted();
  
  /** Режим для энкодера с датчиками Холла */
  static void setHallMode(bool enable);
  static bool isHallMode();
  static void setDebounceUs(unsigned long us);
  static void setInvertA(bool inv);
  static void setInvertB(bool inv);
  /** Режим «импульс + направление»: A = импульсы, B = направление (HIGH = вперёд) */
  static void setPulseDirMode(bool enable);
  static bool isPulseDirMode();
  /** Режим «только импульсы»: A = импульсы, B не используется (всегда +1 или −1 по inverted) */
  static void setPulseOnlyMode(bool enable);
  static bool isPulseOnlyMode();
  
  // Настройка для оптического энкодера (аналоговые сигналы)
  static void setOpticalMode(bool enabled);  // Включить режим оптического энкодера
  static void setAnalogThreshold(int pinAThreshold, int pinBThreshold);  // Установить пороги для аналоговых сигналов
  static void setAutoThreshold(bool enabled);  // Автоматическое определение порогов
  
  // ТЕСТОВАЯ КОНФИГУРАЦИЯ: Установка направления мотора для энкодера (когда нет канала A)
  static void setMotorDirection(int direction);  // 1=вверх, -1=вниз, 0=стоп
  
  // Проверка нажатия кнопки (если подключена)
  static bool isButtonPressed();
  static bool wasButtonClicked(); // Одно нажатие (с задержкой антидребезга)
  
private:
  static int encoderPinA;
  static int encoderPinB;
  static int encoderPinSW;
  static volatile int encoderCount;
  static volatile int lastEncoded;
  static int lastMSB;
  static int lastLSB;
  
  // Для кнопки
  static bool buttonPressed;
  static bool buttonClicked;
  static unsigned long lastButtonTime;
  static bool lastButtonState;
  
  // Защита от дребезга и помех
  static volatile unsigned long lastISRTime;  // Время последнего срабатывания ISR (микросекунды)
  static volatile int lastValidState;        // Последнее валидное состояние
  static bool inverted;                     // Инверсия направления энкодера
  static unsigned long debounceUs;          // Минимальный интервал между срабатываниями (µs). 1=оптика, 50–200=Hall.
  static bool invertA;                      // Инвертировать канал A (для Hall)
  static bool invertB;                      // Инвертировать канал B (для Hall)
  static bool hallMode;                     // Режим Hall: увеличенный debounce
  static bool pulseDirMode;                 // Режим «импульс + направление» (A=импульсы, B=направление)
  static bool pulseOnlyMode;                // Режим «только импульсы» (B=импульсы, A не используется - ТЕСТОВАЯ КОНФИГУРАЦИЯ)
  static volatile int lastPulseDirA;        // Последнее состояние A в режиме pulse+dir
  static volatile int lastPulseDirB;        // Последнее состояние B в режиме pulse-only (ТЕСТОВАЯ КОНФИГУРАЦИЯ)
  static volatile int motorDirectionForEncoder; // Направление мотора для энкодера (1=вверх, -1=вниз, 0=стоп) - ТЕСТОВАЯ КОНФИГУРАЦИЯ
  
  // Для оптического энкодера (аналоговые сигналы)
  static bool opticalMode;                  // Режим оптического энкодера
  static int analogThresholdA;             // Порог для пина A
  static int analogThresholdB;             // Порог для пина B
  static bool autoThreshold;                // Автоматическое определение порогов
  static int analogMinA, analogMaxA;       // Минимум и максимум для пина A
  static int analogMinB, analogMaxB;       // Минимум и максимум для пина B
  static unsigned long lastAnalogReadTime; // Время последнего чтения аналоговых значений
  
  // Внутренние методы
  static int readAnalogDigital(int pin, int threshold);  // Чтение аналогового значения и преобразование в цифровой
  static void updateAnalogThresholds();  // Обновление порогов на основе min/max
  
  // Обработчики прерываний (статичные функции)
  static void IRAM_ATTR encoderISR();
};

#endif




