/**
 * TenZilla Motor Settings Screen — дизайн.
 * Использует тот же стиль что и Main Screen (COMPRESSION)
 * Только LVGL.
 */

#include "TenZillaMotorSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

void TenZillaMotorSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaMotorSettingsScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  #define _F14 &lv_font_montserrat_14
  #define _F22 &lv_font_montserrat_22
  #define _F30 &lv_font_montserrat_30
  #define _TITLE "MOTOR SETTINGS"

  // Заголовок (BGR как значок стоп на сжатии)
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, _TITLE);
  lv_obj_set_style_text_color(title, lv_color_make(0, 136, 255), 0);  /* BGR orange */
  lv_obj_set_style_text_font(title, _F22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  // Абсолютное перемещение (BGR)
  lv_obj_t* containerDisp = lv_obj_create(screen);
  lv_obj_set_size(containerDisp, 227, 50);
  lv_obj_align(containerDisp, LV_ALIGN_TOP_LEFT, 10, 50);
  lv_obj_set_style_bg_opa(containerDisp, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerDisp, lv_color_make(0, 255, 255), 0);  /* BGR cyan */
  lv_obj_set_style_border_width(containerDisp, 2, 0);
  lv_obj_set_style_radius(containerDisp, 8, 0);
  lv_obj_clear_flag(containerDisp, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelDispTitle = lv_label_create(containerDisp);
  lv_label_set_text(labelDispTitle, "DISP:");
  lv_obj_set_style_text_color(labelDispTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelDispTitle, _F22, 0);
  lv_obj_set_style_text_align(labelDispTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelDispTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelDispTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelDisplacement = lv_label_create(containerDisp);
  lv_label_set_text(labelDisplacement, "0.0 mm");
  lv_obj_set_style_text_color(labelDisplacement, lv_color_make(0, 255, 255), 0);  /* BGR cyan */
  lv_obj_set_style_text_font(labelDisplacement, _F30, 0);
  lv_obj_set_style_text_align(labelDisplacement, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelDisplacement, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelDisplacement, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Оптический счетчик (BGR)
  lv_obj_t* containerCount = lv_obj_create(screen);
  lv_obj_set_size(containerCount, 228, 50);
  lv_obj_align(containerCount, LV_ALIGN_TOP_LEFT, 242, 50);  // Справа от DISP с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerCount, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerCount, lv_color_make(0, 255, 255), 0);  /* BGR cyan */
  lv_obj_set_style_border_width(containerCount, 2, 0);
  lv_obj_set_style_radius(containerCount, 8, 0);
  lv_obj_clear_flag(containerCount, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelCountTitle = lv_label_create(containerCount);
  lv_label_set_text(labelCountTitle, "COUNT:");
  lv_obj_set_style_text_color(labelCountTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCountTitle, _F22, 0);
  lv_obj_set_style_text_align(labelCountTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCountTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCountTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelOpticalCount = lv_label_create(containerCount);
  lv_label_set_text(labelOpticalCount, "0");
  lv_obj_set_style_text_color(labelOpticalCount, lv_color_make(0, 255, 255), 0);  /* BGR cyan */
  lv_obj_set_style_text_font(labelOpticalCount, _F30, 0);
  lv_obj_set_style_text_align(labelOpticalCount, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelOpticalCount, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelOpticalCount, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Шаг энкодера (BGR), без "mm"
  lv_obj_t* containerStep = lv_obj_create(screen);
  lv_obj_set_size(containerStep, 227, 50);
  lv_obj_align(containerStep, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_obj_set_style_bg_opa(containerStep, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerStep, lv_color_make(0, 136, 255), 0);  /* BGR orange */
  lv_obj_set_style_border_width(containerStep, 2, 0);
  lv_obj_set_style_radius(containerStep, 8, 0);
  lv_obj_clear_flag(containerStep, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelStepTitle = lv_label_create(containerStep);
  lv_label_set_text(labelStepTitle, "STEP:");
  lv_obj_set_style_text_color(labelStepTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStepTitle, _F22, 0);
  lv_obj_set_style_text_align(labelStepTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelStepTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelStepTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelEncoderStep = lv_label_create(containerStep);
  lv_label_set_text(labelEncoderStep, "0.0000");
  lv_obj_set_style_text_color(labelEncoderStep, lv_color_make(0, 136, 255), 0);  /* BGR orange */
  lv_obj_set_style_text_font(labelEncoderStep, _F30, 0);
  lv_obj_set_style_text_align(labelEncoderStep, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelEncoderStep, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelEncoderStep, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Максимальное значение (без размерности)
  lv_obj_t* containerMax = lv_obj_create(screen);
  lv_obj_set_size(containerMax, 227, 50);
  lv_obj_align(containerMax, LV_ALIGN_TOP_LEFT, 10, 170);
  lv_obj_set_style_bg_opa(containerMax, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMax, lv_color_make(0, 0, 255), 0);  /* BGR red */
  lv_obj_set_style_border_width(containerMax, 2, 0);
  lv_obj_set_style_radius(containerMax, 8, 0);
  lv_obj_clear_flag(containerMax, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelMaxTitle = lv_label_create(containerMax);
  lv_label_set_text(labelMaxTitle, "MAX:");
  lv_obj_set_style_text_color(labelMaxTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMaxTitle, _F22, 0);
  lv_obj_set_style_text_align(labelMaxTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMaxTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMaxTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelEncoderMax = lv_label_create(containerMax);
  lv_label_set_text(labelEncoderMax, "0.00");
  lv_obj_set_style_text_color(labelEncoderMax, lv_color_make(0, 0, 255), 0);  /* BGR red */
  lv_obj_set_style_text_font(labelEncoderMax, _F30, 0);
  lv_obj_set_style_text_align(labelEncoderMax, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelEncoderMax, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelEncoderMax, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Максимальное значение (в импульсах) — BGR gray
  lv_obj_t* containerMaxPulses = lv_obj_create(screen);
  lv_obj_set_size(containerMaxPulses, 228, 50);
  lv_obj_align(containerMaxPulses, LV_ALIGN_TOP_LEFT, 242, 170);  // Справа от MAX с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerMaxPulses, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMaxPulses, lv_color_make(170, 170, 170), 0);  /* BGR gray */
  lv_obj_set_style_border_width(containerMaxPulses, 2, 0);
  lv_obj_set_style_radius(containerMaxPulses, 8, 0);
  lv_obj_clear_flag(containerMaxPulses, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelMaxPulsesTitle = lv_label_create(containerMaxPulses);
  lv_label_set_text(labelMaxPulsesTitle, "MAX P:");
  lv_obj_set_style_text_color(labelMaxPulsesTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMaxPulsesTitle, _F22, 0);
  lv_obj_set_style_text_align(labelMaxPulsesTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMaxPulsesTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMaxPulsesTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelEncoderMaxPulses = lv_label_create(containerMaxPulses);
  lv_label_set_text(labelEncoderMaxPulses, "0");
  lv_obj_set_style_text_color(labelEncoderMaxPulses, lv_color_make(170, 170, 170), 0);  /* BGR gray */
  lv_obj_set_style_text_font(labelEncoderMaxPulses, _F30, 0);
  lv_obj_set_style_text_align(labelEncoderMaxPulses, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelEncoderMaxPulses, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelEncoderMaxPulses, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Активный уровень реле (BGR)
  lv_obj_t* containerRelay = lv_obj_create(screen);
  lv_obj_set_size(containerRelay, 227, 50);
  lv_obj_align(containerRelay, LV_ALIGN_TOP_LEFT, 10, 230);
  lv_obj_set_style_bg_opa(containerRelay, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerRelay, lv_color_make(0, 255, 0), 0);  /* BGR green */
  lv_obj_set_style_border_width(containerRelay, 2, 0);
  lv_obj_set_style_radius(containerRelay, 8, 0);
  lv_obj_clear_flag(containerRelay, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelRelayTitle = lv_label_create(containerRelay);
  lv_label_set_text(labelRelayTitle, "RELAY:");
  lv_obj_set_style_text_color(labelRelayTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelRelayTitle, _F22, 0);
  lv_obj_set_style_text_align(labelRelayTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelRelayTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelRelayTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelRelayActive = lv_label_create(containerRelay);
  lv_label_set_text(labelRelayActive, "HIGH");
  lv_obj_set_style_text_color(labelRelayActive, lv_color_make(0, 255, 0), 0);  /* BGR green */
  lv_obj_set_style_text_font(labelRelayActive, _F30, 0);
  lv_obj_set_style_text_align(labelRelayActive, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelRelayActive, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelRelayActive, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Статус двигателя (BGR) - напротив STEP
  lv_obj_t* containerMotor = lv_obj_create(screen);
  lv_obj_set_size(containerMotor, 228, 50);
  lv_obj_align(containerMotor, LV_ALIGN_TOP_LEFT, 242, 110);  // Справа от STEP с разрывом 5px (10 + 227 + 5 = 242)
  
  // Подсказка
  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Hold 2s for menu");
  lv_obj_set_style_text_color(hint, lv_color_make(102, 102, 102), 0);  /* BGR gray */
  lv_obj_set_style_text_font(hint, _F14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_set_style_bg_opa(containerMotor, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMotor, lv_color_make(0, 255, 255), 0);  /* BGR yellow */
  lv_obj_set_style_border_width(containerMotor, 2, 0);
  lv_obj_set_style_radius(containerMotor, 8, 0);
  lv_obj_clear_flag(containerMotor, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelMotorTitle = lv_label_create(containerMotor);
  lv_label_set_text(labelMotorTitle, "MOTOR:");
  lv_obj_set_style_text_color(labelMotorTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMotorTitle, _F22, 0);
  lv_obj_set_style_text_align(labelMotorTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMotorTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMotorTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelMotorStatus = lv_label_create(containerMotor);
  lv_label_set_text(labelMotorStatus, "STOP");
  lv_obj_set_style_text_color(labelMotorStatus, lv_color_make(0, 0, 255), 0);  /* BGR red */
  lv_obj_set_style_text_font(labelMotorStatus, _F30, 0);
  lv_obj_set_style_text_align(labelMotorStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelMotorStatus, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMotorStatus, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

#undef _F14
#undef _F22
#undef _F30
#undef _TITLE

  out_ui->screen = screen;
  out_ui->labelDisplacement = labelDisplacement;
  out_ui->labelOpticalCount = labelOpticalCount;
  out_ui->labelEncoderStep = labelEncoderStep;
  out_ui->labelEncoderMax = labelEncoderMax;
  out_ui->labelEncoderMaxPulses = labelEncoderMaxPulses;
  out_ui->labelRelayActive = labelRelayActive;
  out_ui->labelMotorStatus = labelMotorStatus;
  *out_screen = screen;
}
