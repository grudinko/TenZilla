/**
 * Menu / Confirmation Screen — дизайн. Стиль как у OTHER SETTINGS.
 * 4 кнопки: START, RESET MOV, RESET ZERO, EXIT. START скрывается на экранах 3–6.
 */

#include "Confirmation_ui.h"
#include "TenZillaLvglShim.h"

static void make_btn(lv_obj_t* parent, lv_obj_t** out_btn, const char* label, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  lv_obj_t* c = lv_obj_create(parent);
  lv_obj_set_size(c, 220, 50);
  lv_obj_align(c, LV_ALIGN_TOP_LEFT, x, y);
  lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(c, lv_color_make(b, g, r), 0);
  lv_obj_set_style_border_width(c, 2, 0);
  lv_obj_set_style_radius(c, 8, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(c, 4, 0);

  lv_obj_t* lbl = lv_label_create(c);
  lv_label_set_text(lbl, label);
  lv_obj_set_style_text_color(lbl, lv_color_make(b, g, r), 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
  lv_obj_center(lbl);

  *out_btn = c;
}

void Confirmation_ui_create(lv_obj_t** out_screen, ConfirmationUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "MENU");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  make_btn(screen, &out_ui->btnStart,     "START",      20,  50, 0, 255, 0);    /* green */
  make_btn(screen, &out_ui->btnResetMov,  "RESET MOV", 250,  50, 0, 255, 255); /* cyan */
  make_btn(screen, &out_ui->btnResetZero, "RESET ZERO", 20, 110, 255, 136, 0); /* orange */
  make_btn(screen, &out_ui->btnExit,      "EXIT",      250, 110, 255, 0, 0);   /* red */

  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Short: next  Long: select");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

  out_ui->screen = screen;
  *out_screen = screen;
}
