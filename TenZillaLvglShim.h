#ifndef TENZILLA_LVGL_SHIM_H
#define TENZILLA_LVGL_SHIM_H

/**
 * Единая точка подключения LVGL для TenZilla (корень скетча — в include path Arduino IDE).
 * LovyanGFX (lgfx/v1/lvgl.h) при отсутствии lvgl/lvgl.h подключает совместимые
 * заголовки со своими типами — они конфликтуют с библиотекой lvgl 8.x.
 * Если M5GFX_USING_REAL_LVGL уже задан, LovyanGFX использует установленный LVGL.
 */
#ifndef M5GFX_USING_REAL_LVGL
#define M5GFX_USING_REAL_LVGL 1
#endif

#include <lvgl.h>

#endif // TENZILLA_LVGL_SHIM_H
