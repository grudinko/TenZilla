#ifndef LV_FONT_FONTAWESOME_H
#define LV_FONT_FONTAWESOME_H

#include "TenZillaLvglShim.h"

#ifdef __cplusplus
extern "C" {
#endif

// Проверяем, включены ли шрифты FontAwesome
// Определения должны быть в TenZillaPins.h или другом конфигурационном файле
#ifndef LV_FONT_FA14_ENABLED
  #define LV_FONT_FA14_ENABLED 0
#endif

#ifndef LV_FONT_FA48_ENABLED
  #define LV_FONT_FA48_ENABLED 0
#endif

#ifndef LV_FONT_FA60_ENABLED
  #define LV_FONT_FA60_ENABLED 0
#endif

#ifndef LV_FONT_FA96_ENABLED
  #define LV_FONT_FA96_ENABLED 0
#endif

// Объявления шрифтов FontAwesome (определены в .c файлах)
// Примечание: Созданы заглушки. Для реальных иконок сгенерируйте шрифты через онлайн-конвертер
extern const lv_font_t lv_font_fontawesome_14;
extern const lv_font_t lv_font_fontawesome_48;
extern const lv_font_t lv_font_fontawesome_60;
extern const lv_font_t lv_font_fontawesome_96;

#if LV_FONT_FA14_ENABLED
#define LV_FONT_FA14 &lv_font_fontawesome_14
#else
#define LV_FONT_FA14 &lv_font_montserrat_14  // Fallback на стандартный шрифт
#endif

#if LV_FONT_FA48_ENABLED
#define LV_FONT_FA48 &lv_font_fontawesome_48
#else
#define LV_FONT_FA48 &lv_font_montserrat_48  // Fallback на стандартный шрифт
#endif

#if LV_FONT_FA60_ENABLED
#define LV_FONT_FA60 &lv_font_fontawesome_60  // 60px - увеличение на 20% от 48px
#else
#define LV_FONT_FA60 &lv_font_montserrat_48  // Fallback на стандартный шрифт
#endif

#if LV_FONT_FA96_ENABLED
#define LV_FONT_FA96 &lv_font_fontawesome_96
#else
#define LV_FONT_FA96 &lv_font_montserrat_48  // Fallback на стандартный шрифт (если нет 96px)
#endif

// Unicode коды иконок FontAwesome (UTF-8)
#define FA_WIFI      "\xEF\x87\xAB"  // U+F1EB - WiFi icon
#define FA_CIRCLE_UP "\xEF\x82\xAA"  // U+F0AA - Circle arrow up
#define FA_CIRCLE_DOWN "\xEF\x82\xAB" // U+F0AB - Circle arrow down
#define FA_CIRCLE_STOP "\xEF\x8A\x8D" // U+F28D - Circle stop

#ifdef __cplusplus
}
#endif

#endif // LV_FONT_FONTAWESOME_H
