#ifndef TENZILLA_HISTORY_SCREEN_UI_H
#define TENZILLA_HISTORY_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* scrollCont;
} TenZillaHistoryScreenUI;

void TenZillaHistoryScreen_ui_create(lv_obj_t** out_screen, TenZillaHistoryScreenUI* out_ui);
void TenZillaHistoryScreen_ui_fill_list(TenZillaHistoryScreenUI* ui);

#endif
