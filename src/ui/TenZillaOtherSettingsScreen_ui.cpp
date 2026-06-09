/**
 * TenZilla Other Settings Screen — дизайн.
 * Использует тот же стиль что и Main Screen (COMPRESSION)
 * Только LVGL.
 */

#include "TenZillaOtherSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

void TenZillaOtherSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaOtherSettingsScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  #define _F14 &lv_font_montserrat_14
  #define _F22 &lv_font_montserrat_22
  #define _F30 &lv_font_montserrat_30
  #define _TITLE "OTHER SETTINGS"

  // Заголовок
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, _TITLE);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, _F22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  // Порог начала накопления для сжатия
  lv_obj_t* containerCompThresh = lv_obj_create(screen);
  lv_obj_set_size(containerCompThresh, 227, 50);
  lv_obj_align(containerCompThresh, LV_ALIGN_TOP_LEFT, 10, 50);
  lv_obj_set_style_bg_opa(containerCompThresh, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerCompThresh, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_border_width(containerCompThresh, 2, 0);
  lv_obj_set_style_radius(containerCompThresh, 8, 0);
  lv_obj_clear_flag(containerCompThresh, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelCompThreshTitle = lv_label_create(containerCompThresh);
  lv_label_set_text(labelCompThreshTitle, "CM TH:");
  lv_obj_set_style_text_color(labelCompThreshTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCompThreshTitle, _F22, 0);
  lv_obj_set_style_text_align(labelCompThreshTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCompThreshTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCompThreshTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelCompThreshold = lv_label_create(containerCompThresh);
  lv_label_set_text(labelCompThreshold, "0.0N");
  lv_obj_set_style_text_color(labelCompThreshold, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(labelCompThreshold, _F30, 0);
  lv_obj_set_style_text_align(labelCompThreshold, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelCompThreshold, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCompThreshold, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Целевое перемещение для сжатия
  lv_obj_t* containerCompTarget = lv_obj_create(screen);
  lv_obj_set_size(containerCompTarget, 228, 50);
  lv_obj_align(containerCompTarget, LV_ALIGN_TOP_LEFT, 242, 50);  // Справа от CM TH с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerCompTarget, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerCompTarget, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_border_width(containerCompTarget, 2, 0);
  lv_obj_set_style_radius(containerCompTarget, 8, 0);
  lv_obj_clear_flag(containerCompTarget, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelCompTargetTitle = lv_label_create(containerCompTarget);
  lv_label_set_text(labelCompTargetTitle, "CM TG:");
  lv_obj_set_style_text_color(labelCompTargetTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCompTargetTitle, _F22, 0);
  lv_obj_set_style_text_align(labelCompTargetTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCompTargetTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCompTargetTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelCompTarget = lv_label_create(containerCompTarget);
  lv_label_set_text(labelCompTarget, "0mm");
  lv_obj_set_style_text_color(labelCompTarget, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(labelCompTarget, _F30, 0);
  lv_obj_set_style_text_align(labelCompTarget, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelCompTarget, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCompTarget, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Порог падения для разрыва
  lv_obj_t* containerBreakThresh = lv_obj_create(screen);
  lv_obj_set_size(containerBreakThresh, 227, 50);
  lv_obj_align(containerBreakThresh, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_obj_set_style_bg_opa(containerBreakThresh, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerBreakThresh, lv_color_make(0, 0, 255), 0);  /* BGR red */
  lv_obj_set_style_border_width(containerBreakThresh, 2, 0);
  lv_obj_set_style_radius(containerBreakThresh, 8, 0);
  lv_obj_clear_flag(containerBreakThresh, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelBreakThreshTitle = lv_label_create(containerBreakThresh);
  lv_label_set_text(labelBreakThreshTitle, "BRK TH:");
  lv_obj_set_style_text_color(labelBreakThreshTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelBreakThreshTitle, _F22, 0);
  lv_obj_set_style_text_align(labelBreakThreshTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelBreakThreshTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelBreakThreshTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelBreakThreshold = lv_label_create(containerBreakThresh);
  lv_label_set_text(labelBreakThreshold, "0.0%");
  lv_obj_set_style_text_color(labelBreakThreshold, lv_color_make(0, 0, 255), 0);  /* BGR red */
  lv_obj_set_style_text_font(labelBreakThreshold, _F30, 0);
  lv_obj_set_style_text_align(labelBreakThreshold, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelBreakThreshold, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelBreakThreshold, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Подсказка
  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Hold 2s for menu");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, _F14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

#undef _F14
#undef _F22
#undef _F30
#undef _TITLE

  out_ui->screen = screen;
  out_ui->labelCompThreshold = labelCompThreshold;
  out_ui->labelCompTarget = labelCompTarget;
  out_ui->labelBreakThreshold = labelBreakThreshold;
  *out_screen = screen;
}
