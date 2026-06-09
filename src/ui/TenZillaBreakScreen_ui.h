/**
 * TenZilla Break Screen — слой дизайна (UI)
 * Использует тот же дизайн что и Main Screen (COMPRESSION)
 * Динамические виджеты: labelCurrentWeight, labelMaxWeight, labelDisplacement,
 * labelStatus, labelMotorIcon, labelWorkingDisplacement, labelLimVal, progressBarWeight
 */

#ifndef TENZILLA_BREAK_SCREEN_UI_H
#define TENZILLA_BREAK_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelCurrentWeight;
  lv_obj_t* labelMaxWeight;
  lv_obj_t* labelDisplacement;      // MOV - абсолютное перемещение
  lv_obj_t* labelWorkingDisplacement; // WRK - накопленное перемещение при разрыве
  lv_obj_t* labelStatus;
  lv_obj_t* labelMotorIcon;
  lv_obj_t* labelMotorIconCenter;  // Белый центр для стоп-значка
  lv_obj_t* labelLimVal;
  lv_obj_t* progressBarWeight;
  lv_obj_t* labelProgressPercent;
} TenZillaBreakScreenUI;

void TenZillaBreakScreen_ui_create(lv_obj_t** out_screen, TenZillaBreakScreenUI* out_ui);

#endif
