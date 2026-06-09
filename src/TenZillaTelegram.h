#ifndef TENZILLA_TELEGRAM_H
#define TENZILLA_TELEGRAM_H

#include <Arduino.h>

class TenZillaTelegram {
public:
  static void begin();

  /** Send a plain text message if bot is enabled and token/chat configured. */
  static void send(const char* message);
  static void send(const String& message) { send(message.c_str()); }

  /** Send test message; returns true if HTTP 200, false otherwise. Uses stored config. */
  static bool sendTest(const char* message);

  /** Like sendTest, but on false writes a specific error to errOut (e.g. "Укажите токен бота"). */
  static bool sendTestWithError(const char* message, String& errOut);

  /** Process queued messages; should be called periodically from the main loop. */
  static void processQueue();
};

#endif
