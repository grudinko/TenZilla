# Пошаговая инструкция: Генерация FontAwesome шрифтов

## Шаг 1: Конвертация WOFF2 → TTF

У вас уже есть `fa-solid-900.woff2` в папке `src/fonts/`

**Вариант A: Онлайн-конвертер (РЕКОМЕНДУЕТСЯ)**

1. Откройте: **https://convert.routenote.com/woff2-to-ttf**
2. Нажмите **"Choose File"** → выберите `src/fonts/fa-solid-900.woff2`
3. Нажмите **"Convert"**
4. Скачайте файл и сохраните как `fa-solid-900.ttf` в папку `src/fonts/`

**Вариант B: Другой конвертер**
- https://cloudconvert.com/woff2-to-ttf
- https://anyconv.com/woff2-to-ttf-converter/

## Шаг 2: Генерация шрифтов LVGL

1. Откройте: **https://lvgl.io/tools/fontconverter**

2. **Для шрифта 14px:**
   - **Font**: Загрузите `fa-solid-900.ttf` (из `src/fonts/`)
   - **Name**: `fontawesome_14`
   - **Size**: `14`
   - **Bpp**: `4`
   - **Range**: Добавьте по одному (после каждого нажмите Add):
     - `0xF0AA` → Add
     - `0xF0AB` → Add
     - `0xF1EB` → Add
     - `0xF28D` → Add
   - Нажмите **"Convert"**
   - Скачайте файл
   - **Замените** `src/fonts/lv_font_fontawesome_14.c` скачанным файлом

3. **Для шрифта 48px:**
   - Повторите шаги выше, но:
   - **Name**: `fontawesome_48`
   - **Size**: `48`
   - Те же **Range**
   - **Замените** `src/fonts/lv_font_fontawesome_48.c`

## Шаг 3: Включение шрифтов

В файле `src/TenZillaPins.h` раскомментируйте:
```cpp
#define LV_FONT_FA14_ENABLED 1
#define LV_FONT_FA48_ENABLED 1
```

## Шаг 4: Пересборка проекта

Пересоберите проект в Arduino IDE.

## Проверка

После включения вы должны увидеть:
- ✅ Иконку WiFi вместо текста в правом верхнем углу
- ✅ Круглые иконки для двигателя (вверх/вниз/стоп) вместо стрелок
