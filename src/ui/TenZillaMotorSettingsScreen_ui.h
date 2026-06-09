#ifndef TENZILLA_MOTOR_SETTINGS_SCREEN_UI_H
#define TENZILLA_MOTOR_SETTINGS_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelDisplacement;
  lv_obj_t* labelOpticalCount;
  lv_obj_t* labelEncoderStep;
  lv_obj_t* labelEncoderMax;
  lv_obj_t* labelEncoderMaxPulses;
  lv_obj_t* labelRelayActive;
  lv_obj_t* labelMotorStatus;
} TenZillaMotorSettingsScreenUI;

void TenZillaMotorSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaMotorSettingsScreenUI* out_ui);

#endif
