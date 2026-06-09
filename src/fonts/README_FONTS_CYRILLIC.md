# Шрифты с кириллицей для LVGL

Чтобы экран «Сжатие» (и при необходимости другие) отображал русский текст, нужны шрифты Montserrat 14, 18, 24 с диапазоном **Cyrillic** (и ASCII для цифр и символов).

## Генерация через скрипт (Node.js)

В папке `src/fonts/`:

```bash
npm install
npm run gen
```

Скрипт скачивает Montserrat-Medium.ttf и генерирует `lv_font_montserrat_14_cyr.c`, `_18_cyr.c`, `_24_cyr.c`. Добавьте их в проект (Sketch → Add File или поместите в `src/`).

## Генерация через LVGL Font Converter (онлайн)

1. Откройте **https://lvgl.io/tools/fontconverter**.
2. **Font**: загрузите `Montserrat-Medium.ttf` (можно взять с [Google Fonts](https://fonts.google.com/specimen/Montserrat)).
3. **Name**: например `montserrat_14_cyr` (первый размер).
4. **Size**, **Bpp**: 14, 4 (для первого шрифта).
5. **Range**: добавьте:
   - `0x20-0x7F` (ASCII: пробел, цифры, буквы, знаки);
   - `0x0400-0x04FF` (кириллица).
6. Нажмите **Add** — шрифт появится в списке. Добавьте ещё два:
   - `montserrat_18_cyr`, size 18, bpp 4, те же range;
   - `montserrat_24_cyr`, size 24, bpp 4, те же range.
7. **Convert** → скачайте `.c` файл.
8. Сохраните файлы как **`lv_font_montserrat_14_cyr.c`**, **`_18_cyr.c`**, **`_24_cyr.c`** в **`src/fonts/`** (или `src/`). При одном общем файле — разнесите по трём или сгенерируйте три раза с разными размерами.
9. Убедитесь, что в них объявлены `lv_font_montserrat_14_cyr`, `lv_font_montserrat_18_cyr`, `lv_font_montserrat_24_cyr`. Иначе поправьте `lv_font_cyrillic.h` и `TenZillaMainScreen_ui.cpp`.

## Подключение к проекту

- **Arduino IDE**: **Sketch → Add File** и выберите `lv_font_montserrat_14_cyr.c`, `_18_cyr.c`, `_24_cyr.c` (если `src/fonts/` не подхватывается, положите их в `src/` и добавьте).
- **PlatformIO**: файлы в `src/` и `src/fonts/` обычно компилируются автоматически.

## Включение кириллицы

- **`USE_CYRILLIC 1`** — русские надписи и статусы.
- **`USE_CYRILLIC_FONTS 0`** — стандартные шрифты (сборка без .c). Кириллица на дисплее отображаться не будет (пустые квадраты).
- **`USE_CYRILLIC_FONTS 1`** — используйте шрифты с кириллицей. Добавьте `lv_font_montserrat_14_cyr.c`, `_18_cyr.c`, `_24_cyr.c` в проект, иначе сборка выдаст `undefined reference to lv_font_montserrat_*_cyr`.
