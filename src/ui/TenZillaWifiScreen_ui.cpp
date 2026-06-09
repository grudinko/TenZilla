/**
 * TenZilla WiFi Screen — дизайн.
 * Стиль как у SCALE SETTINGS / других экранов настроек.
 */

#include "TenZillaWifiScreen_ui.h"
#include "TenZillaLvglShim.h"

void TenZillaWifiScreen_ui_create(lv_obj_t** out_screen, TenZillaWifiScreenUI* out_ui) {
  if (out_screen == nullptr || out_ui == nullptr) return;

  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  #define _F14 &lv_font_montserrat_14
  #define _F22 &lv_font_montserrat_22
  #define _F30 &lv_font_montserrat_30
  #define _TITLE "WIFI"

  // Заголовок
  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, _TITLE);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(title, _F22, 0);
  lv_obj_set_style_text_letter_space(title, 1, 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

  // Status
  lv_obj_t* containerStatus = lv_obj_create(screen);
  lv_obj_set_size(containerStatus, 227, 50);
  lv_obj_align(containerStatus, LV_ALIGN_TOP_LEFT, 10, 50);
  lv_obj_set_style_bg_opa(containerStatus, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerStatus, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_border_width(containerStatus, 2, 0);
  lv_obj_set_style_radius(containerStatus, 8, 0);
  lv_obj_clear_flag(containerStatus, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelStatusTitle = lv_label_create(containerStatus);
  lv_label_set_text(labelStatusTitle, "ST:");
  lv_obj_set_style_text_color(labelStatusTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelStatusTitle, _F22, 0);
  lv_obj_set_style_text_align(labelStatusTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelStatusTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelStatusTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelStatus = lv_label_create(containerStatus);
  lv_label_set_text(labelStatus, "-");
  lv_obj_set_style_text_color(labelStatus, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(labelStatus, _F30, 0);
  lv_obj_set_style_text_align(labelStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelStatus, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelStatus, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // SSID
  lv_obj_t* containerSSID = lv_obj_create(screen);
  lv_obj_set_size(containerSSID, 228, 50);
  lv_obj_align(containerSSID, LV_ALIGN_TOP_LEFT, 242, 50);  // Справа от ST с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerSSID, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerSSID, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_border_width(containerSSID, 2, 0);
  lv_obj_set_style_radius(containerSSID, 8, 0);
  lv_obj_clear_flag(containerSSID, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelSSIDTitle = lv_label_create(containerSSID);
  lv_label_set_text(labelSSIDTitle, "SSID:");
  lv_obj_set_style_text_color(labelSSIDTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelSSIDTitle, _F22, 0);
  lv_obj_set_style_text_align(labelSSIDTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelSSIDTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelSSIDTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelSSID = lv_label_create(containerSSID);
  lv_label_set_text(labelSSID, "-");
  lv_obj_set_style_text_color(labelSSID, lv_color_hex(0x00FFFF), 0);
  lv_obj_set_style_text_font(labelSSID, _F22, 0);
  lv_obj_set_style_text_align(labelSSID, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelSSID, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelSSID, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // IP
  lv_obj_t* containerIP = lv_obj_create(screen);
  lv_obj_set_size(containerIP, 227, 50);
  lv_obj_align(containerIP, LV_ALIGN_TOP_LEFT, 10, 110);
  lv_obj_set_style_bg_opa(containerIP, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerIP, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_border_width(containerIP, 2, 0);
  lv_obj_set_style_radius(containerIP, 8, 0);
  lv_obj_clear_flag(containerIP, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelIPTitle = lv_label_create(containerIP);
  lv_label_set_text(labelIPTitle, "IP:");
  lv_obj_set_style_text_color(labelIPTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelIPTitle, _F22, 0);
  lv_obj_set_style_text_align(labelIPTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelIPTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelIPTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelIP = lv_label_create(containerIP);
  lv_label_set_text(labelIP, "-");
  lv_obj_set_style_text_color(labelIP, lv_color_hex(0xFFFF00), 0);
  lv_obj_set_style_text_font(labelIP, _F22, 0);
  lv_obj_set_style_text_align(labelIP, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelIP, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelIP, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // RSSI
  lv_obj_t* containerRSSI = lv_obj_create(screen);
  lv_obj_set_size(containerRSSI, 228, 50);
  lv_obj_align(containerRSSI, LV_ALIGN_TOP_LEFT, 242, 110);  // Справа от IP с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerRSSI, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerRSSI, lv_color_hex(0xFF00FF), 0);
  lv_obj_set_style_border_width(containerRSSI, 2, 0);
  lv_obj_set_style_radius(containerRSSI, 8, 0);
  lv_obj_clear_flag(containerRSSI, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelRSSITitle = lv_label_create(containerRSSI);
  lv_label_set_text(labelRSSITitle, "RSSI:");
  lv_obj_set_style_text_color(labelRSSITitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelRSSITitle, _F22, 0);
  lv_obj_set_style_text_align(labelRSSITitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelRSSITitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelRSSITitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelRSSI = lv_label_create(containerRSSI);
  lv_label_set_text(labelRSSI, "-");
  lv_obj_set_style_text_color(labelRSSI, lv_color_hex(0xFF00FF), 0);
  lv_obj_set_style_text_font(labelRSSI, _F30, 0);
  lv_obj_set_style_text_align(labelRSSI, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelRSSI, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelRSSI, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Clients
  lv_obj_t* containerClients = lv_obj_create(screen);
  lv_obj_set_size(containerClients, 227, 50);
  lv_obj_align(containerClients, LV_ALIGN_TOP_LEFT, 10, 170);
  lv_obj_set_style_bg_opa(containerClients, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerClients, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_border_width(containerClients, 2, 0);
  lv_obj_set_style_radius(containerClients, 8, 0);
  lv_obj_clear_flag(containerClients, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelClientsTitle = lv_label_create(containerClients);
  lv_label_set_text(labelClientsTitle, "CLIENTS:");
  lv_obj_set_style_text_color(labelClientsTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelClientsTitle, _F22, 0);
  lv_obj_set_style_text_align(labelClientsTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelClientsTitle, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelClientsTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelClients = lv_label_create(containerClients);
  lv_label_set_text(labelClients, "0");
  lv_obj_set_style_text_color(labelClients, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_text_font(labelClients, _F30, 0);
  lv_obj_set_style_text_align(labelClients, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelClients, 227);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelClients, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Максимум подключений (лимит)
  lv_obj_t* containerMaxConn = lv_obj_create(screen);
  lv_obj_set_size(containerMaxConn, 228, 50);
  lv_obj_align(containerMaxConn, LV_ALIGN_TOP_LEFT, 242, 170);  // Справа от CLIENTS с разрывом 5px (10 + 227 + 5 = 242)
  lv_obj_set_style_bg_opa(containerMaxConn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(containerMaxConn, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_border_width(containerMaxConn, 2, 0);
  lv_obj_set_style_radius(containerMaxConn, 8, 0);
  lv_obj_clear_flag(containerMaxConn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* labelMaxConnTitle = lv_label_create(containerMaxConn);
  lv_label_set_text(labelMaxConnTitle, "MAX CONN:");
  lv_obj_set_style_text_color(labelMaxConnTitle, lv_color_white(), 0);
  lv_obj_set_style_text_font(labelMaxConnTitle, _F22, 0);
  lv_obj_set_style_text_align(labelMaxConnTitle, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_width(labelMaxConnTitle, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMaxConnTitle, LV_ALIGN_LEFT_MID, -10, 0);  // Прижато к левому краю рамки

  lv_obj_t* labelMaxConnections = lv_label_create(containerMaxConn);
  lv_label_set_text(labelMaxConnections, "1");
  lv_obj_set_style_text_color(labelMaxConnections, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_text_font(labelMaxConnections, _F30, 0);
  lv_obj_set_style_text_align(labelMaxConnections, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_width(labelMaxConnections, 228);  // Ширина для выравнивания текста внутри области
  lv_obj_align(labelMaxConnections, LV_ALIGN_RIGHT_MID, 10, 0);  // Прижато к правому краю рамки

  // Подсказка
  lv_obj_t* hint = lv_label_create(screen);
  lv_label_set_text(hint, "Hold 2s for menu");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_font(hint, _F14, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 10, -10);

#undef _F14
#undef _F22
#undef _F30
#undef _TITLE

  out_ui->screen = screen;
  out_ui->labelStatus = labelStatus;
  out_ui->labelSSID = labelSSID;
  out_ui->labelIP = labelIP;
  out_ui->labelRSSI = labelRSSI;
  out_ui->labelClients = labelClients;
  out_ui->labelMaxConnections = labelMaxConnections;
  *out_screen = screen;
}
