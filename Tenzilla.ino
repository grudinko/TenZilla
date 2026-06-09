/**
 * TenZilla - ESP32-S3 Control System
 * 
 * ⚠️ ВАЖНО ДЛЯ РАБОТЫ Serial и загрузки прошивки:
 * 
 * Если Serial не работает или прошивка заливается только через BOOT:
 * 
 * 1. ПРОВЕРЬТЕ НАСТРОЙКИ ПЛАТЫ В ARDUINO IDE:
 *    - Tools > Board: выберите правильную плату ESP32-S3
 *    - Tools > USB CDC On Boot: ENABLED (ОБЯЗАТЕЛЬНО!)
 *    - Tools > USB Mode: "Hardware CDC and JTAG"
 *    - Tools > Upload Mode: "UART0 / USB CDC" (или "USB CDC")
 *    - Tools > Upload Speed: 115200 или 460800 (НЕ используйте 921600!)
 *    - Tools > USB DFU On Boot: DISABLED (если есть)
 *    - Скорость монитора порта: 115200 бод
 * 
 * ⚠️ ВАЖНО: Если возникают ошибки загрузки, снизьте Upload Speed до 115200!
 * 
 * 2. ЕСЛИ ПРОБЛЕМА СОХРАНЯЕТСЯ:
 *    - Отключите плату от USB
 *    - Зажмите кнопку BOOT на плате
 *    - Подключите USB
 *    - Отпустите кнопку BOOT
 *    - Загрузите прошивку
 *    - После загрузки нажмите кнопку RESET
 * 
 * 3. ПРОВЕРКА РАБОТЫ Serial:
 *    - Откройте Serial Monitor (115200 бод)
 *    - Нажмите кнопку RESET на плате
 *    - Должно появиться сообщение "SERIAL TEST - If you see this, Serial works!"
 * 
 * 4. ЕСЛИ Serial ВСЕ ЕЩЕ НЕ РАБОТАЕТ:
 *    - Попробуйте другой USB кабель (должен поддерживать данные)
 *    - Попробуйте другой USB порт
 *    - Проверьте драйверы USB для ESP32-S3
 *    - Перезагрузите Arduino IDE
 * 
 * ВАЖНО: Программа работает независимо от подключения USB/Arduino IDE.
 * Все вызовы Serial.flush() удалены, чтобы избежать блокировки при
 * отключенном USB. Программа будет работать даже без Serial Monitor.
 *
 * OTA (обновление через Web): Tools > Partition Scheme — «Custom partition table»,
 * укажите partitions.csv из корня проекта. Иначе OTA через веб не работает.
 * Слоты ota_0/ota_1 по 1.75 MB; SPIFFS 384 KB. После смены схемы разделов
 * первую прошивку обязательно делать через USB. При "Image doesn't fit" увеличьте
 * размер слотов в partitions.csv.
 *
 * Память для SSL (Telegram, HTTPS): в корне sdkconfig.defaults. Буферы TLS уменьшены
 * до 4KB (вместо 16KB) для борьбы с "SSL - Memory allocation failed". После изменения
 * sdkconfig.defaults: Sketch > Clean + полная пересборка. Если ошибка остаётся — в
 * настройках платы включите PSRAM (Tools > PSRAM), затем снова Clean + сборка.
 */

// Function declarations from main_functions.cpp
extern void tenzilla_setup();
extern void tenzilla_loop();

// Arduino setup and loop functions
void setup() {
  // Инициализация Serial для ESP32-S3
  Serial.begin(115200);
  
  // КРИТИЧЕСКИ ВАЖНО: Ждем инициализации USB CDC
  delay(3000);
  
  // Простой тест Serial
  Serial.println();
  Serial.println("========================================");
  Serial.println("SERIAL TEST - If you see this, Serial works!");
  Serial.println("========================================");
  Serial.print("Milliseconds since start: ");
  Serial.println(millis());
  Serial.println("Starting TenZilla initialization...");
  delay(100);
  
  tenzilla_setup();
  
  Serial.println("tenzilla_setup() completed!");
}

void loop() {
  tenzilla_loop();
}