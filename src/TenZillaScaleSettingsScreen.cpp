#include "TenZillaScaleSettingsScreen.h"
#include "TenZillaScale.h"
#include "ui/TenZillaScaleSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaScaleSettingsScreenUI s_ui;

void TenZillaScaleSettingsScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaScaleSettingsScreen_ui_create(&screen, &s_ui);
}

void TenZillaScaleSettingsScreen::updateLVGL(lv_obj_t* screen, float currentWeight) {
  if (screen == nullptr || s_ui.labelRawValue == nullptr) return;

  char buffer[64];
  static float lastWeight = -9999.0f;
  static long lastRawValue = -9999;
  static float lastFactor = -9999.0f;
  static long lastZeroRaw = -9999;
  static long lastCalRaw = -9999;
  static float lastNoise = -9999.0f;
  static float lastMaxWeight = -9999.0f;

  // 1. Текущий вес (N) - обновляем только при изменении
  if (s_ui.labelCurN != nullptr) {
    if (fabs(currentWeight - lastWeight) > 0.01f) {
      snprintf(buffer, sizeof(buffer), "%.1f N", currentWeight);
      lv_label_set_text(s_ui.labelCurN, buffer);
      lastWeight = currentWeight;
    }
  }

  // 2. Текущее RAW - обновляем только при изменении
  if (s_ui.labelRawValue != nullptr) {
    long rawValue = (long)TenZillaScale::getRawValue();
    if (rawValue != lastRawValue) {
      snprintf(buffer, sizeof(buffer), "%ld", rawValue);
      lv_label_set_text(s_ui.labelRawValue, buffer);
      lastRawValue = rawValue;
    }
  }

  // 3. Factor - обновляем только при изменении
  if (s_ui.labelCalibrationFactor != nullptr) {
    float factor = TenZillaScale::getCalibrationFactor();
    if (fabs(factor - lastFactor) > 0.001f) {
      snprintf(buffer, sizeof(buffer), "%.2f", factor);
      lv_label_set_text(s_ui.labelCalibrationFactor, buffer);
      lastFactor = factor;
    }
  }

  // 4. RAW нулевой точки - обновляем только при изменении
  if (s_ui.labelZeroRaw != nullptr) {
    long zeroRaw = TenZillaScale::getZeroRaw();
    if (zeroRaw != lastZeroRaw) {
      snprintf(buffer, sizeof(buffer), "%ld", zeroRaw);
      lv_label_set_text(s_ui.labelZeroRaw, buffer);
      lastZeroRaw = zeroRaw;
    }
  }

  // 5. Значение в N второй точки: N = rawDiff / factor (factor переводит RAW напрямую в ньютоны)
  if (s_ui.labelCalN != nullptr) {
    long calRaw = TenZillaScale::getCalibrationRaw();
    long zeroRaw = TenZillaScale::getZeroRaw();
    long diff = calRaw - zeroRaw;
    float factor = TenZillaScale::getCalibrationFactor();
    float calN = 0.0f;
    if (factor != 0.0f && diff != 0) {
      calN = (float)diff / factor;
      snprintf(buffer, sizeof(buffer), "%.1f N", calN);
    } else {
      snprintf(buffer, sizeof(buffer), "-");
    }
    lv_label_set_text(s_ui.labelCalN, buffer);
  }

  // 6. RAW второй точки - обновляем только при изменении
  if (s_ui.labelCalRaw != nullptr) {
    long calRaw = TenZillaScale::getCalibrationRaw();
    if (calRaw != lastCalRaw) {
      snprintf(buffer, sizeof(buffer), "%ld", calRaw);
      lv_label_set_text(s_ui.labelCalRaw, buffer);
      lastCalRaw = calRaw;
    }
  }

  // 7. Текущий шум — зелёный в допуске, красный вне - обновляем только при изменении
  if (s_ui.labelNoise != nullptr) {
    float noise = TenZillaScale::getNoiseLevel();
    float thresh = TenZillaScale::getNoiseThreshold();
    if (fabs(noise - lastNoise) > 0.01f) {
      snprintf(buffer, sizeof(buffer), "%.2f%%", (double)noise);
      lv_label_set_text(s_ui.labelNoise, buffer);
      lv_color_t color = (noise < thresh) ? lv_color_hex(0x00FF00) : lv_color_make(0, 0, 255);  /* BGR red */
      lv_obj_set_style_text_color(s_ui.labelNoise, color, 0);
      lastNoise = noise;
    }
  }

  // 8. Максимальный вес (N) - обновляем только при изменении
  if (s_ui.labelMaxWeight != nullptr) {
    float maxN = TenZillaScale::getMaxWeight();
    if (fabs(maxN - lastMaxWeight) > 0.01f) {
      snprintf(buffer, sizeof(buffer), "%.1f N", maxN);
      lv_label_set_text(s_ui.labelMaxWeight, buffer);
      lastMaxWeight = maxN;
    }
  }
}
