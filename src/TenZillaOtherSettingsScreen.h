#ifndef TENZILLA_OTHER_SETTINGS_SCREEN_H
#define TENZILLA_OTHER_SETTINGS_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaOtherSettingsScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen);
  
private:
  // Дизайн: ui/TenZillaOtherSettingsScreen_ui.*
};

#endif
