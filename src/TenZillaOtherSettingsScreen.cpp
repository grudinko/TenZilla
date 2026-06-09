#include "TenZillaOtherSettingsScreen.h"
#include "TenZillaProgram.h"
#include "ui/TenZillaOtherSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaOtherSettingsScreenUI s_ui;

void TenZillaOtherSettingsScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaOtherSettingsScreen_ui_create(&screen, &s_ui);
}

void TenZillaOtherSettingsScreen::updateLVGL(lv_obj_t* screen) {
  if (screen == nullptr || s_ui.labelCompThreshold == nullptr) return;

  char buffer[64];
  static float lastCompThreshold = -9999.0f;
  static float lastCompTarget = -9999.0f;
  static float lastBreakThreshold = -9999.0f;
  
  // Порог начала сжатия - обновляем только при изменении
  float compThreshold = TenZillaProgram::getCompressionStartThreshold();
  if (fabs(compThreshold - lastCompThreshold) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f N", compThreshold);
    lv_label_set_text(s_ui.labelCompThreshold, buffer);
    lastCompThreshold = compThreshold;
  }
  
  // Целевое перемещение - обновляем только при изменении
  float compTarget = TenZillaProgram::getCompressionTargetDisplacement();
  if (fabs(compTarget - lastCompTarget) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.0f mm", compTarget);
    if (s_ui.labelCompTarget != nullptr) {
      lv_label_set_text(s_ui.labelCompTarget, buffer);
    }
    lastCompTarget = compTarget;
  }
  
  // Порог падения при разрыве - обновляем только при изменении
  float breakThreshold = TenZillaProgram::getBreakDropThreshold();
  if (fabs(breakThreshold - lastBreakThreshold) >= 0.1f) {
    snprintf(buffer, sizeof(buffer), "%.1f%%", breakThreshold);
    if (s_ui.labelBreakThreshold != nullptr) {
      lv_label_set_text(s_ui.labelBreakThreshold, buffer);
    }
    lastBreakThreshold = breakThreshold;
  }
}
