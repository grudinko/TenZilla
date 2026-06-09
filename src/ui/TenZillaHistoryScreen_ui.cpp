/**
 * TenZilla History Screen — список последних измерений (СЖАТИЕ/РАЗРЫВ).
 * Прокручиваемый список: дата, тип, результат, вес.
 */

#include "TenZillaHistoryScreen_ui.h"
#include "../TenZillaMeasurements.h"
#include "TenZillaLvglShim.h"
#include <time.h>

#define ROW_H   20
#define FONT    &lv_font_montserrat_14

static void add_row(lv_obj_t* parent, const char* text, uint32_t color) {
  lv_obj_t* lb = lv_label_create(parent);
  lv_label_set_text(lb, text);
  lv_obj_set_style_text_color(lb, lv_color_hex(color), 0);
  lv_obj_set_style_text_font(lb, FONT, 0);
  lv_obj_set_height(lb, ROW_H);
}

void TenZillaHistoryScreen_ui_fill_list(TenZillaHistoryScreenUI* ui) {
  if (ui == nullptr || ui->scrollCont == nullptr) return;
  lv_obj_clean(ui->scrollCont);

  int n = TenZillaMeasurements::getCount();
  if (n == 0) {
    add_row(ui->scrollCont, "No data", 0x888888);
    return;
  }

  char buf[48];
  for (int i = 0; i < n; i++) {
    uint32_t ts;
    uint8_t type, outcome;
    float w;
    TenZillaMeasurements::getEntry(i, ts, type, outcome, w);

    const char* typeStr = (type == 1) ? "CM" : "BR";
    const char* outStr = (outcome == 0) ? "OK" : ((outcome == 1) ? "ST" : "ER");
    uint32_t color = (outcome == 0) ? 0x00FF00 : ((outcome == 1) ? 0xFFAA00 : 0xFF4444);

    /* ASCII hyphens only (0x2D); avoid U+2014 em-dash missing glyph */
    char datePart[16] = "\x2D\x2D.\x2D\x2D \x2D\x2D:\x2D\x2D";
    if (ts > 0) {
      time_t t = (time_t)ts;
      struct tm ti;
      if (localtime_r(&t, &ti) != nullptr) {
        snprintf(datePart, sizeof(datePart), "%02d.%02d %02d:%02d",
                 ti.tm_mday, ti.tm_mon + 1, ti.tm_hour, ti.tm_min);
      }
    }
    snprintf(buf, sizeof(buf), "%s  %s %s  %5.1f N", datePart, typeStr, outStr, (double)w);
    add_row(ui->scrollCont, buf, color);
  }
}

void TenZillaHistoryScreen_ui_create(lv_obj_t** out_screen, TenZillaHistoryScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "HISTORY");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  lv_obj_t* scrollCont = lv_obj_create(screen);
  lv_obj_set_size(scrollCont, 300, 400);
  lv_obj_align(scrollCont, LV_ALIGN_TOP_LEFT, 10, 42);
  lv_obj_set_style_bg_opa(scrollCont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(scrollCont, lv_color_hex(0x404040), 0);
  lv_obj_set_style_border_width(scrollCont, 1, 0);
  lv_obj_set_style_radius(scrollCont, 6, 0);
  lv_obj_set_scroll_dir(scrollCont, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(scrollCont, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(scrollCont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(scrollCont, 2, 0);
  lv_obj_set_style_pad_column(scrollCont, 4, 0);

  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Hold 2s for menu");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

  out_ui->screen = screen;
  out_ui->scrollCont = scrollCont;
  *out_screen = screen;
}
