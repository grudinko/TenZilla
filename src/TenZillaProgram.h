#ifndef TENZILLA_PROGRAM_H
#define TENZILLA_PROGRAM_H

#include <Arduino.h>
#include "TenZillaPins.h"  // Централизованные определения пинов

class TenZillaProgram {
public:
  enum StopReason {
    STOP_REASON_MANUAL = 0,
    STOP_REASON_WEB = 1,
    STOP_REASON_ENCODER_FAULT = 2,
    STOP_REASON_OTHER = 3
  };

  // Инициализация
  static void begin();
  static void update();
  
  // Управление программой
  static void startCompressionProgram();  // Запуск программы СЖАТИЕ
  static void startBreakProgram();        // Запуск программы РАЗРЫВ
  static void stopProgram(StopReason reason = STOP_REASON_MANUAL);  // Остановка программы
  static bool isRunning();                // Программа запущена?
  static int getProgramType();            // 1=COMPRESSION, 2=BREAK, 0=STOPPED
  
  // Настройки программы
  static void setCompressionStartThreshold(float thresholdN);  // Порог начала накопления для сжатия (Н)
  static float getCompressionStartThreshold();
  static void setCompressionTargetDisplacement(float targetMm); // Целевое рабочее перемещение для сжатия (мм)
  static float getCompressionTargetDisplacement();
  static void setCompressionUnloadRetractMm(float mm);  // Расстояние после разгрузки до 0 (мм), не ниже нуля энкодера
  static float getCompressionUnloadRetractMm();
  static void setBreakDropThreshold(float thresholdPercent);   // Порог падения для разрыва (%)
  static float getBreakDropThreshold();
  
  // Получение данных программы
  static float getAbsoluteDisplacement();  // Абсолютное перемещение (из энкодера)
  static float getWorkingDisplacement();   // Рабочее перемещение (накопленное с момента начала роста)
  static unsigned long getProgramStartTimeMs();  // millis() в момент старта программы, 0 если не запущена
  static unsigned long getLastProgramDurationMs();  // длительность последнего запуска (мс), 0 если не было
  
  // Получение статуса программы для отображения предупреждений
  static String getStatusMessage();  // Текущее сообщение о статусе программы
  static bool isCompletedSuccessfully();  // Проверка успешного завершения программы
  static bool isStoppedManually();       // Проверка остановки программы вручную
  
  // Звуковые сигналы
  static void beepLong();      // Протяжный гудок
  static void beepShort();     // Короткий гудок
  static void beepError();     // 5 коротких гудков (ошибка)
  
private:
  // Состояние программы
  static int programType;      // 0=остановлена, 1=СЖАТИЕ, 2=РАЗРЫВ
  static bool programRunning;
  static unsigned long programStartTime;
  static unsigned long lastProgramDurationMs;  // длительность последнего запуска при остановке
  static bool programCompletedSuccessfully;  // Флаг успешного завершения программы
  static bool programStoppedManually;       // Флаг остановки программы вручную
  
  // Состояние программы СЖАТИЕ
  static bool compressionWaiting;      // Ожидание 3 секунды
  static bool compressionMovingDown;   // Движение вниз до начала роста
  static bool compressionMeasuring;    // Измерение рабочего перемещения
  static bool compressionHolding;      // Удержание позиции перед разгрузкой (для стабилизации веса)
  static bool compressionUnloading;    // Разгрузка до 0 и откат на X мм (не ниже нуля энкодера)
  static int compressionRetractTargetEncoder;  // Целевой счётчик энкодера после отката (при достижении веса 0)
  static float compressionStartWeight; // Вес в момент начала роста
  static float workingDisplacement;    // Накопленное рабочее перемещение
  static float absoluteDisplacementStart; // Абсолютное перемещение в начале программы
  
  // Состояние программы РАЗРЫВ
  static bool breakWaiting;
  static bool breakMovingDown;
  static bool breakMeasuring;
  static float breakMaxWeight;         // Максимальный вес во время разрыва
  static bool breakMaxReached;         // Достигнут максимум
  static float breakAbsoluteDisplacementStart; // Абсолютное перемещение в момент начала роста
  
  // Настройки программы
  static float compressionStartThreshold;  // Порог начала накопления (Н)
  static float compressionTargetDisplacement; // Целевое перемещение (мм)
  static float compressionUnloadRetractMm;    // Расстояние после разгрузки до 0 (мм), по умолчанию 5.0
  static float breakDropThreshold;        // Порог падения (% от максимума)
  
  // Внутреннее время для фаз удержания и ожиданий
  static unsigned long compressionHoldStartTime;  // Время начала удержания перед разгрузкой
  
  // Бипер - пин определен в TenZillaPins.h (используется макрос BUZZER_PIN напрямую)
  static void beep(int durationMs, int pauseMs = 0);
};

#endif

