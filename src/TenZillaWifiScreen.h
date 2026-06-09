#ifndef TENZILLA_WIFI_SCREEN_H
#define TENZILLA_WIFI_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaWifiScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen, bool wifiConnected, String wifiSSID, String wifiIP, int wifiRSSI, int wifiClients);
  
  // Старые методы TFT_eSPI (для обратной совместимости)
  static void drawStatic(void* tft);
  static void updateData(void* tft,
                         bool forceUpdate,
                         bool wifiConnected,
                         String wifiSSID,
                         String wifiIP,
                         int wifiRSSI,
                         int wifiClients,
                         String apName,
                         String apPassword);
  
  // Дизайн: ui/TenZillaWifiScreen_ui.*
};

#endif
