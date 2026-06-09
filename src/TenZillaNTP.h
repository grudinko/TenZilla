#ifndef TENZILLA_NTP_H
#define TENZILLA_NTP_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class TenZillaNTP {
public:
  // Инициализация NTP
  static void begin();
  
  // Обновление NTP (вызывать периодически)
  static void update();
  
  // Настройки NTP
  static void setNTPServer(const char* server);
  static const char* getNTPServer();
  static void setNTPInterval(unsigned long intervalSeconds);
  static unsigned long getNTPInterval();

  // Настройка часового пояса (POSIX-строка TZ, по умолчанию Москва)
  static void setTimezone(const char* tz);
  static const char* getTimezone();
  
  // Проверка синхронизации
  static bool isTimeSynced();
  static unsigned long getLastSyncTime();
  
  // Получение времени работы системы (uptime) в миллисекундах
  static unsigned long getUptimeMs();
  
  // Форматирование uptime в строку (дни:часы:минуты:секунды)
  static String getUptimeString();

private:
  static char ntpServer[64];
  static char timezoneStr[64];
  static unsigned long ntpInterval;
  static unsigned long lastNTPUpdate;
  static bool timeSynced;
  static unsigned long lastSyncTime;
  static unsigned long systemStartTime;
  
  static void syncTime();
};

#endif
