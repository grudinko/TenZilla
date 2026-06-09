#ifndef TENZILLA_DISPLAY_H
#define TENZILLA_DISPLAY_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"
#include <LovyanGFX.hpp>
#include "TenZillaPins.h"
#include "TenzillaLGFX.h"  // Класс LGFX для дисплея
#include "TenZillaMainScreen.h"
#include "TenZillaBreakScreen.h"
#include "TenZillaWifiScreen.h"
#include "TenZillaScaleSettingsScreen.h"
#include "TenZillaMotorSettingsScreen.h"
#include "TenZillaOtherSettingsScreen.h"
#include "TenZillaHistoryScreen.h"

// Размеры экрана (исправлено: поменяны местами)
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 480

class TenZillaDisplay {
public:
  // Инициализация
  static void begin();
  static void update();
  static void checkButton();
  
  // Управление экранами
  static void showSplashScreen();
  static void showMainScreen();  // COMPRESSION
  static void showBreakScreen(); // BREAK
  static void showWiFiScreen();
  static void showScaleSettingsScreen();  // Настройки тензодатчика
  static void showMotorSettingsScreen();  // Настройки двигателя
  static void showOtherSettingsScreen();  // Прочие настройки
  static void showHistoryScreen();       // История измерений
  static void showConfirmationScreen(); // Экран подтверждения YES/NO
  static void nextScreen();
  
  // Обновление данных
  static void updateWeight(float weight);
  static void updateMaxWeight(float maxWeight);
  static void updateOpticalCount(int count);
  static void updateWiFiStatus(bool connected, String ssid = "");
  static void updateWiFiIP(String ip);
  static void updateWiFiClients(int clients);
  static void updateMotorStatus(bool running, int direction = 0);
  static void updateRSSI(int rssi);
  
  // Принудительное обновление экрана (для немедленного отображения изменений)
  static void forceUpdateScreen();
  
  // Частичное обновление: обновить только конкретный объект LVGL
  // Используйте для быстрого обновления отдельных элементов (например, только label с весом)
  // LVGL автоматически определит минимальную область для обновления
  // Пример: TenZillaDisplay::invalidateObject(labelWeight); // Обновит только label с весом
  static void invalidateObject(lv_obj_t* obj);
  
  // Частичное обновление: обновить конкретную область экрана (координаты в пикселях)
  // Полезно для обновления графиков или больших областей без перерисовки всего экрана
  // Пример: TenZillaDisplay::invalidateArea(nullptr, 0, 0, 100, 50); // Обновит область 0,0-100,50
  static void invalidateArea(lv_obj_t* obj, int x1, int y1, int x2, int y2);
  
  // Управление максимумами экранов
  static void resetMainScreenMax();   // Сброс максимума СЖАТИЕ
  static void resetBreakScreenMax();  // Сброс максимума РАЗРЫВ
  static float getMainScreenMax();    // Получить максимум СЖАТИЕ
  static float getBreakScreenMax();   // Получить максимум РАЗРЫВ
  
  // Текущий экран (для синхронизации с веб-клиентами)
  static int getCurrentScreen();
  
  // Получить указатель на LVGL display
  static lv_disp_t* getDisplay();
  
  // Получить метрики производительности
  static float getCPULoad();
  static float getCPULoadCore1();  // Загрузка второго ядра (Core 1)
  static float getFPS();
  static int getWiFiRSSI();
  
private:
  static bool displayInitialized;
  static unsigned long lastUpdate;
  static unsigned long lastButtonCheck;
  static bool lastButtonState;
  static int currentScreen;
  static int confirmationSelection;           // Выбор на экране подтверждения: 0=YES, 1=NO
  static int previousScreenBeforeConfirmation; // Экран, с которого открыли меню YES/NO
  static unsigned long lastNavigationTime;    // Время последней навигации (защита от дребезга)
  static bool buttonPressed;                  // Флаг нажатия кнопки
  static unsigned long buttonPressStartTime; // Время начала нажатия кнопки
  static bool buttonProcessed;                // Флаг обработки нажатия (для предотвращения повторной обработки)
  
  // Мониторинг производительности
  static float currentFPS;
  static float cpuLoadPercent;
  static unsigned long lastFPSUpdate;
  static unsigned long frameCount;
  static unsigned long lastLoopTime;
  static unsigned long totalDrawTime;
  static unsigned long maxDrawTime;
  
  // Объекты дисплея
  static LGFX* tft;                    // LovyanGFX объект
  static lv_disp_t* lvglDisplay;       // LVGL display объект (lv_disp_t для v8.x)
  
  // Данные для отображения
  static float currentWeight;
  static float maxWeight;
  static int opticalCount;
  static bool wifiConnected;
  static String wifiSSID;
  static String wifiIP;
  static int wifiRSSI;
  static int wifiClients;
  static bool motorRunning;
  static int motorDirection;
  
  // Кэш для частичного обновления
  static float lastDisplayedWeight;
  static int lastDisplayedCount;
  static bool lastDisplayedWiFi;
  static String lastDisplayedSSID;
  static int lastDisplayedRSSI;
  static bool lastDisplayedMotor;
  
  // Overlay (layer_top): дата, CPU, FPS, WiFi — общие для всех экранов
  static lv_obj_t* overlayContainer;

  // LVGL экраны (объекты)
  static lv_obj_t* splashScreen;
  static lv_obj_t* mainScreen;
  static lv_obj_t* breakScreen;
  static lv_obj_t* wifiScreen;
  static lv_obj_t* scaleSettingsScreen;  // Настройки тензодатчика
  static lv_obj_t* motorSettingsScreen;  // Настройки двигателя
  static lv_obj_t* otherSettingsScreen;  // Прочие настройки
  static lv_obj_t* historyScreen;       // История измерений
  static lv_obj_t* confirmationScreen;
  
  // Вспомогательные методы
  static void initLovyanGFX();
  static void initLVGL();
  static void lvglFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
  static void updateConfirmationHighlight();
  static void createOverlay();
  static void updateOverlay();
  static void showOverlay(bool show);
  
  // Мониторинг производительности
  static void drawPerformanceStats();
};

#endif
