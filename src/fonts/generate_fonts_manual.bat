@echo off
REM Ручная генерация FontAwesome шрифтов через lv_font_conv
REM Требуется: npm install (выполните вручную, если скрипт не работает)

echo ========================================
echo Генерация FontAwesome шрифтов для LVGL
echo ========================================
echo.

cd /d "%~dp0"

REM Проверка наличия TTF файла
if not exist "fa-solid-900.ttf" (
    echo ОШИБКА: Файл fa-solid-900.ttf не найден!
    echo Поместите TTF файл в эту папку.
    pause
    exit /b 1
)

echo Найден файл: fa-solid-900.ttf
echo.

REM Проверка наличия node_modules
if not exist "node_modules\.bin\lv_font_conv.cmd" (
    echo Установка зависимостей...
    call npm install
    if errorlevel 1 (
        echo ОШИБКА: Не удалось установить зависимости!
        echo Выполните вручную: npm install
        pause
        exit /b 1
    )
)

echo.
echo Генерация шрифта 14px...
call node_modules\.bin\lv_font_conv.cmd --font "fa-solid-900.ttf" -r 0xF0AA -r 0xF0AB -r 0xF1EB -r 0xF28D --size 14 --bpp 4 --format lvgl --no-compress -o "lv_font_fontawesome_14.c"

if errorlevel 1 (
    echo ОШИБКА при генерации 14px шрифта!
    pause
    exit /b 1
)

echo Генерация шрифта 48px...
call node_modules\.bin\lv_font_conv.cmd --font "fa-solid-900.ttf" -r 0xF0AA -r 0xF0AB -r 0xF1EB -r 0xF28D --size 48 --bpp 4 --format lvgl --no-compress -o "lv_font_fontawesome_48.c"

if errorlevel 1 (
    echo ОШИБКА при генерации 48px шрифта!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Генерация завершена успешно!
echo ========================================
echo.
echo Сгенерированные файлы:
echo - lv_font_fontawesome_14.c
echo - lv_font_fontawesome_48.c
echo.
echo Теперь в src/TenZillaPins.h раскомментируйте:
echo   #define LV_FONT_FA14_ENABLED 1
echo   #define LV_FONT_FA48_ENABLED 1
echo.
pause
