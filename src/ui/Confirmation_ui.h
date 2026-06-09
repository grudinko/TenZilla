/**
 * Menu / Confirmation Screen — 4 или 3 кнопки (START только на экранах 1–2).
 * Стиль как у OTHER SETTINGS и др.
 */

#ifndef CONFIRMATION_UI_H
#define CONFIRMATION_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* btnStart;     /* Скрыт на экранах 3–6 */
  lv_obj_t* btnResetMov;
  lv_obj_t* btnResetZero;
  lv_obj_t* btnExit;
} ConfirmationUI;

void Confirmation_ui_create(lv_obj_t** out_screen, ConfirmationUI* out_ui);

#endif
