# ✅ TTF файл найден! Генерация шрифтов

Файл `fa-solid-900.ttf` находится в папке `src/fonts/`

## Быстрая генерация через онлайн-конвертер:

### Для шрифта 14px:

1. Откройте: **https://lvgl.io/tools/fontconverter**
2. **Font**: Загрузите `fa-solid-900.ttf` из папки `src/fonts/`
3. **Name**: `fontawesome_14`
4. **Size**: `14`
5. **Bpp**: `4`
6. **Range**: Добавьте по одному (после каждого нажмите Add):
   - `0xF0AA` → Add
   - `0xF0AB` → Add
   - `0xF1EB` → Add
   - `0xF28D` → Add
7. Нажмите **"Convert"**
8. Скачайте файл
9. **Замените** `src/fonts/lv_font_fontawesome_14.c` скачанным файлом

### Для шрифта 48px:

1. Повторите те же шаги, но:
   - **Name**: `fontawesome_48`
   - **Size**: `48`
   - Те же **Range**
2. **Замените** `src/fonts/lv_font_fontawesome_48.c`

## После замены файлов:

1. В `src/TenZillaPins.h` раскомментируйте:
   ```cpp
   #define LV_FONT_FA14_ENABLED 1
   #define LV_FONT_FA48_ENABLED 1
   ```

2. Пересоберите проект

3. Готово! Иконки FontAwesome появятся на экране.
