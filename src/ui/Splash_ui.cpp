/**
 * Splash Screen — дизайн. Только LVGL.
 * Заставка с версией, датой и прогресс-баром.
 */

#include "Splash_ui.h"
#include "../TenZillaVersion.h"
#include "TenZillaLvglShim.h"

void Splash_ui_create(SplashUI* out_ui) {
  if (out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  // Заголовок
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "TenZilla");
  lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -80);

  // Подзаголовок
  lv_obj_t* subtitle = lv_label_create(screen);
  lv_label_set_text(subtitle, "Control System");
  lv_obj_set_style_text_color(subtitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_18, 0);
  lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, -40);

  // Версия релиза
  lv_obj_t* labelVersion = lv_label_create(screen);
  lv_label_set_text(labelVersion, "Release " TENZILLA_RELEASE_NUMBER);
  lv_obj_set_style_text_color(labelVersion, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_text_font(labelVersion, &lv_font_montserrat_18, 0);
  lv_obj_align(labelVersion, LV_ALIGN_CENTER, 0, 0);

  // Дата релиза
  lv_obj_t* labelDate = lv_label_create(screen);
  lv_label_set_text(labelDate, TENZILLA_RELEASE_DATE);
  lv_obj_set_style_text_color(labelDate, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(labelDate, &lv_font_montserrat_14, 0);
  lv_obj_align(labelDate, LV_ALIGN_CENTER, 0, 30);

  // Прогресс-бар загрузки (внизу экрана)
  lv_obj_t* progressBar = lv_bar_create(screen);
  lv_obj_set_size(progressBar, 300, 25);  // Увеличен размер для лучшей видимости
  lv_obj_align(progressBar, LV_ALIGN_BOTTOM_MID, 0, -20);  // Поднят выше
  
  // Настройка диапазона и начального значения
  lv_bar_set_range(progressBar, 0, 100);
  lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);
  
  // Стиль фона (основная часть) - темно-серый с границей
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(progressBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(progressBar, 12, LV_PART_MAIN);
  lv_obj_set_style_border_width(progressBar, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(progressBar, lv_color_hex(0x666666), LV_PART_MAIN);
  lv_obj_set_style_border_opa(progressBar, LV_OPA_COVER, LV_PART_MAIN);
  
  // Стиль индикатора (заполненная часть) - ярко-зеленый
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(progressBar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(progressBar, 10, LV_PART_INDICATOR);
  
  // Убеждаемся, что прогресс-бар виден и не скрыт
  lv_obj_clear_flag(progressBar, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(progressBar, LV_OBJ_FLAG_CLICKABLE);  // Не кликабельный
  
  // Устанавливаем минимальную ширину индикатора для видимости даже при 0%
  lv_obj_set_style_min_width(progressBar, 5, LV_PART_INDICATOR);

  // Статус загрузки
  lv_obj_t* status = lv_label_create(screen);
  lv_label_set_text(status, "Initializing...");
  lv_obj_set_style_text_color(status, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(status, &lv_font_montserrat_14, 0);
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -60);

  // Заполняем структуру
  out_ui->screen = screen;
  out_ui->labelVersion = labelVersion;
  out_ui->labelDate = labelDate;
  out_ui->progressBar = progressBar;
}
