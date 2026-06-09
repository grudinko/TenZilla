/**
 * TenZilla Main Screen (COMPRESSION) — дизайн.
 * Только LVGL.
 */

#include "TenZillaMainScreen_ui.h"
#include "TenZillaLvglShim.h"
// FontAwesome шрифт (если доступен)
#if defined(LV_FONT_FA14_ENABLED) || defined(LV_FONT_FA48_ENABLED) || defined(LV_FONT_FA60_ENABLED) || defined(LV_FONT_FA96_ENABLED)
  #include "../fonts/lv_font_fontawesome.h"
#endif

// Стиль плашек - размер, положение, отступы (унифицированный стиль)
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

void TenZillaMainScreen_ui_create(lv_obj_t** out_screen, TenZillaMainScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // Увеличенные шрифты (на ~20%): F18->F22, F24->F30
  // Используем ближайшие доступные размеры: F22 и F30
  #define _F14 &lv_font_montserrat_14
  #define _F22 &lv_font_montserrat_22  // Было F18, увеличение на 22%
  #define _F30 &lv_font_montserrat_30  // Было F24, увеличение на 25%
  #define _F48 &lv_font_montserrat_48  // Для крупных символов направления движения
  #define _TITLE "COMPRESSION"
  #define _CUR   "CUR."
  #define _MAX   "MAX."
  #define _MOV   "MOV."
  #define _LIM   "LIM:"
  #define _HINT  "Hold 2s for menu"

  // Позиции плашек (вычисляются из стиля во время выполнения)
  const int _X1 = TILE_STYLE.outerPadding;
  const int _X2 = TILE_STYLE.outerPadding + TILE_STYLE.widthLeft + TILE_STYLE.innerGap;
  const int _Y1 = 50;
  const int _Y2 = 110;
  const int _Y3 = 170;

  // Заголовок окна - увеличен и полужирный эффект через больший размер (левый угол)
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

  // CUR - левая рамка (желтый) - создаем вручную как в BreakScreen
  lv_obj_t* containerCur = make_tile(_X1, _Y1, 0xFFFF00, TILE_STYLE.widthLeft);
  
  lv_obj_t* labelCur = lv_label_create(containerCur);
  lv_label_set_text(labelCur, _CUR);
  lv_obj_set_style_text_color(labelCur, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCur, _F22, 0);
  lv_obj_set_style_text_letter_space(labelCur, 1, 0);
  lv_obj_set_style_text_align(labelCur, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCur, TILE_STYLE.widthLeft);
  lv_obj_align(labelCur, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  
  out_ui->labelCurrentWeight = lv_label_create(containerCur);
  lv_label_set_text(out_ui->labelCurrentWeight, "0.0 N");
  lv_obj_set_style_text_color(out_ui->labelCurrentWeight, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(out_ui->labelCurrentWeight, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelCurrentWeight, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelCurrentWeight, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelCurrentWeight, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelCurrentWeight, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);
  
  // CUR - правая рамка (прогресс-бар) - используем оставшееся место с разрывом 5px
  lv_obj_t* containerCurProgress = lv_obj_create(screen);
  lv_obj_set_size(containerCurProgress, 228, 50);  // Используем оставшееся место (10 + 227 + 5 = 242)
  lv_obj_align(containerCurProgress, LV_ALIGN_TOP_LEFT, 242, 50);  // Справа от CUR с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerCurProgress, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerCurProgress, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_border_width(containerCurProgress, 2, 0);
  lv_obj_set_style_radius(containerCurProgress, 8, 0);
  lv_obj_clear_flag(containerCurProgress, LV_OBJ_FLAG_SCROLLABLE);
  
  // Прогресс-бар размером как рамка, без рамки
  lv_obj_t* progressBarWeight = lv_bar_create(containerCurProgress);
  lv_obj_set_size(progressBarWeight, 228, 50);  // Размер как рамка
  lv_obj_align(progressBarWeight, LV_ALIGN_CENTER, 0, 0);  // По центру контейнера
  lv_bar_set_range(progressBarWeight, 0, 100);
  lv_bar_set_value(progressBarWeight, 0, LV_ANIM_OFF);
  
  // Стиль фона прогресс-бара (без рамки)
  lv_obj_set_style_bg_color(progressBarWeight, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(progressBarWeight, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(progressBarWeight, 8, LV_PART_MAIN);
  lv_obj_set_style_border_width(progressBarWeight, 0, LV_PART_MAIN);  // Без рамки
  
  // Начальный цвет индикатора (белый, будет обновляться динамически к красному)
  lv_obj_set_style_bg_color(progressBarWeight, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(progressBarWeight, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(progressBarWeight, 8, LV_PART_INDICATOR);
  
  // Метка процентов в центре прогресс-бара
  lv_obj_t* labelProgressPercent = lv_label_create(progressBarWeight);  // Создаем на прогресс-баре
  lv_label_set_text(labelProgressPercent, "0%");
  lv_obj_set_style_text_color(labelProgressPercent, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelProgressPercent, _F22, 0);
  lv_obj_align(labelProgressPercent, LV_ALIGN_CENTER, 0, 0);  // По центру

  // MAX - контейнер с рамкой (оранжевый) - создаем вручную
  lv_obj_t* containerMax = make_tile(_X1, _Y2, 0xFF8800, TILE_STYLE.widthLeft);
  
  lv_obj_t* labelMax = lv_label_create(containerMax);
  lv_label_set_text(labelMax, _MAX);
  lv_obj_set_style_text_color(labelMax, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMax, _F22, 0);
  lv_obj_set_style_text_letter_space(labelMax, 1, 0);
  lv_obj_set_style_text_align(labelMax, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMax, TILE_STYLE.widthLeft);
  lv_obj_align(labelMax, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  
  out_ui->labelMaxWeight = lv_label_create(containerMax);
  lv_label_set_text(out_ui->labelMaxWeight, "0.0 N");
  lv_obj_set_style_text_color(out_ui->labelMaxWeight, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(out_ui->labelMaxWeight, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelMaxWeight, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelMaxWeight, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelMaxWeight, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelMaxWeight, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // LIM - контейнер справа от MAX (рамка с данными лимита) - создаем вручную как другие плашки
  lv_obj_t* containerLim = make_tile(_X2, _Y2, 0xAAAAAA, TILE_STYLE.widthRight);
  
  lv_obj_t* labelLim = lv_label_create(containerLim);
  lv_label_set_text(labelLim, _LIM);
  lv_obj_set_style_text_color(labelLim, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelLim, _F22, 0);
  lv_obj_set_style_text_letter_space(labelLim, 1, 0);
  lv_obj_set_style_text_align(labelLim, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelLim, TILE_STYLE.widthRight);
  lv_obj_align(labelLim, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  
  out_ui->labelLimVal = lv_label_create(containerLim);
  lv_label_set_text(out_ui->labelLimVal, "\x2D\x2D\x2D");
  lv_obj_set_style_text_color(out_ui->labelLimVal, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(out_ui->labelLimVal, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelLimVal, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelLimVal, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelLimVal, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelLimVal, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // WRK - контейнер с рамкой (накопленное перемещение при сжатии) - создаем вручную как LIM
  lv_obj_t* containerWrk = make_tile(_X1, _Y3, 0xFF00FF, TILE_STYLE.widthLeft);
  
  lv_obj_t* labelWrk = lv_label_create(containerWrk);
  lv_label_set_text(labelWrk, "WRK.");
  lv_obj_set_style_text_color(labelWrk, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelWrk, _F22, 0);
  lv_obj_set_style_text_letter_space(labelWrk, 1, 0);
  lv_obj_set_style_text_align(labelWrk, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelWrk, TILE_STYLE.widthLeft);
  lv_obj_align(labelWrk, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  
  out_ui->labelWorkingDisplacement = lv_label_create(containerWrk);
  lv_label_set_text(out_ui->labelWorkingDisplacement, "0.0");
  lv_obj_set_style_text_color(out_ui->labelWorkingDisplacement, lv_color_hex(0xFF00FF), 0);
  lv_obj_set_style_text_font(out_ui->labelWorkingDisplacement, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelWorkingDisplacement, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelWorkingDisplacement, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelWorkingDisplacement, TILE_STYLE.widthLeft);
  lv_obj_align(out_ui->labelWorkingDisplacement, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // MOV - контейнер с рамкой (абсолютное перемещение) - создаем вручную
  lv_obj_t* containerMov = make_tile(_X2, _Y3, 0x00FFFF, TILE_STYLE.widthRight);
  
  lv_obj_t* labelMov = lv_label_create(containerMov);
  lv_label_set_text(labelMov, _MOV);
  lv_obj_set_style_text_color(labelMov, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMov, _F22, 0);
  lv_obj_set_style_text_letter_space(labelMov, 1, 0);
  lv_obj_set_style_text_align(labelMov, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMov, TILE_STYLE.widthRight);
  lv_obj_align(labelMov, LV_ALIGN_LEFT_MID, TILE_STYLE.labelOffsetLeft, 0);
  
  out_ui->labelDisplacement = lv_label_create(containerMov);
  lv_label_set_text(out_ui->labelDisplacement, "0");
  lv_obj_set_style_text_color(out_ui->labelDisplacement, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_text_font(out_ui->labelDisplacement, _F30, 0);
  lv_obj_set_style_text_letter_space(out_ui->labelDisplacement, 1, 0);
  lv_obj_set_style_text_align(out_ui->labelDisplacement, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(out_ui->labelDisplacement, TILE_STYLE.widthRight);
  lv_obj_align(out_ui->labelDisplacement, LV_ALIGN_RIGHT_MID, TILE_STYLE.labelOffsetRight, 0);

  // Статус программы - между 3-й строкой (y=170) и нижним баром (y=-10), по центру
  lv_obj_t* labelStatus = lv_label_create(screen);
  lv_label_set_text(labelStatus, "");
  lv_obj_set_style_text_color(labelStatus, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStatus, _F30, 0);  // Увеличенный шрифт для статуса
  lv_obj_align(labelStatus, LV_ALIGN_TOP_MID, 0, 240);  // По центру, между 3-й строкой и нижним баром

  // Крупные символы направления движения - на той же линии, что и статус
  // Используем шрифт 60px (увеличение на 20% от 48px)
  lv_obj_t* labelMotorIcon = lv_label_create(screen);
  lv_label_set_text(labelMotorIcon, LV_SYMBOL_STOP);
  lv_obj_set_style_text_color(labelMotorIcon, lv_color_make(0, 0, 255), 0);  /* BGR red, как стоп на сжатии */
  #if LV_FONT_FA60_ENABLED
    lv_obj_set_style_text_font(labelMotorIcon, LV_FONT_FA60, 0);  // 60px - увеличение на 20%
  #elif LV_FONT_FA48_ENABLED
    lv_obj_set_style_text_font(labelMotorIcon, LV_FONT_FA48, 0);  // 48px - fallback
  #else
    lv_obj_set_style_text_font(labelMotorIcon, _F48, 0);
  #endif
  lv_obj_align(labelMotorIcon, LV_ALIGN_TOP_RIGHT, -20, 230);
  lv_obj_t* labelMotorIconCenter = nullptr;

  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, _HINT);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, _F14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

#undef _F14
#undef _F22
#undef _F30
#undef _F48
#undef _TITLE
#undef _CUR
#undef _MAX
#undef _MOV
#undef _LIM
#undef _HINT

  out_ui->screen = screen;
  // labelCurrentWeight, labelMaxWeight, labelDisplacement уже заполнены через add_tile
  // labelWorkingDisplacement, labelLimVal уже заполнены выше
  out_ui->labelStatus = labelStatus;
  out_ui->labelMotorIcon = labelMotorIcon;
  out_ui->labelMotorIconCenter = labelMotorIconCenter;
  // labelLimVal уже заполнен выше
  out_ui->progressBarWeight = progressBarWeight;
  out_ui->labelProgressPercent = labelProgressPercent;
  *out_screen = screen;
}
