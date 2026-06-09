/**
 * TenZilla Main Screen (COMPRESSION) — слой дизайна (UI)
 *
 * Только создание виджетов, стили, layout. Без логики приложения.
 * Можно править в SquareLine Studio / LVGL Editor и заменять этот файл.
 *
 * Динамические виджеты (логика обновляет в updateLVGL):
 *   labelCurrentWeight, labelMaxWeight, labelDisplacement,
 *   labelStatus, labelMotorIcon, labelLimVal
 */

#ifndef TENZILLA_MAIN_SCREEN_UI_H
#define TENZILLA_MAIN_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelCurrentWeight;
  lv_obj_t* labelMaxWeight;
  lv_obj_t* labelDisplacement;      // MOV - абсолютное перемещение
  lv_obj_t* labelWorkingDisplacement; // WRK - накопленное перемещение при сжатии
  lv_obj_t* labelStatus;
  lv_obj_t* labelMotorIcon;
  lv_obj_t* labelMotorIconCenter;  // Белый центр для стоп-значка
  lv_obj_t* labelLimVal;
  lv_obj_t* progressBarWeight;
  lv_obj_t* labelProgressPercent;
} TenZillaMainScreenUI;

/** Создаёт экран и заполняет ui. *out_screen перезаписывается. */
void TenZillaMainScreen_ui_create(lv_obj_t** out_screen, TenZillaMainScreenUI* out_ui);

#endif
