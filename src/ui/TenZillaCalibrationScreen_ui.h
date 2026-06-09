/**
 * TenZilla Calibration Screen — слой дизайна (UI)
 * Динамические виджеты: labelCurrentWeight, labelCalibrationFactor, labelStep
 */

#ifndef TENZILLA_CALIBRATION_SCREEN_UI_H
#define TENZILLA_CALIBRATION_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelCurrentWeight;
  lv_obj_t* labelCalibrationFactor;
  lv_obj_t* labelStep;
} TenZillaCalibrationScreenUI;

void TenZillaCalibrationScreen_ui_create(lv_obj_t** out_screen, TenZillaCalibrationScreenUI* out_ui);

#endif
