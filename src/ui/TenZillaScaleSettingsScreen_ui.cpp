/**
 * TenZilla Scale Settings Screen — дизайн.
 * Плитки слева направо, сверху вниз:
 * 1. Текущий вес  2. Текущее RAW  3. Factor  4. RAW нулевой точки
 * 5. N второй точки  6. RAW второй точки  7. Текущий шум  8. Макс. вес
 */

#include "TenZillaScaleSettingsScreen_ui.h"
#include "TenZillaLvglShim.h"

// Стиль плашек - размер, положение, отступы
struct TileStyle {
  int widthLeft;      // Ширина левой плашки
  int widthRight;     // Ширина правой плашки
  int height;         // Высота плашки
  int outerPadding;   // Внешний отступ от края экрана
  int innerGap;       // Разрыв между левой и правой плашками
  int labelOffsetLeft;   // Отступ заголовка от левого края (отрицательный для прижатия)
  int labelOffsetRight;  // Отступ значения от правого края (положительный для прижатия)
  int borderWidth;    // Толщина рамки
  int radius;         // Радиус скругления
};

// Стандартный стиль плашек
static const TileStyle TILE_STYLE = {
  .widthLeft = 227,      // Левая плашка
  .widthRight = 228,     // Правая плашка
  .height = 50,          // Высота
  .outerPadding = 10,    // Отступ от края экрана
  .innerGap = 5,         // Разрыв между плашками
  .labelOffsetLeft = -10, // Заголовок прижат к левому краю
  .labelOffsetRight = 10, // Значение прижато к правому краю
  .borderWidth = 2,      // Толщина рамки
  .radius = 8            // Радиус скругления
};

void TenZillaScaleSettingsScreen_ui_create(lv_obj_t** out_screen, TenZillaScaleSettingsScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  #define _F14 &lv_font_montserrat_14
  #define _F22 &lv_font_montserrat_22
  #define _F30 &lv_font_montserrat_30
  #define _TITLE "SCALE SETTINGS"
  
  // Позиции плашек (вычисляются из стиля)
  #define _X1 TILE_STYLE.outerPadding
  #define _X2 (TILE_STYLE.outerPadding + TILE_STYLE.widthLeft + TILE_STYLE.innerGap)
  #define _Y1 50
  #define _Y2 110
  #define _Y3 170
  #define _Y4 230

  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, _TITLE);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, _F22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  // Функция создания плашки с применением стиля
  auto make_tile = [&](int x, int y, uint32_t borderColor, int width) {
    lv_obj_t* c = lv_obj_create(screen);
    lv_obj_set_size(c, width, TILE_STYLE.height);
    lv_obj_align(c, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(borderColor), 0);
    lv_obj_set_style_border_width(c, TILE_STYLE.borderWidth, 0);
    lv_obj_set_style_radius(c, TILE_STYLE.radius, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
  };

  // 1. Текущий вес (N) - создаем вручную
  lv_obj_t* c1 = make_tile(_X1, _Y1, 0xFFFF00, TILE_STYLE.widthLeft);
  lv_obj_t* labelCurTitle = lv_label_create(c1);
  lv_label_set_text(labelCurTitle, "CUR:");
  lv_obj_set_style_text_color(labelCurTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCurTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelCurTitle, 1, 0);
  lv_obj_set_style_text_align(labelCurTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCurTitle, TILE_STYLE.widthLeft);
  lv_obj_align(labelCurTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelCurN = lv_label_create(c1);
  lv_label_set_text(out_ui->labelCurN, "0.0 N");
  lv_obj_set_style_text_color(out_ui->labelCurN, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(out_ui->labelCurN, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelCurN, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelCurN, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelCurN, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelCurN, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 2. Текущее RAW - создаем вручную
  lv_obj_t* c2 = make_tile(_X2, _Y1, 0x00FFFF, TILE_STYLE.widthRight);
  lv_obj_t* labelRawTitle = lv_label_create(c2);
  lv_label_set_text(labelRawTitle, "RAW:");
  lv_obj_set_style_text_color(labelRawTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelRawTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelRawTitle, 1, 0);
  lv_obj_set_style_text_align(labelRawTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelRawTitle, TILE_STYLE.widthRight);
  lv_obj_align(labelRawTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelRawValue = lv_label_create(c2);
  lv_label_set_text(out_ui->labelRawValue, "0");
  lv_obj_set_style_text_color(out_ui->labelRawValue, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_text_font(out_ui->labelRawValue, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelRawValue, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelRawValue, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelRawValue, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelRawValue, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 3. Factor - создаем вручную
  lv_obj_t* c3 = make_tile(_X1, _Y2, 0xFF00FF, TILE_STYLE.widthLeft);
  lv_obj_t* labelFactorTitle = lv_label_create(c3);
  lv_label_set_text(labelFactorTitle, "FACTOR:");
  lv_obj_set_style_text_color(labelFactorTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelFactorTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelFactorTitle, 1, 0);
  lv_obj_set_style_text_align(labelFactorTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelFactorTitle, TILE_STYLE.widthLeft);
  lv_obj_align(labelFactorTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelCalibrationFactor = lv_label_create(c3);
  lv_label_set_text(out_ui->labelCalibrationFactor, "0.00");
  lv_obj_set_style_text_color(out_ui->labelCalibrationFactor, lv_color_hex(0xFF00FF), 0);
  lv_obj_set_style_text_font(out_ui->labelCalibrationFactor, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelCalibrationFactor, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelCalibrationFactor, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelCalibrationFactor, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelCalibrationFactor, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 4. RAW нулевой точки - создаем вручную
  lv_obj_t* c4 = make_tile(_X2, _Y2, 0xAAAAAA, TILE_STYLE.widthRight);
  lv_obj_t* labelZeroTitle = lv_label_create(c4);
  lv_label_set_text(labelZeroTitle, "ZERO:");
  lv_obj_set_style_text_color(labelZeroTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelZeroTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelZeroTitle, 1, 0);
  lv_obj_set_style_text_align(labelZeroTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelZeroTitle, TILE_STYLE.widthRight);
  lv_obj_align(labelZeroTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelZeroRaw = lv_label_create(c4);
  lv_label_set_text(out_ui->labelZeroRaw, "0");
  lv_obj_set_style_text_color(out_ui->labelZeroRaw, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(out_ui->labelZeroRaw, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelZeroRaw, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelZeroRaw, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelZeroRaw, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelZeroRaw, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 5. Значение в N второй точки - создаем вручную
  lv_obj_t* c5 = make_tile(_X1, _Y3, 0xFF8800, TILE_STYLE.widthLeft);
  lv_obj_t* labelCalNTitle = lv_label_create(c5);
  lv_label_set_text(labelCalNTitle, "CAL N:");
  lv_obj_set_style_text_color(labelCalNTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCalNTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelCalNTitle, 1, 0);
  lv_obj_set_style_text_align(labelCalNTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCalNTitle, TILE_STYLE.widthLeft);
  lv_obj_align(labelCalNTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelCalN = lv_label_create(c5);
  lv_label_set_text(out_ui->labelCalN, "0.0 N");
  lv_obj_set_style_text_color(out_ui->labelCalN, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(out_ui->labelCalN, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelCalN, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelCalN, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelCalN, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelCalN, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 6. RAW второй точки - создаем вручную
  lv_obj_t* c6 = make_tile(_X2, _Y3, 0x00FF00, TILE_STYLE.widthRight);
  lv_obj_t* labelCalRawTitle = lv_label_create(c6);
  lv_label_set_text(labelCalRawTitle, "CAL RAW:");
  lv_obj_set_style_text_color(labelCalRawTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCalRawTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelCalRawTitle, 1, 0);
  lv_obj_set_style_text_align(labelCalRawTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCalRawTitle, TILE_STYLE.widthRight);
  lv_obj_align(labelCalRawTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelCalRaw = lv_label_create(c6);
  lv_label_set_text(out_ui->labelCalRaw, "0");
  lv_obj_set_style_text_color(out_ui->labelCalRaw, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(out_ui->labelCalRaw, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelCalRaw, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelCalRaw, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelCalRaw, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelCalRaw, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 7. Текущий шум (цвет зелёный/красный в updateLVGL) - создаем вручную
  lv_obj_t* c7 = make_tile(_X1, _Y4, 0xAAAAAA, TILE_STYLE.widthLeft);
  lv_obj_t* labelNoiseTitle = lv_label_create(c7);
  lv_label_set_text(labelNoiseTitle, "NOISE:");
  lv_obj_set_style_text_color(labelNoiseTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelNoiseTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelNoiseTitle, 1, 0);
  lv_obj_set_style_text_align(labelNoiseTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelNoiseTitle, TILE_STYLE.widthLeft);
  lv_obj_align(labelNoiseTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelNoise = lv_label_create(c7);
  lv_label_set_text(out_ui->labelNoise, "0.00%");
  lv_obj_set_style_text_color(out_ui->labelNoise, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(out_ui->labelNoise, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelNoise, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelNoise, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelNoise, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelNoise, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // 8. Максимальный вес (красный: BGR как значок стоп на сжатии) - создаем вручную
  lv_obj_t* c8 = make_tile(_X2, _Y4, 0x0000FF, TILE_STYLE.widthRight);
  lv_obj_t* labelMaxTitle = lv_label_create(c8);
  lv_label_set_text(labelMaxTitle, "MAX:");
  lv_obj_set_style_text_color(labelMaxTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMaxTitle, _F22, 0);
  lv_obj_set_style_text_letter_space(labelMaxTitle, 1, 0);
  lv_obj_set_style_text_align(labelMaxTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMaxTitle, TILE_STYLE.widthRight);
  lv_obj_align(labelMaxTitle, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  out_ui->labelMaxWeight = lv_label_create(c8);
  lv_label_set_text(out_ui->labelMaxWeight, "0.0 N");
  lv_obj_set_style_text_color(out_ui->labelMaxWeight, lv_color_hex(0x0000FF), 0);
  lv_obj_set_style_text_font(out_ui->labelMaxWeight, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelMaxWeight, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelMaxWeight, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelMaxWeight, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelMaxWeight, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Hold 2s for menu");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, _F14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

#undef _F14
#undef _F22
#undef _F30
#undef _TITLE
#undef _X1
#undef _X2
#undef _Y1
#undef _Y2
#undef _Y3
#undef _Y4

  *out_screen = screen;
}
