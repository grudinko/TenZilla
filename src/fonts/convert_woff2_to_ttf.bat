@echo off
REM Конвертация FontAwesome WOFF2 в TTF
REM Требуется Python с установленным fonttools[woff]

echo Конвертация fa-solid-900.woff2 в TTF...
echo.

REM Проверка наличия Python
python --version >nul 2>&1
if errorlevel 1 (
    echo Ошибка: Python не найден!
    echo Установите Python с https://www.python.org/
    pause
    exit /b 1
)

REM Проверка наличия fonttools
python -c "import fontTools" >nul 2>&1
if errorlevel 1 (
    echo Установка fonttools[woff]...
    python -m pip install --user fonttools[woff]
    if errorlevel 1 (
        echo Ошибка при установке fonttools!
        pause
        exit /b 1
    )
)

REM Запуск конвертации
python convert_woff2_to_ttf.py

pause
