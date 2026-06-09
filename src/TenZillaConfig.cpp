#include "TenZillaConfig.h"

TenZillaSettings TenZillaConfig::settings;
Preferences TenZillaConfig::preferences;

void TenZillaConfig::begin() {
  load();
}

bool TenZillaConfig::load() {
  preferences.begin("tenzilla-config", true);  // read-only режим
  
  bool hasConfig = false;
  
  // Загружаем SSID
  if (preferences.isKey("ssid")) {
    String ssidStr = preferences.getString("ssid", "");
    ssidStr.toCharArray(settings.ssid, sizeof(settings.ssid));
    hasConfig = true;
  } else {
    settings.ssid[0] = '\0';
  }
  
  // Загружаем password
  if (preferences.isKey("password")) {
    String pwdStr = preferences.getString("password", "");
    pwdStr.toCharArray(settings.password, sizeof(settings.password));
    hasConfig = true;
  } else {
    settings.password[0] = '\0';
  }
  
  // Загружаем brightness
  settings.brightness = preferences.getUChar("brightness", 13);
  
  // Загружаем led_enabled
  settings.led_enabled = preferences.getBool("led_enabled", true);
  
  // Загружаем maxWebConnections
  settings.maxWebConnections = preferences.getUChar("max_web_conn", 1);
  if (settings.maxWebConnections < 1 || settings.maxWebConnections > 10) {
    settings.maxWebConnections = 1;
  }
  
  settings.relayActiveHigh = preferences.getBool("relay_active_high", false);

  if (preferences.isKey("tg_token")) {
    preferences.getString("tg_token", "").toCharArray(settings.tgBotToken, sizeof(settings.tgBotToken));
    hasConfig = true;
  } else {
    settings.tgBotToken[0] = '\0';
  }
  if (preferences.isKey("tg_chat")) {
    preferences.getString("tg_chat", "").toCharArray(settings.tgChatId, sizeof(settings.tgChatId));
    hasConfig = true;
  } else {
    settings.tgChatId[0] = '\0';
  }
  settings.tgEnabled = preferences.getBool("tg_enabled", false);
  settings.tgNotifyProgramResults = preferences.getBool("tg_n_results", true);
  settings.tgNotifyStartup = preferences.getBool("tg_n_startup", false);
  settings.tgNotifyOverload = preferences.getBool("tg_n_overload", true);
  settings.tgNotifyStopped = preferences.getBool("tg_n_stopped", true);
  Serial.print("TG load: notifyStartup=");
  Serial.println(settings.tgNotifyStartup ? 1 : 0);

  preferences.end();
  
  if (!hasConfig) {
    Serial.println("⚠️  No TenZilla config (WiFi nor Telegram), using defaults");
    reset();
    return false;
  }
  
  Serial.println("✅ TenZilla config loaded from NVS");
  return true;
}

bool TenZillaConfig::save() {
  preferences.begin("tenzilla-config", false);  // read-write режим
  
  // Сохраняем строки
  preferences.putString("ssid", String(settings.ssid));
  preferences.putString("password", String(settings.password));
  
  // Сохраняем числовые значения
  preferences.putUChar("brightness", settings.brightness);
  preferences.putBool("led_enabled", settings.led_enabled);
  preferences.putUChar("max_web_conn", settings.maxWebConnections);
  preferences.putBool("relay_active_high", settings.relayActiveHigh);
  preferences.putString("tg_token", String(settings.tgBotToken));
  preferences.putString("tg_chat", String(settings.tgChatId));
  preferences.putBool("tg_enabled", settings.tgEnabled);
  preferences.putBool("tg_n_results", settings.tgNotifyProgramResults);
  preferences.putBool("tg_n_startup", settings.tgNotifyStartup);
  preferences.putBool("tg_n_overload", settings.tgNotifyOverload);
  preferences.putBool("tg_n_stopped", settings.tgNotifyStopped);

  preferences.end();
  return true;
}

void TenZillaConfig::reset() {
  memset(&settings, 0, sizeof(settings));
  settings.brightness = 13;
  settings.led_enabled = true;
  settings.maxWebConnections = 1;
  settings.relayActiveHigh = false;
  settings.tgBotToken[0] = '\0';
  settings.tgChatId[0] = '\0';
  settings.tgEnabled = false;
  settings.tgNotifyProgramResults = true;
  settings.tgNotifyStartup = false;
  settings.tgNotifyOverload = true;
  settings.tgNotifyStopped = true;
  save();
}

TenZillaSettings TenZillaConfig::get() {
  return settings;
}

void TenZillaConfig::set(const TenZillaSettings& newSettings) {
  settings = newSettings;
}

const char* TenZillaConfig::getVersion() {
  return "TenZilla v1.0";
}