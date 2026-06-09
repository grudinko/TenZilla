#ifndef TENZILLA_SCALE_SETTINGS_SCREEN_H
#define TENZILLA_SCALE_SETTINGS_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaScaleSettingsScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen, float currentWeight);
  
private:
  // Дизайн: ui/TenZillaScaleSettingsScreen_ui.*
};

#endif
