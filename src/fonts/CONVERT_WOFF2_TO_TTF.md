# Конвертация WOFF2 → TTF

Онлайн-конвертер LVGL не поддерживает WOFF2. Нужно сначала конвертировать в TTF.

## Онлайн-конвертеры (выберите один):

1. **RouteNote Convert** (рекомендуется):
   - https://convert.routenote.com/woff2-to-ttf
   - Загрузите `src/fonts/fa-solid-900.woff2`
   - Скачайте TTF файл
   - Сохраните как `fa-solid-900.ttf` в `src/fonts/`

2. **CloudConvert**:
   - https://cloudconvert.com/woff2-to-ttf
   - Загрузите файл
   - Скачайте TTF

3. **AnyConv**:
   - https://anyconv.com/woff2-to-ttf-converter/
   - Загрузите файл
   - Скачайте TTF

## После конвертации:

1. Используйте `fa-solid-900.ttf` в онлайн-конвертере LVGL:
   - https://lvgl.io/tools/fontconverter
   - Загрузите TTF файл
   - Настройки:
     - Name: `fontawesome_14` (для первого) или `fontawesome_48` (для второго)
     - Size: `14` или `48`
     - Bpp: `4`
     - Range: `0xF0AA`, `0xF0AB`, `0xF1EB`, `0xF28D`
   - Convert → замените соответствующий .c файл
