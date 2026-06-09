# UI-слой (дизайн экранов)

Экраны вынесены в отдельные модули `*_ui.h` / `*_ui.cpp`. **Только** создание виджетов, стили и layout. Вся логика (обновление данных, навигация) остаётся в `TenZilla*Screen.cpp` и `TenZillaDisplay.cpp`.

## Структура

| Файл | Экран | Динамические виджеты (логика обновляет) |
|------|--------|----------------------------------------|
| `TenZillaMainScreen_ui.*` | COMPRESSION | labelCurrentWeight, labelMaxWeight, labelDisplacement, labelStatus, labelMotorIcon, labelLimVal |
| `TenZillaBreakScreen_ui.*` | BREAK | labelCurrentWeight, labelMaxWeight, labelDisplacement, labelStatus, labelMotorIcon |
| `TenZillaWifiScreen_ui.*` | WiFi | labelStatus, labelSSID, labelIP, labelRSSI, labelClients |
| `TenZillaCalibrationScreen_ui.*` | CALIBRATION | labelCurrentWeight, labelCalibrationFactor, labelStep |
| `Splash_ui.*` | Splash | — |
| `Confirmation_ui.*` | MENU | btnStart, btnResetMov, btnResetZero, btnExit (START скрыт на 3–6) |

## Редактирование в визуальных редакторах

1. **SquareLine Studio** (https://squareline.io) или **LVGL Editor** (https://lvgl.io/editor): сделайте экран, экспортируйте C-код.
2. Замените содержимое соответствующего `*_ui.cpp` сгенерированным кодом (или вставьте создание виджетов в `*_ui_create`).
3. **Важно:** сохраните заполнение `out_ui` в конце: в структуру должны попасть указатели на все **динамические** виджеты из таблицы выше. Без них логика не сможет обновлять данные.
4. Имена виджетов в редакторе лучше задавать так же, как в структуре (например, `labelCurrentWeight`), чтобы было проще сопоставить.

## Сборка

Убедитесь, что `src/ui/*.cpp` участвуют в сборке. В Arduino IDE часто компилируются только файлы из папки скетча; если `ui/` не подхватывается, добавьте файлы через **Sketch → Add File** или настройте включение `src/ui` в компиляцию.

## Зависимости

В `*_ui.cpp` допустимы только `#include <lvgl.h>` и соответствующий `*_ui.h`. Никаких `TenZilla*`, `Arduino.h`, `TenZillaScale` и т.п.
