/**
 * TenZilla Break Screen — дизайн.
 * Использует тот же дизайн что и Main Screen (COMPRESSION)
 * Только LVGL.
 */

#include "TenZillaBreakScreen_ui.h"
#include "TenZillaLvglShim.h"
// FontAwesome шрифт (если доступен)
#if defined(LV_FONT_FA14_ENABLED) || defined(LV_FONT_FA48_ENABLED) || defined(LV_FONT_FA60_ENABLED) || defined(LV_FONT_FA96_ENABLED)
  #include "../fonts/lv_font_fontawesome.h"
#endif

void TenZillaBreakScreen_ui_create(lv_obj_t** out_screen, TenZillaBreakScreenUI* out_ui) {
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
  #define _TITLE "BREAK"
  #define _CUR   "CUR."
  #define _MAX   "MAX."
  #define _MOV   "MOV."
  #define _LIM   "LIM:"
  #define _HINT  "Hold 2s for menu"

  // Заголовок: BGR-дисплей — lv_color_make(b, g, r), не lv_color_hex(RGB)
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, _TITLE);
  lv_obj_set_style_text_color(title, lv_color_make(0, 0, 255), 0);  // алый (красный)
  lv_obj_set_style_text_font(title, _F30, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  // CUR - левая рамка (желтый) - используем всё доступное пространство с разрывом 5px
  lv_obj_t* containerCur = lv_obj_create(screen);
  lv_obj_set_size(containerCur, 227, 50);  // Используем оставшееся место
  lv_obj_align(containerCur, LV_ALIGN_TOP_LEFT, 10, 50);
  lv_obj_set_style_bg_opa(containerCur, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerCur, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_border_width(containerCur, 2, 0);
  lv_obj_set_style_radius(containerCur, 8, 0);
  lv_obj_clear_flag(containerCur, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelCur = lv_label_create(containerCur);
  lv_label_set_text(labelCur, _CUR);
  lv_obj_set_style_text_color(labelCur, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelCur, _F22, 0);
  lv_obj_set_style_text_letter_space(labelCur, 1, 0);
  lv_obj_set_style_text_align(labelCur, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelCur, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCur, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelCurrentWeight = lv_label_create(containerCur);
  lv_label_set_text(labelCurrentWeight, "0.0 N");
  lv_obj_set_style_text_color(labelCurrentWeight, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(labelCurrentWeight, _F30, 0);
  lv_obj_set_style_text_letter_space(labelCurrentWeight, 1, 0);
  lv_obj_set_style_text_align(labelCurrentWeight, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelCurrentWeight, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelCurrentWeight, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки
  
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

  // MAX - контейнер с рамкой (оранжевый) - используем всё доступное пространство с разрывом 5px
  lv_obj_t* containerMax = lv_obj_create(screen);
  lv_obj_set_size(containerMax, 227, 50);  // Используем оставшееся место
  lv_obj_align(containerMax, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_obj_set_style_bg_opa(containerMax, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMax, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_border_width(containerMax, 2, 0);
  lv_obj_set_style_radius(containerMax, 8, 0);
  lv_obj_clear_flag(containerMax, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelMax = lv_label_create(containerMax);
  lv_label_set_text(labelMax, _MAX);
  lv_obj_set_style_text_color(labelMax, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMax, _F22, 0);
  lv_obj_set_style_text_letter_space(labelMax, 1, 0);
  lv_obj_set_style_text_align(labelMax, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMax, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMax, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelMaxWeight = lv_label_create(containerMax);
  lv_label_set_text(labelMaxWeight, "0.0 N");
  lv_obj_set_style_text_color(labelMaxWeight, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(labelMaxWeight, _F30, 0);
  lv_obj_set_style_text_letter_space(labelMaxWeight, 1, 0);
  lv_obj_set_style_text_align(labelMaxWeight, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelMaxWeight, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMaxWeight, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // LIM - контейнер справа от MAX (рамка с данными лимита) - используем оставшееся место с разрывом 5px
  lv_obj_t* containerLim = lv_obj_create(screen);
  lv_obj_set_size(containerLim, 228, 50);  // Используем оставшееся место (10 + 227 + 5 = 242)
  lv_obj_align(containerLim, LV_ALIGN_TOP_LEFT, 242, 110);  // Справа от MAX с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerLim, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerLim, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_border_width(containerLim, 2, 0);
  lv_obj_set_style_radius(containerLim, 8, 0);
  lv_obj_clear_flag(containerLim, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelLim = lv_label_create(containerLim);
  lv_label_set_text(labelLim, _LIM);
  lv_obj_set_style_text_color(labelLim, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(labelLim, _F22, 0);
  lv_obj_set_style_text_align(labelLim, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelLim, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelLim, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelLimVal = lv_label_create(containerLim);
  lv_label_set_text(labelLimVal, "\x2D\x2D\x2D");
  lv_obj_set_style_text_color(labelLimVal, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(labelLimVal, _F30, 0);
  lv_obj_set_style_text_align(labelLimVal, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelLimVal, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelLimVal, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // WRK - контейнер с рамкой (накопленное перемещение при разрыве) - используем всё доступное пространство с разрывом 5px
  lv_obj_t* containerWrk = lv_obj_create(screen);
  lv_obj_set_size(containerWrk, 227, 50);  // Используем оставшееся место
  lv_obj_align(containerWrk, LV_ALIGN_TOP_LEFT, 10, 170);
  lv_obj_set_style_bg_opa(containerWrk, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerWrk, lv_color_hex(0xFF00FF), 0);  // Пурпурный цвет
  lv_obj_set_style_border_width(containerWrk, 2, 0);
  lv_obj_set_style_radius(containerWrk, 8, 0);
  lv_obj_clear_flag(containerWrk, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelWrk = lv_label_create(containerWrk);
  lv_label_set_text(labelWrk, "WRK.");
  lv_obj_set_style_text_color(labelWrk, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelWrk, _F22, 0);
  lv_obj_set_style_text_letter_space(labelWrk, 1, 0);
  lv_obj_set_style_text_align(labelWrk, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelWrk, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelWrk, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelWorkingDisplacement = lv_label_create(containerWrk);
  lv_label_set_text(labelWorkingDisplacement, "0.0 mm");
  lv_obj_set_style_text_color(labelWorkingDisplacement, lv_color_hex(0xFF00FF), 0);  // Пурпурный цвет
  lv_obj_set_style_text_font(labelWorkingDisplacement, _F30, 0);
  lv_obj_set_style_text_letter_space(labelWorkingDisplacement, 1, 0);
  lv_obj_set_style_text_align(labelWorkingDisplacement, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelWorkingDisplacement, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelWorkingDisplacement, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // MOV - контейнер с рамкой (абсолютное перемещение) - используем оставшееся место с разрывом 5px
  lv_obj_t* containerMov = lv_obj_create(screen);
  lv_obj_set_size(containerMov, 228, 50);  // Используем оставшееся место (10 + 227 + 5 = 242)
  lv_obj_align(containerMov, LV_ALIGN_TOP_LEFT, 242, 170);  // Справа от WRK с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerMov, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMov, lv_color_hex(0x00FFFF), 0);  // Голубой цвет
  lv_obj_set_style_border_width(containerMov, 2, 0);
  lv_obj_set_style_radius(containerMov, 8, 0);
  lv_obj_clear_flag(containerMov, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_t* labelMov = lv_label_create(containerMov);
  lv_label_set_text(labelMov, _MOV);
  lv_obj_set_style_text_color(labelMov, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMov, _F22, 0);
  lv_obj_set_style_text_letter_space(labelMov, 1, 0);
  lv_obj_set_style_text_align(labelMov, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMov, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMov, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки
  
  lv_obj_t* labelDisplacement = lv_label_create(containerMov);
  lv_label_set_text(labelDisplacement, "0.0 mm");
  lv_obj_set_style_text_color(labelDisplacement, lv_color_hex(0x00FFFF), 0);  // Голубой цвет
  lv_obj_set_style_text_font(labelDisplacement, _F30, 0);
  lv_obj_set_style_text_letter_space(labelDisplacement, 1, 0);
  lv_obj_set_style_text_align(labelDisplacement, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelDisplacement, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelDisplacement, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Статус программы - между 3-й строкой (y=170) и нижним баром (y=-10), по центру
  lv_obj_t* labelStatus = lv_label_create(screen);
  lv_label_set_text(labelStatus, "");
  lv_obj_set_style_text_color(labelStatus, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStatus, _F30, 0);  // Увеличенный шрифт для статуса
  lv_obj_align(labelStatus, LV_ALIGN_TOP_MID, 0, 240);  // По центру, между 3-й строкой и нижним баром

  // Крупные символы направления движения — как на Сжатии: FA_CIRCLE_STOP при FA, иначе LV_SYMBOL_STOP
  lv_obj_t* labelMotorIcon = lv_label_create(screen);
  #if LV_FONT_FA60_ENABLED
    lv_obj_set_style_text_font(labelMotorIcon, LV_FONT_FA60, 0);
    lv_label_set_text(labelMotorIcon, FA_CIRCLE_STOP);
  #elif LV_FONT_FA48_ENABLED
    lv_obj_set_style_text_font(labelMotorIcon, LV_FONT_FA48, 0);
    lv_label_set_text(labelMotorIcon, FA_CIRCLE_STOP);
  #else
    lv_obj_set_style_text_font(labelMotorIcon, _F48, 0);
    lv_label_set_text(labelMotorIcon, LV_SYMBOL_STOP);
  #endif
  lv_obj_set_style_text_color(labelMotorIcon, lv_color_make(0, 0, 255), 0);
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
  out_ui->labelCurrentWeight = labelCurrentWeight;
  out_ui->labelMaxWeight = labelMaxWeight;
  out_ui->labelDisplacement = labelDisplacement;
  out_ui->labelWorkingDisplacement = labelWorkingDisplacement;
  out_ui->labelStatus = labelStatus;
  out_ui->labelMotorIcon = labelMotorIcon;
  out_ui->labelMotorIconCenter = labelMotorIconCenter;
  out_ui->labelLimVal = labelLimVal;
  out_ui->progressBarWeight = progressBarWeight;
  out_ui->labelProgressPercent = labelProgressPercent;
  *out_screen = screen;
}
