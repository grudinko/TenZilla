#include "TenZillaCalibrationScreen.h"
#include "TenZillaScale.h"
#include "ui/TenZillaCalibrationScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaCalibrationScreenUI s_ui;

void TenZillaCalibrationScreen::drawStatic(void* tft) {
  (void)tft;
}

void TenZillaCalibrationScreen::updateData(void* tft, bool forceUpdate, float currentWeight, float calibrationFactor, int calibrationStep, float absoluteDisplacement) {
  (void)tft; (void)forceUpdate; (void)currentWeight; (void)calibrationFactor;
  (void)calibrationStep; (void)absoluteDisplacement;
}

// ============================================
// LVGL (логика; дизайн в ui/)
// ============================================

void TenZillaCalibrationScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaCalibrationScreen_ui_create(&screen, &s_ui);
}

void TenZillaCalibrationScreen::updateLVGL(lv_obj_t* screen, float currentWeight) {
  if (screen == nullptr || s_ui.labelCurrentWeight == nullptr) return;

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.1f N", currentWeight);
  lv_label_set_text(s_ui.labelCurrentWeight, buffer);

  float calibrationFactor = TenZillaScale::getCalibrationFactor();
  snprintf(buffer, sizeof(buffer), "%.1f", calibrationFactor);
  lv_label_set_text(s_ui.labelCalibrationFactor, buffer);

  int calibrationStep = TenZillaScale::isCalibrationInProgress() ? 1 : 0;
  snprintf(buffer, sizeof(buffer), "%d", calibrationStep);
  lv_label_set_text(s_ui.labelStep, buffer);
}
