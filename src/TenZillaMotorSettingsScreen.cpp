#include "TenZillaMotorSettingsScreen.h"
#include "TenZillaScale.h"
#include "ui/TenZillaMotorSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaMotorSettingsScreenUI s_ui;

void TenZillaMotorSettingsScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaMotorSettingsScreen_ui_create(&screen, &s_ui);
}

void TenZillaMotorSettingsScreen::updateLVGL(lv_obj_t* screen) {
  if (screen == nullptr || s_ui.labelDisplacement == nullptr) return;

  char buffer[64];
  static float lastDisplacement = -9999.0f;
  static int lastOpticalCount = -9999;
  static float lastStepMm = -9999.0f;
  static float lastEncoderMaxMm = -9999.0f;
  static int lastEncoderMaxPulses = -9999;
  static bool lastRelayActiveHigh = false;
  static bool lastMotorRunning = false;
  static int lastMotorDirection = 0;
  
  // Абсолютное перемещение - обновляем только при изменении
  float displacement = TenZillaScale::getDisplacement();
  if (fabs(displacement - lastDisplacement) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f mm", displacement);
    lv_label_set_text(s_ui.labelDisplacement, buffer);
    lastDisplacement = displacement;
  }
  
  // Оптический счетчик (импульсы) - обновляем только при изменении
  int opticalCount = TenZillaScale::getOpticalCount();
  if (opticalCount != lastOpticalCount) {
    snprintf(buffer, sizeof(buffer), "%d", opticalCount);
    if (s_ui.labelOpticalCount != nullptr) {
      lv_label_set_text(s_ui.labelOpticalCount, buffer);
    }
    lastOpticalCount = opticalCount;
  }
  
  // Шаг энкодера - обновляем только при изменении
  float stepMm = TenZillaScale::getEncoderStepMm();
  if (fabs(stepMm - lastStepMm) >= 0.0001f) {
    // Убираем завершающие нули
    char temp[32];
    snprintf(temp, sizeof(temp), "%.4f", stepMm);
    int len = strlen(temp);
    while (len > 1 && temp[len-1] == '0') len--;
    if (len > 1 && temp[len-1] == '.') len--;
    snprintf(buffer, sizeof(buffer), "%.*s mm", len, temp);
    if (s_ui.labelEncoderStep != nullptr) {
      lv_label_set_text(s_ui.labelEncoderStep, buffer);
    }
    lastStepMm = stepMm;
  }
  
  // Максимальное значение - обновляем только при изменении
  int encoderMaxPulses = TenZillaScale::getEncoderMax();
  float encoderMaxMm = encoderMaxPulses * stepMm;
  if (fabs(encoderMaxMm - lastEncoderMaxMm) >= 0.01f || encoderMaxPulses != lastEncoderMaxPulses) {
    char temp[32];
    snprintf(temp, sizeof(temp), "%.2f", encoderMaxMm);
    int len = strlen(temp);
    while (len > 1 && temp[len-1] == '0') len--;
    if (len > 1 && temp[len-1] == '.') len--;
    snprintf(buffer, sizeof(buffer), "%.*s mm", len, temp);
    if (s_ui.labelEncoderMax != nullptr) {
      lv_label_set_text(s_ui.labelEncoderMax, buffer);
    }
    lastEncoderMaxMm = encoderMaxMm;
    
    // Максимальное значение (в импульсах) - обновляем только при изменении
    if (encoderMaxPulses != lastEncoderMaxPulses) {
      snprintf(buffer, sizeof(buffer), "%d", encoderMaxPulses);
      if (s_ui.labelEncoderMaxPulses != nullptr) {
        lv_label_set_text(s_ui.labelEncoderMaxPulses, buffer);
      }
      lastEncoderMaxPulses = encoderMaxPulses;
    }
  }
  
  // Активный уровень реле - обновляем только при изменении
  bool relayActiveHigh = TenZillaScale::getRelayActiveHigh();
  if (relayActiveHigh != lastRelayActiveHigh) {
    if (s_ui.labelRelayActive != nullptr) {
      lv_label_set_text(s_ui.labelRelayActive, relayActiveHigh ? "HIGH" : "LOW");
      lv_color_t relayColor = relayActiveHigh ? lv_color_make(0, 255, 0) : lv_color_make(0, 0, 255);  /* BGR green / red */
      lv_obj_set_style_text_color(s_ui.labelRelayActive, relayColor, 0);
    }
    lastRelayActiveHigh = relayActiveHigh;
  }
  
  // Статус двигателя - обновляем только при изменении
  bool motorRunning = TenZillaScale::isMotorRunning();
  int motorDirection = TenZillaScale::getMotorDirection();
  if (motorRunning != lastMotorRunning || motorDirection != lastMotorDirection) {
    if (s_ui.labelMotorStatus != nullptr) {
      if (motorRunning) {
        if (motorDirection == 1) {
          lv_label_set_text(s_ui.labelMotorStatus, "UP");
          lv_obj_set_style_text_color(s_ui.labelMotorStatus, lv_color_make(0, 255, 0), 0);   /* BGR green */
        } else if (motorDirection == -1) {
          lv_label_set_text(s_ui.labelMotorStatus, "DOWN");
          lv_obj_set_style_text_color(s_ui.labelMotorStatus, lv_color_make(0, 255, 255), 0); /* BGR cyan */
        }
      } else {
        lv_label_set_text(s_ui.labelMotorStatus, "STOP");
        lv_obj_set_style_text_color(s_ui.labelMotorStatus, lv_color_make(0, 0, 255), 0);     /* BGR red */
      }
    }
    lastMotorRunning = motorRunning;
    lastMotorDirection = motorDirection;
  }
}
