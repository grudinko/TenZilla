/**
 * Splash Screen — слой дизайна (UI)
 * Экран загрузки с версией, датой и прогресс-баром.
 */

#ifndef SPLASH_UI_H
#define SPLASH_UI_H

#include "TenZillaLvglShim.h"

/** Структура для хранения указателей на элементы UI заставки */
struct SplashUI {
  lv_obj_t* screen;           // Корневой экран
  lv_obj_t* labelVersion;     // Метка версии
  lv_obj_t* labelDate;        // Метка даты
  lv_obj_t* progressBar;      // Прогресс-бар загрузки
};

/** Создаёт сплэш-экран. Заполняет структуру UI. */
void Splash_ui_create(SplashUI* out_ui);

#endif
