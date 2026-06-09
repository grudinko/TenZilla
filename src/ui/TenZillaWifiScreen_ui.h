/**
 * TenZilla WiFi Screen — слой дизайна (UI)
 * Динамические виджеты: labelStatus, labelSSID, labelIP, labelRSSI, labelClients, labelMaxConnections
 */

#ifndef TENZILLA_WIFI_SCREEN_UI_H
#define TENZILLA_WIFI_SCREEN_UI_H

#include "TenZillaLvglShim.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* labelStatus;
  lv_obj_t* labelSSID;
  lv_obj_t* labelIP;
  lv_obj_t* labelRSSI;
  lv_obj_t* labelClients;
  lv_obj_t* labelMaxConnections;
} TenZillaWifiScreenUI;

void TenZillaWifiScreen_ui_create(lv_obj_t** out_screen, TenZillaWifiScreenUI* out_ui);

#endif
