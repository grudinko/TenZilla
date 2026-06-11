#include "TenZillaMainScreen.h"
#include "TenZillaDisplay.h"
#include "TenZillaScale.h"
#include "TenZillaProgram.h"
#include "TenZillaEncoder.h"
#include "ui/TenZillaMainScreen_ui.h"
#include "TenZillaLvglShim.h"
#include <cmath>
#ifdef ESP32
  #include <esp_system.h>
#endif
// FontAwesome шрифт (если доступен)
#if defined(LV_FONT_FA14_ENABLED) || defined(LV_FONT_FA48_ENABLED) || defined(LV_FONT_FA60_ENABLED) || defined(LV_FONT_FA96_ENABLED)
  #include "fonts/lv_font_fontawesome.h"
#endif

// Инициализация статических переменных
float TenZillaMainScreen::lastDisplayedWeight = 0.0f;
float TenZillaMainScreen::lastDisplayedMaxWeight = -1.0f;
int TenZillaMainScreen::lastDisplayedDisplacement = -999999;
float TenZillaMainScreen::smoothedWeight = 0.0f;
float TenZillaMainScreen::smoothedMaxWeight = 0.0f;
float TenZillaMainScreen::screenMaxWeight = 0.0f;  // Сохраняется между переключениями

// История для графика силы
float TenZillaMainScreen::forceHistory[GRAPH_HISTORY_SIZE] = {0};
int TenZillaMainScreen::historyIndex = 0;
static float cachedMaxForce = 0.0f;  // Кэшированный максимум для оптимизации
static unsigned long lastMaxUpdate = 0;  // Время последнего обновления максимума

// Состояние двигателя для отслеживания изменений
bool TenZillaMainScreen::lastMotorRunning = false;
int TenZillaMainScreen::lastMotorDirection = 0;

// UI-слой (дизайн в ui/TenZillaMainScreen_ui.*)
static TenZillaMainScreenUI s_ui;

void TenZillaMainScreen::drawStatic(void* tft) {
  // Старый метод TFT_eSPI - больше не используется (заменен на LVGL)
  (void)tft;
  return;

}

void TenZillaMainScreen::updateData(void* tft, bool forceUpdate, float currentWeight, float maxWeight, int opticalCount, float absoluteDisplacement, float workingDisplacement) {
  // Старый метод TFT_eSPI - больше не используется (заменен на LVGL)
  (void)tft; (void)forceUpdate; (void)currentWeight; (void)maxWeight; (void)opticalCount; (void)absoluteDisplacement; (void)workingDisplacement;
  return;
}

void TenZillaMainScreen::drawGraph(void* tft) {
  // Старый метод TFT_eSPI - больше не используется (заменен на LVGL)
  (void)tft;
  return;
}

void TenZillaMainScreen::drawMotorIcon(void* tft) {
  // Старый метод TFT_eSPI - больше не используется (заменен на LVGL)
  (void)tft;
  return;
}

void TenZillaMainScreen::resetMaxWeight() {
  screenMaxWeight = 0.0f;
  lastDisplayedMaxWeight = -1.0f;
  smoothedMaxWeight = 0.0f;
}

float TenZillaMainScreen::getMaxWeight() {
  return screenMaxWeight;  // N
}

void TenZillaMainScreen::drawStatusMessage(void* tft) {
  // Старый метод TFT_eSPI - больше не используется (заменен на LVGL)
  (void)tft;
  return;
}

void TenZillaMainScreen::clearStatusMessage() {
  // Очистка сообщения происходит автоматически в drawStatusMessage
  // при обнаружении запуска новой программы
  // Эта функция оставлена для совместимости API
}

// ============================================
// LVGL МЕТОДЫ (логика; дизайн в ui/)
// ============================================

void TenZillaMainScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaMainScreen_ui_create(&screen, &s_ui);
  if (screen == nullptr) return;

  char limBuf[16];
  snprintf(limBuf, sizeof(limBuf), "%.0fN", TenZillaScale::getMaxWeight());
  lv_label_set_text(s_ui.labelLimVal, limBuf);
}

void TenZillaMainScreen::updateLVGL(lv_obj_t* screen, float currentWeight, float maxWeight, int opticalCount) {
  (void)maxWeight;
  (void)opticalCount;
  if (screen == nullptr || s_ui.labelCurrentWeight == nullptr) return;

  float currentN = currentWeight;  // N
  if (currentN > screenMaxWeight) screenMaxWeight = currentN;

  // Динамическое обновление: обновляем только если значение изменилось
  // Это значительно уменьшает количество перерисовок и повышает FPS
  char buffer[32];
  
  // Обновляем текущий вес только если изменился (с точностью до 0.1 Н)
  if (fabs(currentN - lastDisplayedWeight) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f N", currentN);
    lv_label_set_text(s_ui.labelCurrentWeight, buffer);
    lastDisplayedWeight = currentN;
  }

  // Обновляем максимум только если изменился
  if (fabs(screenMaxWeight - lastDisplayedMaxWeight) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f N", screenMaxWeight);
    lv_label_set_text(s_ui.labelMaxWeight, buffer);
    lastDisplayedMaxWeight = screenMaxWeight;
  }

  // Обновление прогресс-бара и процентов от лимита - только при изменении
  if (s_ui.progressBarWeight != nullptr && s_ui.labelProgressPercent != nullptr) {
    static int lastPercent = -1;
    static int lastPercentForBar = -1;
    
    float limitN = TenZillaScale::getMaxWeight();  // N
    int percent = 0;
    int percentForBar = 0;  // Для прогресс-бара (ограничен 100%)
    if (limitN > 0.0f) {
      percent = (int)((currentN / limitN) * 100.0f);
      if (percent < 0) percent = 0;
      // Проценты могут быть больше 100 - убираем ограничение для отображения
      percentForBar = percent;
      if (percentForBar > 100) percentForBar = 100;  // Прогресс-бар ограничен 100%
    }
    
    // Обновляем прогресс-бар только если значение изменилось
    if (percentForBar != lastPercentForBar) {
      lv_bar_set_value(s_ui.progressBarWeight, percentForBar, LV_ANIM_OFF);
      
      // Динамическое изменение цвета: белый -> красный
      // Вычисляем цвет в зависимости от процента (без градиента, только один цвет)
      uint8_t r, g, b;
      // От белого (255, 255, 255) к красному (255, 0, 0)
      float ratio = percentForBar / 100.0f;
      r = 255;                                    // Всегда 255 (красный)
      g = (uint8_t)(255 - (255 * ratio));        // 255 -> 0
      b = (uint8_t)(255 - (255 * ratio));        // 255 -> 0
      
      // Убеждаемся, что в максимуме цвет красный (255, 0, 0)
      if (percentForBar >= 100) {
        r = 255;
        g = 0;
        b = 0;
      }
      
      // Если дисплей показывает синий вместо красного, возможно проблема в порядке RGB/BGR
      // Если дисплей использует BGR вместо RGB, красный (255,0,0) будет отображаться как синий (0,0,255)
      // Пробуем поменять R и B местами - если это решит проблему, значит дисплей использует BGR
      lv_color_t barColor = lv_color_make(b, g, r);  // Меняем местами R и B для BGR дисплея
      
      // Устанавливаем цвет напрямую через стиль
      lv_obj_set_style_bg_color(s_ui.progressBarWeight, barColor, LV_PART_INDICATOR);
      
      // Отключаем градиент явно и принудительно
      lv_obj_set_style_bg_grad_dir(s_ui.progressBarWeight, LV_GRAD_DIR_NONE, LV_PART_INDICATOR);
      lv_obj_set_style_bg_grad_color(s_ui.progressBarWeight, barColor, LV_PART_INDICATOR);
      lv_obj_set_style_bg_grad_stop(s_ui.progressBarWeight, 0, LV_PART_INDICATOR);
      
      lastPercentForBar = percentForBar;
    }
    
    // Обновляем текст процентов только если изменился
    if (percent != lastPercent) {
      snprintf(buffer, sizeof(buffer), "%d%%", percent);
      lv_label_set_text(s_ui.labelProgressPercent, buffer);
      lastPercent = percent;
    }
  }
  
  float absoluteDisplacement = TenZillaProgram::isRunning() ? TenZillaProgram::getAbsoluteDisplacement() : TenZillaScale::getDisplacement();
  // Рабочее перемещение показываем всегда (даже после завершения программы)
  float workingDisplacement = TenZillaProgram::getWorkingDisplacement();
  
  // Обновление LIM (лимит веса) - только если изменился
  if (s_ui.labelLimVal != nullptr) {
    static float lastLimitN = -1.0f;
    float limitN = TenZillaScale::getMaxWeight();
    if (fabs(limitN - lastLimitN) >= 0.1f) {
      snprintf(buffer, sizeof(buffer), "%.0fN", limitN);
      lv_label_set_text(s_ui.labelLimVal, buffer);
      lastLimitN = limitN;
    }
  }
  
  // Обновление WRK (накопленное перемещение при сжатии) - только если изменилось
  if (s_ui.labelWorkingDisplacement != nullptr) {
    static float lastWorkingDisplacement = -9999.0f;
    if (fabs(workingDisplacement - lastWorkingDisplacement) >= 0.1f) {
      snprintf(buffer, sizeof(buffer), "%.1f mm", workingDisplacement);
      lv_label_set_text(s_ui.labelWorkingDisplacement, buffer);
      lastWorkingDisplacement = workingDisplacement;
    }
  }
  
  // Обновление MOV (абсолютное перемещение) - только если изменилось
  if (s_ui.labelDisplacement != nullptr) {
    static float lastAbsoluteDisplacement = -9999.0f;
    if (fabs(absoluteDisplacement - lastAbsoluteDisplacement) >= 0.1f) {
      snprintf(buffer, sizeof(buffer), "%.1f mm", absoluteDisplacement);
      lv_label_set_text(s_ui.labelDisplacement, buffer);
      lastAbsoluteDisplacement = absoluteDisplacement;
    }
  }

  // Проверка лимитов энкодера и перегрузки по весу для статусного сообщения
  int encoderCount = TenZillaScale::getOpticalCount();
  int encoderMin = TenZillaScale::getEncoderMin();
  int encoderMax = TenZillaScale::getEncoderMax();
  bool limitExceeded = false;
  String limitMessage = "";
  
  // Проверка лимитов энкодера
  if (encoderCount < encoderMin) {
    limitExceeded = true;
    limitMessage = "LIMIT: < MIN";
  } else if (encoderCount > encoderMax) {
    limitExceeded = true;
    limitMessage = "LIMIT: > MAX";
  }
  
  // Проверка перегрузки по весу (всё в N)
  float currentWeightN = currentWeight;
  float maxWeightN = TenZillaScale::getMaxWeight();
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    limitExceeded = true;
    limitMessage = "OVERLOAD";
  }
  
  // Приоритет: статус программы (включая завершение) > отключение ограничений > перегрузка/лимиты
  // Обновляем статус всегда, если labelStatus инициализирован
  if (s_ui.labelStatus != nullptr) {
    String status = TenZillaProgram::getStatusMessage();
    
    // Проверка отключения ограничений (высший приоритет после статуса программы)
    bool limitsDisabled = TenZillaScale::areLimitsDisabled();
    bool showOverload = limitExceeded && (status.length() == 0 || !TenZillaProgram::isRunning());
    
    if (status.length() > 0 && TenZillaProgram::isRunning()) {
      // Перед стартом (Waiting) — мигание надписи каждые 500 мс
      if (status.indexOf("Waiting") >= 0) {
        static unsigned long lastWaitingBlink = 0;
        static bool waitingBlinkOn = true;
        unsigned long now = millis();
        if (now - lastWaitingBlink >= 500) {
          waitingBlinkOn = !waitingBlinkOn;
          lastWaitingBlink = now;
        }
        if (waitingBlinkOn) {
          lv_label_set_text(s_ui.labelStatus, status.c_str());
          lv_obj_set_style_text_color(s_ui.labelStatus, lv_color_hex(0x00FFFF), 0);
        } else {
          lv_label_set_text(s_ui.labelStatus, "");
        }
      } else {
        // Остальной статус программы — без мигания
        lv_label_set_text(s_ui.labelStatus, status.c_str());
        if (status.indexOf("COMPLETED") >= 0) {
          lv_color_t successColor = lv_color_make(0, 255, 0);  // BGR: зеленый
          lv_obj_set_style_text_color(s_ui.labelStatus, successColor, 0);
        } else if (status.indexOf("STOPPED") >= 0) {
          lv_color_t stoppedColor = lv_color_make(0, 255, 255);  // BGR: желтый
          lv_obj_set_style_text_color(s_ui.labelStatus, stoppedColor, 0);
        } else if (status.indexOf("ENCODER FAULT") >= 0) {
          lv_color_t faultColor = lv_color_make(0, 0, 255);  // BGR: красный
          lv_obj_set_style_text_color(s_ui.labelStatus, faultColor, 0);
        } else {
          lv_obj_set_style_text_color(s_ui.labelStatus, lv_color_hex(0x00FFFF), 0);
        }
      }
    } else if (limitsDisabled) {
      // Показываем мигающее предупреждение об отключенных ограничениях
      static unsigned long lastBlinkTime = 0;
      static bool blinkState = false;
      unsigned long now = millis();
      if (now - lastBlinkTime >= 500) {  // Мигание каждые 500мс
        blinkState = !blinkState;
        lastBlinkTime = now;
      }
      
      if (blinkState) {
        lv_label_set_text(s_ui.labelStatus, "! LIMITS DISABLED");
        // Желтый цвет для BGR дисплея: lv_color_make(b, g, r)
        lv_color_t warningColor = lv_color_make(0, 255, 255);  // BGR порядок: желтый
        lv_obj_set_style_text_color(s_ui.labelStatus, warningColor, 0);
      } else {
        lv_label_set_text(s_ui.labelStatus, "");
      }
    } else if (showOverload) {
      // Показываем перегрузку/лимиты, если программа не запущена или нет статуса
      lv_label_set_text(s_ui.labelStatus, limitMessage.c_str());
      // Красный цвет для BGR дисплея: как в прогресс-баре - lv_color_make(b, g, r)
      lv_color_t limitColor = lv_color_make(0, 0, 255);  // BGR порядок: красный
      lv_obj_set_style_text_color(s_ui.labelStatus, limitColor, 0);
    } else if (status.length() > 0) {
      lv_label_set_text(s_ui.labelStatus, status.c_str());
      if (status.indexOf("ENCODER FAULT") >= 0) {
        lv_color_t faultColor = lv_color_make(0, 0, 255);  // BGR: красный
        lv_obj_set_style_text_color(s_ui.labelStatus, faultColor, 0);
      } else if (status.indexOf("COMPLETED") >= 0) {
        lv_color_t successColor = lv_color_make(0, 255, 0);  // BGR: зеленый
        lv_obj_set_style_text_color(s_ui.labelStatus, successColor, 0);
      } else if (status.indexOf("STOPPED") >= 0) {
        lv_color_t stoppedColor = lv_color_make(0, 255, 255);  // BGR: желтый
        lv_obj_set_style_text_color(s_ui.labelStatus, stoppedColor, 0);
      }
    } else {
      // Нет ни статуса, ни перегрузки/лимитов — показываем READY, если система в норме
      bool motorRunning = TenZillaScale::isMotorRunning();
      bool encoderFault = TenZillaScale::getEncoderFault();
      if (!TenZillaProgram::isRunning() && !motorRunning && !encoderFault) {
        lv_label_set_text(s_ui.labelStatus, "Ready");
        lv_obj_set_style_text_color(s_ui.labelStatus, lv_color_hex(0x00FFFF), 0);
      } else {
        lv_label_set_text(s_ui.labelStatus, "");
      }
    }
  }

  // Анимация статуса двигателя с FontAwesome-подобными символами
  bool motorRunning = TenZillaScale::isMotorRunning();
  int motorDirection = TenZillaScale::getMotorDirection();
  
  if (s_ui.labelMotorIcon != nullptr) {
    static unsigned long lastMotorAnimTime = 0;
    static int animFrame = 0;
    unsigned long now = millis();
    
    if (motorRunning && motorDirection == 1) {
      // Вращение вверх - только FontAwesome circle-up (зеленый, как в веб-интерфейсе)
      #if LV_FONT_FA60_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);  // 60px - увеличение на 20%
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_UP);  // FontAwesome circle-up
      #elif LV_FONT_FA48_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);  // 48px - fallback
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_UP);  // FontAwesome circle-up
      #else
        lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_UP);  // Fallback
      #endif
      lv_obj_set_style_text_color(s_ui.labelMotorIcon, lv_color_hex(0x008000), 0);  // Зеленый как в веб-интерфейсе
    } else if (motorRunning && motorDirection == -1) {
      // Вращение вниз - только FontAwesome circle-down (синий, как в веб-интерфейсе)
      #if LV_FONT_FA60_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);  // 60px - увеличение на 20%
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_DOWN);  // FontAwesome circle-down
      #elif LV_FONT_FA48_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);  // 48px - fallback
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_DOWN);  // FontAwesome circle-down
      #else
        lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_DOWN);  // Fallback
      #endif
      lv_obj_set_style_text_color(s_ui.labelMotorIcon, lv_color_hex(0x00CCCC), 0);  // Синий/cyan как в веб-интерфейсе
    } else {
      // Остановлен - circle-stop (красный, как в веб-интерфейсе)
      lastMotorAnimTime = 0;
      animFrame = 0;
      #if LV_FONT_FA60_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);  // 60px - увеличение на 20%
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_STOP);  // FontAwesome circle-stop
      #elif LV_FONT_FA48_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);  // 48px - fallback
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_STOP);  // FontAwesome circle-stop
      #else
        lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_STOP);  // Fallback
      #endif
      // Красный цвет для BGR дисплея: как в прогресс-баре - lv_color_make(b, g, r)
      // RGB красный (255, 0, 0) -> BGR красный (0, 0, 255)
      lv_color_t stopColor = lv_color_make(0, 0, 255);  // BGR порядок: красный (как в прогресс-баре)
      lv_obj_set_style_text_color(s_ui.labelMotorIcon, stopColor, 0);
    }
  }
}