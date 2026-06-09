# OTA-бинарники по версиям

Сюда складываются собранные `.bin` для обновления устройств через веб (OTA).

## Как получить бинарник для версии

1. В Arduino IDE откройте **Tenzilla.ino** и выполните **Sketch → Verify** (или **Compile**).  
   Бинарник приложения создаётся в папке сборки:  
   **`build/esp32.esp32.esp32s3/Tenzilla.ino.bin`**  
   (в корне скетча `Tenzilla.bin` в Arduino IDE 2 обычно не появляется.)

2. Из **корня проекта** (папка Tenzilla) запустите **`build_ota_binary.cmd`** (двойной клик или из командной строки).  
   Если предпочитаете PowerShell: `.\build_ota_binary.ps1` (при ошибке «выполнение сценариев отключено» используйте .cmd).  
   Скрипт возьмёт `Tenzilla.ino.bin` из `build/...`, версию из `src/TenZillaVersion.h` и создаст  
   **`releases/Tenzilla_<RELEASE_NUMBER>.bin`** (например, `Tenzilla_R2.11.0.149.bin`).

3. Для OTA загружайте файл из `releases/` (или сразу `build/.../Tenzilla.ino.bin`) на страницу **http://&lt;IP&gt;/update**.

## Имена файлов

- Формат: `Tenzilla_R<MAJOR>.<MINOR>.<PATCH>.<BUILD>.bin`
- Пример: `Tenzilla_R2.10.3.145.bin`

Бинарники `*.bin` уже перечислены в корневом `.gitignore`, в репозиторий они не попадут.
