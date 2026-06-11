#include "TenZillaBreakScreen.h"
#include "TenZillaDisplay.h"
#include "TenZillaScale.h"
#include "TenZillaProgram.h"
#include "TenZillaEncoder.h"
#include "ui/TenZillaBreakScreen_ui.h"
#include "TenZillaLvglShim.h"
#include <cmath>
#include <cstring>
#ifdef ESP32
  #include <esp_system.h>
#endif
// FontAwesome шрифт (если доступен)
#if defined(LV_FONT_FA14_ENABLED) || defined(LV_FONT_FA48_ENABLED) || defined(LV_FONT_FA60_ENABLED) || defined(LV_FONT_FA96_ENABLED)
  #include "fonts/lv_font_fontawesome.h"
#endif

float TenZillaBreakScreen::lastDisplayedWeight = 0.0f;
float TenZillaBreakScreen::lastDisplayedMaxWeight = -1.0f;
int TenZillaBreakScreen::lastDisplayedDisplacement = -999999;
float TenZillaBreakScreen::smoothedWeight = 0.0f;
float TenZillaBreakScreen::smoothedMaxWeight = 0.0f;
float TenZillaBreakScreen::screenMaxWeight = 0.0f;

float TenZillaBreakScreen::forceHistory[GRAPH_HISTORY_SIZE] = {0};
int TenZillaBreakScreen::historyIndex = 0;

bool TenZillaBreakScreen::lastMotorRunning = false;
int TenZillaBreakScreen::lastMotorDirection = 0;

static TenZillaBreakScreenUI s_ui;

void TenZillaBreakScreen::drawStatic(void* tft) {
  (void)tft;
}

void TenZillaBreakScreen::updateData(void* tft, bool forceUpdate, float currentWeight, float maxWeight, int opticalCount, float absoluteDisplacement, float workingDisplacement) {
  (void)tft; (void)forceUpdate; (void)currentWeight; (void)maxWeight; (void)opticalCount; (void)absoluteDisplacement; (void)workingDisplacement;
}

void TenZillaBreakScreen::drawGraph(void* tft) {
  (void)tft;
}

void TenZillaBreakScreen::drawMotorIcon(void* tft) {
  (void)tft;
}

void TenZillaBreakScreen::drawStatusMessage(void* tft) {
  (void)tft;
}

void TenZillaBreakScreen::resetMaxWeight() {
  screenMaxWeight = 0.0f;
  lastDisplayedMaxWeight = -1.0f;
  smoothedMaxWeight = 0.0f;
}

float TenZillaBreakScreen::getMaxWeight() {
  return screenMaxWeight;  // N
}

// ============================================
// LVGL (логика; дизайн в ui/)
// ============================================

void TenZillaBreakScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaBreakScreen_ui_create(&screen, &s_ui);
}

void TenZillaBreakScreen::updateLVGL(lv_obj_t* screen, float currentWeight, float maxWeight, int opticalCount) {
  (void)maxWeight;
  (void)opticalCount;
  if (screen == nullptr || s_ui.labelCurrentWeight == nullptr) return;

  float currentN = currentWeight;  // N
  if (currentN > screenMaxWeight) screenMaxWeight = currentN;

  char buffer[32];
  static float lastCurrentN = -9999.0f;
  static float lastMaxWeight = -9999.0f;
  static float lastAbsoluteDisplacement = -9999.0f;
  static float lastWorkingDisplacement = -9999.0f;
  static float lastMaxWeightN = -9999.0f;
  
  // Обновляем текущий вес только при изменении
  if (fabs(currentN - lastCurrentN) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f N", currentN);
    lv_label_set_text(s_ui.labelCurrentWeight, buffer);
    lastCurrentN = currentN;
  }

  // Обновляем максимум только при изменении
  if (fabs(screenMaxWeight - lastMaxWeight) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f N", screenMaxWeight);
    lv_label_set_text(s_ui.labelMaxWeight, buffer);
    lastMaxWeight = screenMaxWeight;
  }

  float absoluteDisplacement = TenZillaProgram::isRunning() ? TenZillaProgram::getAbsoluteDisplacement() : TenZillaScale::getDisplacement();
  float workingDisplacement = TenZillaProgram::isRunning() ? TenZillaProgram::getWorkingDisplacement() : 0.0f;
  
  // Обновляем рабочее перемещение (WRK) только при изменении
  if (s_ui.labelWorkingDisplacement != nullptr) {
    if (fabs(workingDisplacement - lastWorkingDisplacement) >= 0.1f) {
      snprintf(buffer, sizeof(buffer), "%.1f mm", workingDisplacement);
      lv_label_set_text(s_ui.labelWorkingDisplacement, buffer);
      lastWorkingDisplacement = workingDisplacement;
    }
  }
  
  // Обновляем абсолютное перемещение (MOV) только при изменении
  if (fabs(absoluteDisplacement - lastAbsoluteDisplacement) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f mm", absoluteDisplacement);
    lv_label_set_text(s_ui.labelDisplacement, buffer);
    lastAbsoluteDisplacement = absoluteDisplacement;
  }
  
  // Обновляем лимит (LIM) только при изменении
  float maxWeightN = TenZillaScale::getMaxWeight();
  if (s_ui.labelLimVal != nullptr) {
    if (fabs(maxWeightN - lastMaxWeightN) >= 0.1f) {
      if (maxWeightN > 0.0f) {
        snprintf(buffer, sizeof(buffer), "%.0fN", maxWeightN);
        lv_label_set_text(s_ui.labelLimVal, buffer);
      } else {
        lv_label_set_text(s_ui.labelLimVal, "\x2D\x2D\x2D");
      }
      lastMaxWeightN = maxWeightN;
    }
  }
  
  // Обновление прогресс-бара и процентов от лимита (как в MainScreen) - только при изменении
  if (s_ui.progressBarWeight != nullptr && s_ui.labelProgressPercent != nullptr) {
    static int lastPercent = -1;
    static int lastPercentForBar = -1;
    
    float limitN = maxWeightN;
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
      uint8_t r, g, b;
      float ratio = percentForBar / 100.0f;
      r = 255;                                    // Всегда 255 (красный)
      g = (uint8_t)(255 - (255 * ratio));        // 255 -> 0
      b = (uint8_t)(255 - (255 * ratio));        // 255 -> 0
      
      if (percentForBar >= 100) {
        r = 255;
        g = 0;
        b = 0;
      }
      
      lv_color_t barColor = lv_color_make(b, g, r);  // BGR порядок для дисплея
      
      lv_obj_set_style_bg_color(s_ui.progressBarWeight, barColor, LV_PART_INDICATOR);
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
  if (maxWeightN > 0.0f && currentWeightN > maxWeightN) {
    limitExceeded = true;
    limitMessage = "OVERLOAD";
  }
  
  // Приоритет: статус программы (включая завершение) > отключение ограничений > перегрузка/лимиты
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

  // Анимация статуса двигателя с FontAwesome-подобными символами (как в MainScreen)
  bool motorRunning = TenZillaScale::isMotorRunning();
  int motorDirection = TenZillaScale::getMotorDirection();
  
  if (s_ui.labelMotorIcon != nullptr) {
    if (!motorRunning) {
      // Как на экране СЖАТИЕ: FA_CIRCLE_STOP (FontAwesome), иначе LV_SYMBOL_STOP
      lv_obj_clear_flag(s_ui.labelMotorIcon, LV_OBJ_FLAG_HIDDEN);
      #if LV_FONT_FA60_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_STOP);
      #elif LV_FONT_FA48_ENABLED
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);
        lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_STOP);
      #else
        lv_obj_set_style_text_font(s_ui.labelMotorIcon, &lv_font_montserrat_48, 0);
        lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_STOP);
      #endif
      lv_obj_set_style_text_color(s_ui.labelMotorIcon, lv_color_make(0, 0, 255), 0);
    } else {
      lv_obj_clear_flag(s_ui.labelMotorIcon, LV_OBJ_FLAG_HIDDEN);
      if (motorDirection == 1) {
        // Вращение вверх - FontAwesome circle-up (зеленый)
        #if LV_FONT_FA60_ENABLED
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);
          lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_UP);
        #elif LV_FONT_FA48_ENABLED
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);
          lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_UP);
        #else
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, &lv_font_montserrat_48, 0);
          lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_UP);
        #endif
        lv_obj_set_style_text_color(s_ui.labelMotorIcon, lv_color_hex(0x008000), 0);  // Зеленый
      } else if (motorDirection == -1) {
        // Вращение вниз - FontAwesome circle-down (синий)
        #if LV_FONT_FA60_ENABLED
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA60, 0);
          lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_DOWN);
        #elif LV_FONT_FA48_ENABLED
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, LV_FONT_FA48, 0);
          lv_label_set_text(s_ui.labelMotorIcon, FA_CIRCLE_DOWN);
        #else
          lv_obj_set_style_text_font(s_ui.labelMotorIcon, &lv_font_montserrat_48, 0);
          lv_label_set_text(s_ui.labelMotorIcon, LV_SYMBOL_DOWN);
        #endif
        lv_obj_set_style_text_color(s_ui.labelMotorIcon, lv_color_hex(0x00CCCC), 0);  // Синий
      }
    }
  }
}
