#ifndef TENZILLA_OTHER_SETTINGS_SCREEN_UI_H
#define TENZILLA_OTHER_SETTINGS_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelCompThreshold;
  lv_obj_t* labelCompTarget;
  lv_obj_t* labelBreakThreshold;
} TenZillaOtherSettingsScreenUI;

void TenZillaOtherSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaOtherSettingsScreenUI* out_ui);

#endif
