/**
 * TenZilla Calibration Screen — дизайн. Только LVGL.
 */

#include "TenZillaCalibrationScreen_ui.h"
#include "TenZillaLvglShim.h"

void TenZillaCalibrationScreen_ui_create(lv_obj_t** out_screen, TenZillaCalibrationScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "CALIBRATION");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

  static lv_point_t line_points_h[] = {{0, 0}, {310, 0}};
  static lv_point_t line_points_v[] = {{0, 0}, {0, 100}};

  lv_obj_t* line1 = lv_line_create(screen);
  lv_line_set_points(line1, line_points_h, 2);
  lv_obj_set_style_line_color(line1, lv_color_white(), 0);
  lv_obj_align(line1, LV_ALIGN_TOP_LEFT, 5, 25);

  lv_obj_t* line2 = lv_line_create(screen);
  lv_line_set_points(line2, line_points_h, 2);
  lv_obj_set_style_line_color(line2, lv_color_white(), 0);
  lv_obj_align(line2, LV_ALIGN_TOP_LEFT, 5, 125);

  lv_obj_t* lineV1 = lv_line_create(screen);
  lv_line_set_points(lineV1, line_points_v, 2);
  lv_obj_set_style_line_color(lineV1, lv_color_white(), 0);
  lv_obj_align(lineV1, LV_ALIGN_TOP_LEFT, 5, 25);

  lv_obj_t* lineV2 = lv_line_create(screen);
  lv_line_set_points(lineV2, line_points_v, 2);
  lv_obj_set_style_line_color(lineV2, lv_color_white(), 0);
  lv_obj_align(lineV2, LV_ALIGN_TOP_LEFT, 315, 25);

  lv_obj_t* labelCurrentTitle = lv_label_create(screen);
  lv_label_set_text(labelCurrentTitle, "Weight:");
  lv_obj_set_style_text_color(labelCurrentTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCurrentTitle, &lv_font_montserrat_18, 0);
  lv_obj_align(labelCurrentTitle, LV_ALIGN_TOP_LEFT, 15, 35);

  lv_obj_t* labelCurrentWeight = lv_label_create(screen);
  lv_label_set_text(labelCurrentWeight, "0.0 N");
  lv_obj_set_style_text_color(labelCurrentWeight, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(labelCurrentWeight, &lv_font_montserrat_18, 0);
  lv_obj_align(labelCurrentWeight, LV_ALIGN_TOP_LEFT, 100, 35);

  lv_obj_t* labelFactorTitle = lv_label_create(screen);
  lv_label_set_text(labelFactorTitle, "Factor:");
  lv_obj_set_style_text_color(labelFactorTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelFactorTitle, &lv_font_montserrat_18, 0);
  lv_obj_align(labelFactorTitle, LV_ALIGN_TOP_LEFT, 15, 60);

  lv_obj_t* labelCalibrationFactor = lv_label_create(screen);
  lv_label_set_text(labelCalibrationFactor, "0.0");
  lv_obj_set_style_text_color(labelCalibrationFactor, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCalibrationFactor, &lv_font_montserrat_18, 0);
  lv_obj_align(labelCalibrationFactor, LV_ALIGN_TOP_LEFT, 100, 60);

  lv_obj_t* labelStepTitle = lv_label_create(screen);
  lv_label_set_text(labelStepTitle, "Step:");
  lv_obj_set_style_text_color(labelStepTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStepTitle, &lv_font_montserrat_18, 0);
  lv_obj_align(labelStepTitle, LV_ALIGN_TOP_LEFT, 15, 85);

  lv_obj_t* labelStep = lv_label_create(screen);
  lv_label_set_text(labelStep, "0");
  lv_obj_set_style_text_color(labelStep, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStep, &lv_font_montserrat_18, 0);
  lv_obj_align(labelStep, LV_ALIGN_TOP_LEFT, 100, 85);

  lv_obj_t* instruction1 = lv_label_create(screen);
  lv_label_set_text(instruction1, "1. Remove weight & press TARE");
  lv_obj_set_style_text_color(instruction1, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(instruction1, &lv_font_montserrat_14, 0);
  lv_obj_align(instruction1, LV_ALIGN_TOP_LEFT, 15, 135);

  lv_obj_t* instruction2 = lv_label_create(screen);
  lv_label_set_text(instruction2, "2. Add 100 N & press CAL");
  lv_obj_set_style_text_color(instruction2, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(instruction2, &lv_font_montserrat_14, 0);
  lv_obj_align(instruction2, LV_ALIGN_TOP_LEFT, 15, 155);

  out_ui->screen = screen;
  out_ui->labelCurrentWeight = labelCurrentWeight;
  out_ui->labelCalibrationFactor = labelCalibrationFactor;
  out_ui->labelStep = labelStep;
  *out_screen = screen;
}
