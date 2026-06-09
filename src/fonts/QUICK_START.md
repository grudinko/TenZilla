# Быстрый старт: Генерация FontAwesome шрифтов

## ✅ Файл уже скопирован!

Файл `fa-solid-900.woff2` находится в папке `src/fonts/`

## ⚠️ ВАЖНО: Онлайн-конвертер LVGL не поддерживает WOFF2!

Нужно сначала конвертировать WOFF2 → TTF

## Шаг 1: Конвертация WOFF2 → TTF

**Используйте онлайн-конвертер:**

1. Откройте: **https://convert.routenote.com/woff2-to-ttf**
2. Нажмите **"Choose File"** → выберите `src/fonts/fa-solid-900.woff2`
3. Нажмите **"Convert"**
4. Скачайте файл и сохраните как `fa-solid-900.ttf` в папку `src/fonts/`

**Альтернативные конвертеры:**
- https://cloudconvert.com/woff2-to-ttf
- https://anyconv.com/woff2-to-ttf-converter/

## Шаг 2: Генерация шрифтов LVGL

1. Откройте: **https://lvgl.io/tools/fontconverter**

2. **Для 14px шрифта:**
   - Нажмите **"Choose File"** → выберите `src/fonts/fa-solid-900.ttf` (TTF, не WOFF2!)
   - **Name**: `fontawesome_14`
   - **Size**: `14`
   - **Bpp**: `4`
   - **Range**: Добавьте по одному (нажмите Add после каждого):
     - `0xF0AA` → Add
     - `0xF0AB` → Add  
     - `0xF1EB` → Add
     - `0xF28D` → Add
   - Нажмите **"Convert"**
   - Скачайте файл и **замените** `src/fonts/lv_font_fontawesome_14.c`

3. **Для 48px шрифта:**
   - Повторите шаги выше, но:
   - **Name**: `fontawesome_48`
   - **Size**: `48`
   - Те же **Range**
   - Замените `src/fonts/lv_font_fontawesome_48.c`

## Способ 2: Онлайн-конвертер WOFF2 → TTF

Если Python не установлен:

1. **Конвертируйте WOFF2 → TTF онлайн:**
   - https://convert.routenote.com/woff2-to-ttf
   - или https://cloudconvert.com/woff2-to-ttf
   - Загрузите `src/fonts/fa-solid-900.woff2`
   - Скачайте `fa-solid-900.ttf`
   - Сохраните в `src/fonts/`

2. **Используйте TTF в онлайн-конвертере LVGL:**
   - https://lvgl.io/tools/fontconverter
   - Загрузите `fa-solid-900.ttf` (те же настройки Range, Size, Bpp)

## После генерации

1. В `src/TenZillaPins.h` раскомментируйте:
   ```cpp
   #define LV_FONT_FA14_ENABLED 1
   #define LV_FONT_FA48_ENABLED 1
   ```

2. Пересоберите проект

3. Проверьте: должны появиться иконки FontAwesome вместо текста/стрелок
