#ifndef TENZILLA_CONFIG_H
#define TENZILLA_CONFIG_H

#include <Preferences.h>

struct TenZillaSettings {
  char ssid[32];
  char password[64];
  uint8_t brightness;
  bool led_enabled;
  uint8_t maxWebConnections;
  bool relayActiveHigh;
  // Telegram bot
  char tgBotToken[96];
  char tgChatId[32];
  bool tgEnabled;
  bool tgNotifyProgramResults;  // результаты программ (COMPLETED)
  bool tgNotifyStartup;         // включение системы
  bool tgNotifyOverload;        // ошибки перегрузки/лимитов
  bool tgNotifyStopped;         // принудительная остановка
};

class TenZillaConfig {
public:
  static void begin();
  static bool load();
  static bool save();
  static void reset();
  
  static TenZillaSettings get();
  static void set(const TenZillaSettings& settings);
  
  static const char* getVersion();

private:
  static TenZillaSettings settings;
  static Preferences preferences;
};

#endif