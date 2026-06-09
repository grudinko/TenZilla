#ifndef TENZILLA_WIFI_H
#define TENZILLA_WIFI_H

#include <WiFi.h>
#include "TenZillaConfig.h"

class TenZillaWiFi {
public:
  static void begin();
  static void connect();
  static void startAP();
  static void updateStatus();
  static String getAPName();
  static String getAPPassword();
  static String getStatus();
  static bool isConnected();
  
  // LED control
  static void setLEDColor(uint8_t r, uint8_t g, uint8_t b);
  static void blinkLED(uint8_t r, uint8_t g, uint8_t b, uint16_t duration = 500);

private:
  static String apName;
  static const char* AP_PASSWORD;
  static unsigned long lastLEDUpdate;
  static bool ledState;
  
  static void generateAPName();
  static String getMacAddress();
  static uint32_t getChipId();
};

#endif