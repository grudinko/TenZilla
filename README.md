# TenZilla

ESP32-S3 система управления: весы (NAU7802), энкодер (Hall / JGA25-371), двигатель, LVGL-дисплей, WiFi, веб-интерфейс.

## Сборка

- **Arduino IDE** или **PlatformIO**: открыть папку как скетч (`Tenzilla.ino`).
- Плата: **ESP32-S3**.

### Настройки платы в Arduino IDE (обязательно)

Иначе ошибка `Sketch too big` / лимит **1310720** байт (~1,25 MB):

| Tools | Значение |
|-------|----------|
| **Board** | Ваша ESP32-S3 |
| **Flash Size** | **4MB** (или 8MB, если на плате 8 MB — тогда нужна своя `partitions.csv`) |
| **Partition Scheme** | **Custom Partition Table** |
| **Custom Partition Table** | `partitions.csv` (файл в корне скетча) |
| **USB CDC On Boot** | **Enabled** |
| **PSRAM** | по плате (часто Disabled) |

После смены схемы разделов: **Sketch → Clean**, затем полная пересборка.  
Первая прошивка с новой таблицей — **только через USB** (не OTA).

С кастомной `partitions.csv` слот приложения **~1,81 MB** (0x1D0000) — этого хватает для текущей прошивки (~1,74 MB).
- Библиотеки: LVGL, LovyanGFX, Adafruit NAU7802, **FastBot** (GyverLibs) — Telegram-бот, ArduinoJson, стандартные (WiFi, WebServer, Preferences, …).  
  Установка FastBot: Менеджер библиотек → «FastBot» → Установить.
- `lv_conf.h` — файл конфигурации LVGL находится в корне проекта для удобства версионирования.
  
  **Настройка для Arduino IDE:**
  - Если файл не подхватывается автоматически, скопируйте `lv_conf.h` в папку `libraries/` рядом с папкой `lvgl/`
  - Или добавьте флаг компилятора в настройках платы: `-DLV_CONF_INCLUDE_SIMPLE` или `-DLV_CONF_PATH="lv_conf.h"`

## Структура

- `Tenzilla.ino` — точка входа.
- `src/` — основной код (Scale, Encoder, Display, Program, Web, WiFi, экраны).
- `src/ui/` — дизайн экранов LVGL (можно править в SquareLine Studio и т.п.).
- `src/TenZillaPins.h` — пины и опции энкодера.

## Документация

- [ENCODER_HALL.md](ENCODER_HALL.md) — настройка энкодера с датчиками Холла (режимы, шаг мм).
- [MOTOR_JGA25-371.md](MOTOR_JGA25-371.md) — двигатель JGA25-371, подключение, 12 имп./об.

## Лицензия

На усмотрение автора.
