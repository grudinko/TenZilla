#ifndef TENZILLA_CALIBRATION_SCREEN_H
#define TENZILLA_CALIBRATION_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaCalibrationScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen, float currentWeight);
  
  // Старые методы TFT_eSPI (для обратной совместимости)
  static void drawStatic(void* tft);
  static void updateData(void* tft, bool forceUpdate, float currentWeight, float calibrationFactor, int calibrationStep, float absoluteDisplacement);
  
  // Дизайн: ui/TenZillaCalibrationScreen_ui.*
};

#endif
