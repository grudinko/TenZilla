#include "TenZillaWiFi.h"

String TenZillaWiFi::apName = "";
const char* TenZillaWiFi::AP_PASSWORD = "tenzilla123";
unsigned long TenZillaWiFi::lastLEDUpdate = 0;
bool TenZillaWiFi::ledState = false;

void TenZillaWiFi::begin() {
  Serial.println("🔌 Initializing TenZilla WiFi...");
  generateAPName();
  connect();
}

void TenZillaWiFi::connect() {
  TenZillaSettings config = TenZillaConfig::get();
  
  if (strlen(config.ssid) > 0) {
    Serial.println("📡 Connecting to WiFi: " + String(config.ssid));
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.begin(config.ssid, config.password);
    
    int attempts = 0;
    while (attempts < 20 && WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      attempts++;
      blinkLED(0, 0, 255); // Синий во время подключения
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi Connected!");
      Serial.println("📶 IP: " + WiFi.localIP().toString());
      setLEDColor(0, 255, 0); // Зеленый при успехе
    } else {
      Serial.println("\n❌ WiFi Connection Failed");
      startAP();
    }
  } else {
    Serial.println("⚠️  No WiFi config, starting AP mode");
    startAP();
  }
}

void TenZillaWiFi::startAP() {
  WiFi.softAP(apName.c_str(), AP_PASSWORD);
  Serial.println("🛜 AP Mode Started: " + apName);
  Serial.println("🌐 AP IP: " + WiFi.softAPIP().toString());
  setLEDColor(255, 0, 255); // Пурпурный в AP mode
}

void TenZillaWiFi::updateStatus() {
  TenZillaSettings config = TenZillaConfig::get();
  if (!config.led_enabled) {
    neopixelWrite(48, 0, 0, 0);
    return;
  }
  
  unsigned long now = millis();
  if (now - lastLEDUpdate < 2000) return;
  lastLEDUpdate = now;
  
  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();
    if (rssi >= -50) setLEDColor(0, 255, 0);      // Отличный
    else if (rssi >= -70) setLEDColor(0, 0, 255); // Хороший
    else if (rssi >= -80) setLEDColor(255, 255, 0); // Средний
    else setLEDColor(255, 0, 0);                  // Плохой
  } else {
    // Мигание в AP mode
    ledState = !ledState;
    setLEDColor(ledState ? 255 : 0, 0, ledState ? 255 : 0);
  }
}

String TenZillaWiFi::getAPName() {
  return apName;
}

String TenZillaWiFi::getAPPassword() {
  return String(AP_PASSWORD);
}

String TenZillaWiFi::getStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    TenZillaSettings config = TenZillaConfig::get();
    return "Connected to " + String(config.ssid);
  } else {
    return "AP Mode: " + apName;
  }
}

bool TenZillaWiFi::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void TenZillaWiFi::setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  TenZillaSettings config = TenZillaConfig::get();
  float brightness = config.brightness / 255.0;
  neopixelWrite(48, r * brightness, g * brightness, b * brightness);
}

void TenZillaWiFi::blinkLED(uint8_t r, uint8_t g, uint8_t b, uint16_t duration) {
  setLEDColor(r, g, b);
  delay(duration / 2);
  setLEDColor(0, 0, 0);
  delay(duration / 2);
}

void TenZillaWiFi::generateAPName() {
  delay(500);
  WiFi.mode(WIFI_MODE_STA);
  delay(500);
  
  String mac = getMacAddress();
  Serial.print("Final MAC used: ");
  Serial.println(mac);
  
  if (mac == "00:00:00:00:00:00" || mac.length() != 17) {
    Serial.println("Using Chip ID as fallback");
    uint32_t chipId = getChipId();
    String chipHex = String(chipId, HEX);
    chipHex.toUpperCase();
    
    while (chipHex.length() < 4) {
      chipHex = "0" + chipHex;
    }
    if (chipHex.length() > 4) {
      chipHex = chipHex.substring(chipHex.length() - 4);
    }
    apName = "TenZilla_" + chipHex;
  } else {
    mac.replace(":", "");
    String lastFour = mac.substring(mac.length() - 4);
    apName = "TenZilla_" + lastFour;
  }
  
  Serial.print("Generated AP Name: ");
  Serial.println(apName);
}

String TenZillaWiFi::getMacAddress() {
  String mac = WiFi.macAddress();
  Serial.print("STA MAC: ");
  Serial.println(mac);
  
  if (mac != "00:00:00:00:00:00" && mac.length() == 17) {
    return mac;
  }
  
  mac = WiFi.softAPmacAddress();
  Serial.print("AP MAC: ");
  Serial.println(mac);
  
  if (mac != "00:00:00:00:00:00" && mac.length() == 17) {
    return mac;
  }
  
  uint64_t chipmac = ESP.getEfuseMac();
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           (uint8_t)(chipmac >> 40) & 0xFF,
           (uint8_t)(chipmac >> 32) & 0xFF,
           (uint8_t)(chipmac >> 24) & 0xFF,
           (uint8_t)(chipmac >> 16) & 0xFF,
           (uint8_t)(chipmac >> 8) & 0xFF,
           (uint8_t)chipmac & 0xFF);
  Serial.print("EFUSE MAC: ");
  Serial.println(macStr);
  
  return String(macStr);
}

uint32_t TenZillaWiFi::getChipId() {
  uint64_t mac = ESP.getEfuseMac();
  return (uint32_t)(mac >> 24);
}