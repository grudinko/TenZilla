#include "TenZillaWifiScreen.h"
#include "TenZillaDisplay.h"
#include "TenZillaWeb.h"
#include "ui/TenZillaWifiScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaWifiScreenUI s_ui;

void TenZillaWifiScreen::drawStatic(void* tft) {
  (void)tft;
}

void TenZillaWifiScreen::updateData(void* tft,
                                    bool forceUpdate,
                                    bool wifiConnected,
                                    String wifiSSID,
                                    String wifiIP,
                                    int wifiRSSI,
                                    int wifiClients,
                                    String apName,
                                    String apPassword) {
  (void)tft; (void)forceUpdate; (void)wifiConnected; (void)wifiSSID; (void)wifiIP;
  (void)wifiRSSI; (void)wifiClients; (void)apName; (void)apPassword;
}

// ============================================
// LVGL (логика; дизайн в ui/)
// ============================================

void TenZillaWifiScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaWifiScreen_ui_create(&screen, &s_ui);
}

void TenZillaWifiScreen::updateLVGL(lv_obj_t* screen, bool wifiConnected, String wifiSSID, String wifiIP, int wifiRSSI, int wifiClients) {
  if (screen == nullptr || s_ui.labelStatus == nullptr) return;

  static bool lastWifiConnected = false;
  static String lastWifiSSID = "";
  static String lastWifiIP = "";
  static int lastWifiRSSI = -9999;
  static int lastWifiClients = -9999;
  static int lastMaxConn = -9999;

  // Статус подключения - обновляем только при изменении
  if (wifiConnected != lastWifiConnected) {
    if (wifiConnected) {
      lv_label_set_text(s_ui.labelStatus, "Connected");
      lv_obj_set_style_text_color(s_ui.labelStatus, lv_color_hex(0x00FF00), 0);
    } else {
      lv_label_set_text(s_ui.labelStatus, "AP Mode");
      lv_obj_set_style_text_color(s_ui.labelStatus, lv_color_hex(0xFFFF00), 0);
    }
    lastWifiConnected = wifiConnected;
  }

  // SSID - обновляем только при изменении
  if (wifiSSID != lastWifiSSID) {
    if (wifiSSID.length() > 0) {
      lv_label_set_text(s_ui.labelSSID, wifiSSID.c_str());
    } else {
      lv_label_set_text(s_ui.labelSSID, "TenZilla_AP");
    }
    lastWifiSSID = wifiSSID;
  }

  // IP - обновляем только при изменении
  if (wifiIP != lastWifiIP) {
    if (wifiIP.length() > 0) {
      lv_label_set_text(s_ui.labelIP, wifiIP.c_str());
    } else {
      lv_label_set_text(s_ui.labelIP, "-");
    }
    lastWifiIP = wifiIP;
  }

  // RSSI - обновляем только при изменении
  if (wifiRSSI != lastWifiRSSI) {
    char buffer[16];
    if (wifiRSSI > -100) {
      snprintf(buffer, sizeof(buffer), "%d dBm", wifiRSSI);
      lv_label_set_text(s_ui.labelRSSI, buffer);
    } else {
      lv_label_set_text(s_ui.labelRSSI, "-");
    }
    lastWifiRSSI = wifiRSSI;
  }

  // Количество клиентов - обновляем только при изменении
  if (wifiClients != lastWifiClients) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", wifiClients);
    lv_label_set_text(s_ui.labelClients, buffer);
    lastWifiClients = wifiClients;
  }

  // Максимальное количество подключений - обновляем только при изменении
  int maxConn = TenZillaWeb::getMaxConnections();
  if (maxConn != lastMaxConn) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", maxConn);
    if (s_ui.labelMaxConnections != nullptr) {
      lv_label_set_text(s_ui.labelMaxConnections, buffer);
    }
    lastMaxConn = maxConn;
  }
}
