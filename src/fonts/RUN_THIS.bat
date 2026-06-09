@echo off
REM ========================================
REM ГЕНЕРАЦИЯ FONTAWESOME ШРИФТОВ
REM ========================================
REM Запустите этот файл ДВАЖДЫ кликом
REM или выполните в командной строке

cd /d "%~dp0"
echo.
echo ========================================
echo Генерация FontAwesome шрифтов для LVGL
echo ========================================
echo.

REM Проверка TTF файла
if not exist "fa-solid-900.ttf" (
    echo [ОШИБКА] Файл fa-solid-900.ttf не найден!
    echo Поместите TTF файл в эту папку.
    pause
    exit /b 1
)

echo [OK] Найден файл: fa-solid-900.ttf
echo.

REM Установка зависимостей
echo [1/3] Установка зависимостей...
echo Попытка установить lv_font_conv...
call npm install lv_font_conv@1.5.3 --save-dev
if errorlevel 1 (
    echo.
    echo Попытка установить без версии...
    call npm install lv_font_conv --save-dev
    if errorlevel 1 (
        echo.
        echo [ОШИБКА] Не удалось установить зависимости!
        echo.
        echo Попробуйте выполнить вручную в командной строке:
        echo   cd src\fonts
        echo   npm install lv_font_conv --save-dev
        echo.
        echo ИЛИ используйте онлайн-конвертер:
        echo   https://lvgl.io/tools/fontconverter
        echo.
        pause
        exit /b 1
    )
)

echo.
REM node_modules будет создан заново при npm install

echo [2/3] Генерация шрифта 14px...
if exist "node_modules\.bin\lv_font_conv.cmd" (
    call node_modules\.bin\lv_font_conv.cmd --font "fa-solid-900.ttf" -r 0xF0AA -r 0xF0AB -r 0xF1EB -r 0xF28D --size 14 --bpp 4 --format lvgl --no-compress -o "lv_font_fontawesome_14.c"
    if errorlevel 1 (
        echo [ОШИБКА] При генерации 14px шрифта
        pause
        exit /b 1
    )
    echo [OK] Сгенерирован: lv_font_fontawesome_14.c
) else (
    echo [ОШИБКА] lv_font_conv не найден после установки
    pause
    exit /b 1
)

echo.
echo [3/3] Генерация шрифта 48px...
call node_modules\.bin\lv_font_conv.cmd --font "fa-solid-900.ttf" -r 0xF0AA -r 0xF0AB -r 0xF1EB -r 0xF28D --size 48 --bpp 4 --format lvgl --no-compress -o "lv_font_fontawesome_48.c"
if errorlevel 1 (
    echo [ОШИБКА] При генерации 48px шрифта
    pause
    exit /b 1
)
echo [OK] Сгенерирован: lv_font_fontawesome_48.c

echo.
echo ========================================
echo ГЕНЕРАЦИЯ ЗАВЕРШЕНА УСПЕШНО!
echo ========================================
echo.
echo Сгенерированные файлы:
echo   - lv_font_fontawesome_14.c
echo   - lv_font_fontawesome_48.c
echo.
echo СЛЕДУЮЩИЕ ШАГИ:
echo.
echo 1. Откройте файл: src\TenZillaPins.h
echo.
echo 2. Найдите строки (около 118-119):
echo    // #define LV_FONT_FA14_ENABLED 1
echo    // #define LV_FONT_FA48_ENABLED 1
echo.
echo 3. Раскомментируйте (уберите //):
echo    #define LV_FONT_FA14_ENABLED 1
echo    #define LV_FONT_FA48_ENABLED 1
echo.
echo 4. Пересоберите проект в Arduino IDE
echo.
echo 5. Готово! Иконки FontAwesome появятся на экране.
echo.
echo ПРИМЕЧАНИЕ: После генерации node_modules будет удален
echo чтобы Arduino IDE не пыталась компилировать эти файлы.
echo При необходимости можно переустановить через: npm install
echo.
pause

REM Удаляем node_modules после генерации, чтобы не мешать компиляции Arduino IDE
if exist "node_modules" (
    echo.
    echo [INFO] Удаление node_modules для исключения из компиляции Arduino IDE...
    rmdir /s /q "node_modules"
    echo [OK] node_modules удален
)
