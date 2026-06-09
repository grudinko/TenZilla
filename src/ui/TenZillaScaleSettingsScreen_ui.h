#ifndef TENZILLA_SCALE_SETTINGS_SCREEN_UI_H
#define TENZILLA_SCALE_SETTINGS_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelCurN;           /* 1. Текущий вес (N) */
  lv_obj_t* labelRawValue;       /* 2. Текущее RAW */
  lv_obj_t* labelCalibrationFactor; /* 3. Factor */
  lv_obj_t* labelZeroRaw;        /* 4. RAW нулевой точки */
  lv_obj_t* labelCalN;           /* 5. Значение в N второй точки */
  lv_obj_t* labelCalRaw;         /* 6. RAW второй точки */
  lv_obj_t* labelNoise;          /* 7. Текущий шум (green/red) */
  lv_obj_t* labelMaxWeight;      /* 8. Максимальный вес */
} TenZillaScaleSettingsScreenUI;

void TenZillaScaleSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaScaleSettingsScreenUI* out_ui);

#endif
