#ifndef TENZILLA_MOTOR_SETTINGS_SCREEN_H
#define TENZILLA_MOTOR_SETTINGS_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaMotorSettingsScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen);
  
private:
  // Дизайн: ui/TenZillaMotorSettingsScreen_ui.*
};

#endif
