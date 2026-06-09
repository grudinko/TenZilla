# FontAwesome шрифт для LVGL

Этот шрифт содержит иконки FontAwesome для использования в LVGL интерфейсе:
- **WiFi** (U+F1EB) - для индикатора WiFi
- **Circle-up** (U+F0AA) - для иконки двигателя вверх
- **Circle-down** (U+F0AB) - для иконки двигателя вниз
- **Circle-stop** (U+F28D) - для иконки остановки двигателя

## Генерация через скрипт (Node.js)

В папке `src/fonts/`:

```bash
npm install
npm run gen:fa
```

Скрипт скачает FontAwesome Solid шрифт и сгенерирует:
- `lv_font_fontawesome_14.c` - для индикаторов (WiFi, CPU, FPS)
- `lv_font_fontawesome_48.c` - для крупных иконок двигателя

## Генерация через LVGL Font Converter (онлайн)

Если скрипт не работает (например, из-за формата WOFF2):

1. Откройте **https://lvgl.io/tools/fontconverter**
2. **Font**: загрузите `fa-solid-900.woff2` или `fa-solid-900.ttf` из [FontAwesome](https://fontawesome.com/download)
3. **Name**: `fontawesome_14` (для первого размера)
4. **Size**: 14, **Bpp**: 4
5. **Range**: добавьте следующие диапазоны:
   - `0xF0AA` (circle-up)
   - `0xF0AB` (circle-down)
   - `0xF1EB` (WiFi)
   - `0xF28D` (circle-stop)
6. Нажмите **Add** и добавьте второй шрифт:
   - `fontawesome_48`, size 48, bpp 4, те же range
7. **Convert** → скачайте `.c` файл
8. Сохраните как `lv_font_fontawesome_14.c` и `lv_font_fontawesome_48.c` в `src/fonts/`

## Подключение к проекту

1. Добавьте сгенерированные `.c` файлы в проект:
   - **Arduino IDE**: **Sketch → Add File** → выберите `lv_font_fontawesome_14.c` и `lv_font_fontawesome_48.c`
   - **PlatformIO**: файлы в `src/fonts/` обычно компилируются автоматически

2. Создайте заголовочный файл `src/fonts/lv_font_fontawesome.h`:

```cpp
#ifndef LV_FONT_FONTAWESOME_H
#define LV_FONT_FONTAWESOME_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Объявления шрифтов FontAwesome
extern const lv_font_t lv_font_fontawesome_14;
extern const lv_font_t lv_font_fontawesome_48;

// Макросы для удобного использования
#define LV_FONT_FA14 &lv_font_fontawesome_14
#define LV_FONT_FA48 &lv_font_fontawesome_48

// Unicode коды иконок
#define FA_WIFI      "\xEF\x87\xAB"  // U+F1EB
#define FA_CIRCLE_UP "\xEF\x82\xAA"  // U+F0AA
#define FA_CIRCLE_DOWN "\xEF\x82\xAB" // U+F0AB
#define FA_CIRCLE_STOP "\xEF\x8A\x8D" // U+F28D

#ifdef __cplusplus
}
#endif

#endif // LV_FONT_FONTAWESOME_H
```

3. Включите заголовочный файл в вашем коде:

```cpp
#include "fonts/lv_font_fontawesome.h"
```

4. Используйте шрифт и иконки:

```cpp
// Для индикатора WiFi
lv_obj_set_style_text_font(labelStatsWiFi, LV_FONT_FA14, 0);
lv_label_set_text(labelStatsWiFi, FA_WIFI " -50");

// Для иконки двигателя
lv_obj_set_style_text_font(labelMotorIcon, LV_FONT_FA48, 0);
lv_label_set_text(labelMotorIcon, FA_CIRCLE_UP);
```

## Примечания

- FontAwesome использует Unicode Private Use Area (0xF000-0xF8FF)
- Убедитесь, что в `lv_conf.h` включена поддержка нужных размеров шрифтов
- Если иконки не отображаются, проверьте, что шрифт правильно подключен и содержит нужные символы
