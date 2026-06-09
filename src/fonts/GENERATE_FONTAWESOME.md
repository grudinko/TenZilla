# Генерация FontAwesome шрифтов

## ⚠️ ВАЖНО: Текущие файлы - это заглушки!

Файлы `lv_font_fontawesome_14.c` и `lv_font_fontawesome_48.c` содержат только минимальные заглушки для компиляции проекта. **Они не содержат реальных иконок FontAwesome!**

## Способ 1: Онлайн конвертер (РЕКОМЕНДУЕТСЯ)

1. Откройте **https://lvgl.io/tools/fontconverter**

2. **Для шрифта 14px:**
   - **Font**: Загрузите `fa-solid-900.ttf` из [FontAwesome](https://fontawesome.com/download) (Free версия)
   - **Name**: `fontawesome_14`
   - **Size**: `14`
   - **Bpp**: `4`
   - **Range**: Добавьте по одному:
     - `0xF0AA` (Circle-up)
     - `0xF0AB` (Circle-down)
     - `0xF1EB` (WiFi)
     - `0xF28D` (Circle-stop)
   - Нажмите **Add**
   - Нажмите **Convert**
   - Скачайте файл и сохраните как `lv_font_fontawesome_14.c` в папке `src/fonts/`

3. **Для шрифта 48px:**
   - Повторите шаги выше, но:
   - **Name**: `fontawesome_48`
   - **Size**: `48`
   - Те же **Range**
   - Сохраните как `lv_font_fontawesome_48.c`

## Способ 2: Через Node.js (если есть интернет)

```bash
cd src/fonts
npm install
npm run gen:fa
```

Если возникнут проблемы с загрузкой, используйте Способ 1.

## После генерации

1. Убедитесь, что файлы `lv_font_fontawesome_14.c` и `lv_font_fontawesome_48.c` находятся в `src/fonts/`
2. В `src/TenZillaPins.h` раскомментируйте:
   ```cpp
   #define LV_FONT_FA14_ENABLED 1
   #define LV_FONT_FA48_ENABLED 1
   ```
3. Пересоберите проект

## Проверка

После включения шрифтов вы должны увидеть:
- Иконку WiFi вместо текста "WiFi:" в правом верхнем углу
- Круглые иконки для направления двигателя (вверх/вниз/стоп) вместо стрелок
