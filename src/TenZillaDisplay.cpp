/**
 * TenZillaDisplay - Драйвер дисплея на LVGL + LovyanGFX
 * 
 * ПЕРЕПИСАНО НА LVGL + LOVYANGFX:
 * - LVGL для современного GUI
 * - LovyanGFX для работы с дисплеем ILI9341
 * - Оптимизировано для ESP32-S3
 */

#include "TenZillaDisplay.h"
#include "TenZillaLGFX.h"
#include "TenZillaEncoder.h"
#include "ui/Splash_ui.h"
#include "ui/Confirmation_ui.h"
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#ifdef ESP32
  #include <esp_system.h>
#endif
#include "TenZillaScale.h"
#include "TenZillaProgram.h"
#include "TenZillaWiFi.h"
#include "TenZillaNTP.h"
#include "TenZillaScaleSettingsScreen.h"
#include "TenZillaMotorSettingsScreen.h"
#include "TenZillaOtherSettingsScreen.h"
#include "TenZillaHistoryScreen.h"

// Overlay: дата и статусы (CPU, FPS, WiFi) на lv_layer_top — общие для всех экранов, не перерисовываются при переходах
static struct {
  lv_obj_t* labelStatsWiFi;
  lv_obj_t* labelStatsFPS;
  lv_obj_t* labelStatsCPU;
  lv_obj_t* labelStatsCPU1;  // Загрузка второго ядра (Core 1)
  lv_obj_t* labelDateTime;
  lv_obj_t* labelUptime;
  bool created;
} s_overlay = {0};
static unsigned long s_overlayLastDateTimeUpdate = 0;
static unsigned long s_overlayLastUptimeUpdate = 0;

// ============================================
// ИНИЦИАЛИЗАЦИЯ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ
// ============================================

// Объекты дисплея
LGFX* TenZillaDisplay::tft = nullptr;
lv_disp_t* TenZillaDisplay::lvglDisplay = nullptr;

// Статические объекты для LVGL v8.x
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;

// Состояние дисплея
bool TenZillaDisplay::displayInitialized = false;
unsigned long TenZillaDisplay::lastUpdate = 0;
unsigned long TenZillaDisplay::lastButtonCheck = 0;
bool TenZillaDisplay::lastButtonState = HIGH;
int TenZillaDisplay::currentScreen = 0;
int TenZillaDisplay::confirmationSelection = 0; // 0=YES, 1=NO
int TenZillaDisplay::previousScreenBeforeConfirmation = 1; // По умолчанию экран 1
unsigned long TenZillaDisplay::lastNavigationTime = 0;
bool TenZillaDisplay::buttonPressed = false;
unsigned long TenZillaDisplay::buttonPressStartTime = 0;
bool TenZillaDisplay::buttonProcessed = false;

// Мониторинг производительности
float TenZillaDisplay::currentFPS = 0.0f;
float TenZillaDisplay::cpuLoadPercent = 0.0f;
unsigned long TenZillaDisplay::lastFPSUpdate = 0;
unsigned long TenZillaDisplay::frameCount = 0;
unsigned long TenZillaDisplay::lastLoopTime = 0;
unsigned long TenZillaDisplay::totalDrawTime = 0;
unsigned long TenZillaDisplay::maxDrawTime = 0;

// Данные для отображения
float TenZillaDisplay::currentWeight = 0.0f;
float TenZillaDisplay::maxWeight = 0.0f;  // Session max for display (from MainScreen/BreakScreen), not overload limit
int TenZillaDisplay::opticalCount = 0;
bool TenZillaDisplay::wifiConnected = false;
String TenZillaDisplay::wifiSSID = "";
String TenZillaDisplay::wifiIP = "";
int TenZillaDisplay::wifiRSSI = -100;
int TenZillaDisplay::wifiClients = 0;
bool TenZillaDisplay::motorRunning = false;
int TenZillaDisplay::motorDirection = 0;

// Кэш для частичного обновления
float TenZillaDisplay::lastDisplayedWeight = -9999.0f;
int TenZillaDisplay::lastDisplayedCount = -9999;
bool TenZillaDisplay::lastDisplayedWiFi = false;
String TenZillaDisplay::lastDisplayedSSID = "";
int TenZillaDisplay::lastDisplayedRSSI = -9999;
bool TenZillaDisplay::lastDisplayedMotor = false;

// LVGL экраны
lv_obj_t* TenZillaDisplay::splashScreen = nullptr;
lv_obj_t* TenZillaDisplay::mainScreen = nullptr;
lv_obj_t* TenZillaDisplay::breakScreen = nullptr;
lv_obj_t* TenZillaDisplay::wifiScreen = nullptr;
lv_obj_t* TenZillaDisplay::scaleSettingsScreen = nullptr;
lv_obj_t* TenZillaDisplay::motorSettingsScreen = nullptr;  // Настройки двигателя
lv_obj_t* TenZillaDisplay::otherSettingsScreen = nullptr;  // Прочие настройки
lv_obj_t* TenZillaDisplay::historyScreen = nullptr;       // История измерений
lv_obj_t* TenZillaDisplay::confirmationScreen = nullptr;
lv_obj_t* TenZillaDisplay::overlayContainer = nullptr;

// UI экрана подтверждения (дизайн в ui/Confirmation_ui.*)
static ConfirmationUI s_confirmationUI;

// UI заставки
static SplashUI s_splashUI;
static unsigned long splashStartTime = 0;
static bool splashActive = false;

// Буфер для LVGL (для экрана 320x480). Высота буфера — компромисс RAM/производительность.
// 50 строк ≈ 32 KB на буфер (64 KB суммарно); 100 строк давало бы 128 KB и 75% RAM.
#define LVGL_DRAW_BUF_LINES  50
static lv_color_t buf_1[SCREEN_WIDTH * LVGL_DRAW_BUF_LINES];
static lv_color_t buf_2[SCREEN_WIDTH * LVGL_DRAW_BUF_LINES];

// ============================================
// CALLBACK ФУНКЦИИ LVGL
// ============================================

void TenZillaDisplay::lvglFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
  if (tft == nullptr) {
    lv_disp_flush_ready(disp);
    return;
  }
  
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft->pushImage(area->x1, area->y1, w, h, (lgfx::rgb565_t*)color_p);
  lv_disp_flush_ready(disp);
}

// Rounder и SetPixel callbacks больше не используются в новых версиях LVGL

// ============================================
// ИНИЦИАЛИЗАЦИЯ LOVYANGFX
// ============================================

void TenZillaDisplay::initLovyanGFX()
{
  Serial.println("🖥️ Initializing LovyanGFX...");
  
  if (tft == nullptr) {
    tft = new LGFX();
    if (tft == nullptr) {
      Serial.println("❌ FATAL: Failed to allocate LGFX memory!");
      return;
    }
  }
  
  tft->init();
  
  // Для ILI9488 правильная ориентация
  // rotation 1 = правильная ориентация (исправлено: было повернуто на 90° влево)
  tft->setRotation(1);
  tft->setBrightness(255);
  
  // Краткая проверка дисплея (без длинного мигания — при boot loop не мешает)
  tft->fillScreen(TFT_BLACK);
  delay(50);
  
  Serial.println("✅ LovyanGFX initialized");
  Serial.println("📏 Display size: " + String(tft->width()) + "x" + String(tft->height()));
  
  // Дополнительная диагностика для ILI9488
  if (tft->width() != 320 || tft->height() != 480) {
    Serial.println("⚠️ WARNING: Display size mismatch!");
    Serial.println("   Expected: 320x480");
    Serial.println("   Actual: " + String(tft->width()) + "x" + String(tft->height()));
    Serial.println("   Current rotation: 1");
  }
}

// ============================================
// ИНИЦИАЛИЗАЦИЯ LVGL
// ============================================

void TenZillaDisplay::initLVGL()
{
  Serial.println("🎨 Initializing LVGL...");
  
  lv_init();
  
  // В LVGL 8.x используем правильное альбомное разрешение (480x320)
  // так как LovyanGFX уже повернут
  int hor_res = 480;
  int ver_res = 320;
  
  // Инициализация буфера отрисовки (меньший буфер — экономия RAM при приемлемой скорости)
  lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, SCREEN_WIDTH * LVGL_DRAW_BUF_LINES);
  
  // Инициализация драйвера дисплея
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = hor_res;
  disp_drv.ver_res = ver_res;
  disp_drv.flush_cb = lvglFlushCallback;
  disp_drv.draw_buf = &draw_buf;
  
  // Оптимизация для динамического частичного обновления
  // Включаем частичное обновление (refresh только измененных областей)
  disp_drv.full_refresh = 0;  // 0 = частичное обновление (быстрее)
  // LVGL автоматически определяет минимальную область для обновления
  // и вызывает flush_cb только для измененных пикселей
  
  // Дополнительные настройки для оптимизации частичного обновления:
  // - LVGL кэширует области и обновляет только при изменении
  // - Используйте lv_obj_invalidate() для принудительного обновления конкретных объектов
  // - Используйте lv_obj_invalidate_area() для обновления конкретной области
  
  // Регистрация драйвера
  lvglDisplay = lv_disp_drv_register(&disp_drv);
  
  Serial.println("✅ LVGL initialized (v8.x)");
}

// ============================================
// ОСНОВНЫЕ МЕТОДЫ
// ============================================

void TenZillaDisplay::begin() {
  Serial.println("🖥️ Initializing display with LVGL + LovyanGFX...");
  
  // Инициализация кнопки
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(BUTTON_PIN);
  Serial.println("✅ Button initialized on GPIO " + String(BUTTON_PIN));
  
  // Инициализация подсветки
  pinMode(DISPLAY_LED_PIN, OUTPUT);
  digitalWrite(DISPLAY_LED_PIN, HIGH);
  
  // Инициализация LovyanGFX
  initLovyanGFX();
  if (tft == nullptr) {
    Serial.println("❌ Failed to initialize LovyanGFX!");
    return;
  }
  
  // Инициализация LVGL
  initLVGL();
  if (lvglDisplay == nullptr) {
    Serial.println("❌ Failed to initialize LVGL!");
    return;
  }
  
  displayInitialized = true;
  
  // Инициализация переменных мониторинга производительности
  lastFPSUpdate = millis();
  frameCount = 0;
  currentFPS = 0.0f;
  cpuLoadPercent = 0.0f;
  
  Serial.println("✅ Display initialized with LVGL + LovyanGFX");
  Serial.println("📊 Performance monitoring enabled");
  
  #ifdef ESP32
    Serial.println("🔧 CPU Frequency: " + String(getCpuFrequencyMhz()) + " MHz");
  #endif
  
  createOverlay();
  showOverlay(false);
  showSplashScreen();
}

void TenZillaDisplay::showSplashScreen() {
  if (!displayInitialized) return;
  currentScreen = 0;
  showOverlay(false);

  if (splashScreen == nullptr) {
    Splash_ui_create(&s_splashUI);
    splashScreen = s_splashUI.screen;
  }

  lv_scr_load(splashScreen);
  splashStartTime = millis();
  splashActive = true;
  
  // Сбрасываем прогресс-бар
  if (s_splashUI.progressBar != nullptr) {
    lv_bar_set_value(s_splashUI.progressBar, 0, LV_ANIM_OFF);
    Serial.println("✅ Progress bar initialized");
  } else {
    Serial.println("❌ Progress bar is NULL!");
  }
  
  // Принудительное обновление экрана после загрузки
  lv_refr_now(lvglDisplay);
  
  lastDisplayedWeight = -9999.0f;
  lastDisplayedCount = -9999;
  lastDisplayedWiFi = false;
  lastDisplayedSSID = "";
  lastDisplayedRSSI = -9999;
  lastDisplayedMotor = false;
  Serial.println("📱 Splash screen displayed");
}

// ============================================
// УПРАВЛЕНИЕ ЭКРАНАМИ
// ============================================

void TenZillaDisplay::showMainScreen() {
  if (!displayInitialized) return;
  currentScreen = 1;
  if (mainScreen == nullptr) TenZillaMainScreen::createLVGL(mainScreen);
  if (mainScreen != nullptr) lv_scr_load(mainScreen);
  showOverlay(true);
}

void TenZillaDisplay::showBreakScreen() {
  if (!displayInitialized) return;
  currentScreen = 2;
  if (breakScreen == nullptr) TenZillaBreakScreen::createLVGL(breakScreen);
  if (breakScreen != nullptr) lv_scr_load(breakScreen);
  showOverlay(true);
}

void TenZillaDisplay::showWiFiScreen() {
  if (!displayInitialized) return;
  currentScreen = 3;
  if (wifiScreen == nullptr) TenZillaWifiScreen::createLVGL(wifiScreen);
  if (wifiScreen != nullptr) lv_scr_load(wifiScreen);
  showOverlay(true);
}

void TenZillaDisplay::showScaleSettingsScreen() {
  if (!displayInitialized) return;
  currentScreen = 4;
  if (scaleSettingsScreen == nullptr) TenZillaScaleSettingsScreen::createLVGL(scaleSettingsScreen);
  if (scaleSettingsScreen != nullptr) lv_scr_load(scaleSettingsScreen);
  showOverlay(true);
}

void TenZillaDisplay::showMotorSettingsScreen() {
  if (!displayInitialized) return;
  currentScreen = 5;
  if (motorSettingsScreen == nullptr) TenZillaMotorSettingsScreen::createLVGL(motorSettingsScreen);
  if (motorSettingsScreen != nullptr) lv_scr_load(motorSettingsScreen);
  showOverlay(true);
}

void TenZillaDisplay::showOtherSettingsScreen() {
  if (!displayInitialized) return;
  currentScreen = 6;
  if (otherSettingsScreen == nullptr) TenZillaOtherSettingsScreen::createLVGL(otherSettingsScreen);
  if (otherSettingsScreen != nullptr) lv_scr_load(otherSettingsScreen);
  showOverlay(true);
}

void TenZillaDisplay::showHistoryScreen() {
  if (!displayInitialized) return;
  currentScreen = 7;
  if (historyScreen == nullptr) TenZillaHistoryScreen::createLVGL(historyScreen);
  if (historyScreen != nullptr) lv_scr_load(historyScreen);
  showOverlay(true);
}

void TenZillaDisplay::nextScreen() {
  if (!displayInitialized) return;
  
  // Блокируем навигацию, если программа запущена
  if (TenZillaProgram::isRunning()) {
    return;
  }
  
  // Защита от недопустимых состояний экрана
  if (currentScreen == 8 || currentScreen < 0) {
    // Если мы в меню подтверждения или недопустимом состоянии, возвращаемся на главный экран
    if (currentScreen == 8) {
      currentScreen = 1;  // Сбрасываем на главный экран
      showMainScreen();
    }
    return;
  }
  
  if (currentScreen == 0) {
    showMainScreen();
    return;
  }
  
  // Переход на следующий экран (1-7, циклически)
  int next = currentScreen + 1;
  if (next > 7) next = 1;
  currentScreen = next;
  
  switch (currentScreen) {
    case 1: showMainScreen(); break;
    case 2: showBreakScreen(); break;
    case 3: showWiFiScreen(); break;
    case 4: showScaleSettingsScreen(); break;
    case 5: showMotorSettingsScreen(); break;
    case 6: showOtherSettingsScreen(); break;
    case 7: showHistoryScreen(); break;
    default:
      currentScreen = 1;
      showMainScreen();
      break;
  }
}

// ============================================
// ОБНОВЛЕНИЕ И КНОПКА
// ============================================

void TenZillaDisplay::update() {
  if (!displayInitialized) return;
  
  unsigned long now = millis();
  unsigned long loopStartTime = micros();
  
  // Обработка LVGL таймеров (один вызов достаточно)
  lv_timer_handler();
  
  // Обновление градиентной закраски кнопок меню во время длинного нажатия
  if (currentScreen == 8 && buttonPressed && buttonPressStartTime > 0) {
    unsigned long pressDuration = now - buttonPressStartTime;
    const unsigned long LONG_PRESS_TIME = 2000;
    if (pressDuration < LONG_PRESS_TIME) {
      // Обновляем градиент каждые 50ms для плавной анимации
      static unsigned long lastGradientUpdate = 0;
      if (now - lastGradientUpdate >= 50) {
        updateConfirmationHighlight();
        lastGradientUpdate = now;
      }
    }
  }
  
  // Обновление заставки с прогресс-баром
  if (splashActive && currentScreen == 0) {
    if (s_splashUI.progressBar != nullptr) {
      unsigned long elapsed = now - splashStartTime;
      const unsigned long SPLASH_DURATION_MS = 3000; // 3 секунды
      
      if (elapsed >= SPLASH_DURATION_MS) {
        // Завершение загрузки - переключаемся на главный экран
        splashActive = false;
        lv_bar_set_value(s_splashUI.progressBar, 100, LV_ANIM_ON);
        lv_refr_now(lvglDisplay); // Принудительное обновление экрана
        delay(100); // Небольшая задержка для завершения анимации
        showMainScreen();
        return;
      } else {
        // Обновляем прогресс-бар (0-100%)
        int progress = (elapsed * 100) / SPLASH_DURATION_MS;
        if (progress > 100) progress = 100;
        
        // Обновляем только если значение изменилось (для оптимизации)
        static int lastProgress = -1;
        if (progress != lastProgress) {
          lv_bar_set_value(s_splashUI.progressBar, progress, LV_ANIM_OFF);
          lastProgress = progress;
          // Принудительное обновление каждые 100мс для плавности
          static unsigned long lastRefresh = 0;
          if (now - lastRefresh >= 100) {
            lv_refr_now(lvglDisplay);
            lastRefresh = now;
          }
        }
      }
    }
  }
  
  checkButton();

  if (!splashActive && currentScreen >= 1 && currentScreen <= 7)
    updateOverlay();
  
  unsigned long drawStartTime = 0;
  bool screenUpdated = false;
  bool dataChanged = false;
  
  if (currentScreen != 0) {
    // Проверяем, изменились ли данные перед обновлением экрана
    // Это значительно уменьшает количество ненужных перерисовок
    switch(currentScreen) {
      case 1: 
        // Главный экран: по изменению веса/счётчика или периодически (мигание "Waiting" перед стартом)
        if (fabs(currentWeight - lastDisplayedWeight) >= 0.1f || 
            opticalCount != lastDisplayedCount) {
          dataChanged = true;
        } else {
          static unsigned long lastMainScreenUpdate = 0;
          if (now - lastMainScreenUpdate >= 250) {
            dataChanged = true;
            lastMainScreenUpdate = now;
          }
        }
        break;
      case 2: 
        // Экран разрыва: по изменению веса/счётчика или периодически (чтобы отображался лимит LIM)
        if (fabs(currentWeight - lastDisplayedWeight) >= 0.1f || 
            opticalCount != lastDisplayedCount) {
          dataChanged = true;
        } else {
          static unsigned long lastBreakScreenUpdate = 0;
          if (now - lastBreakScreenUpdate >= 250) {
            dataChanged = true;
            lastBreakScreenUpdate = now;
          }
        }
        break;
      case 3: 
        // Проверяем изменения для WiFi экрана
        if (wifiConnected != lastDisplayedWiFi || 
            wifiSSID != lastDisplayedSSID || 
            wifiRSSI != lastDisplayedRSSI) {
          dataChanged = true;
        }
        break;
      case 4: 
        // Для экрана настроек весов обновляем периодически (все значения должны отображаться)
        static unsigned long lastScaleSettingsUpdate = 0;
        if (now - lastScaleSettingsUpdate >= 250) {
          dataChanged = true;
          lastScaleSettingsUpdate = now;
        }
        break;
      case 5: 
        // Для экрана настроек двигателя обновляем реже
        static unsigned long lastMotorSettingsUpdate = 0;
        if (now - lastMotorSettingsUpdate >= 500) {
          dataChanged = true;
          lastMotorSettingsUpdate = now;
        }
        break;
      case 6: 
        // Для экрана прочих настроек обновляем реже
        static unsigned long lastOtherSettingsUpdate = 0;
        if (now - lastOtherSettingsUpdate >= 1000) {
          dataChanged = true;
          lastOtherSettingsUpdate = now;
        }
        break;
      case 7: {
        // История измерений — обновляем раз в 2 с (проверка новых записей)
        static unsigned long lastHistoryUpdate = 0;
        if (now - lastHistoryUpdate >= 2000) {
          dataChanged = true;
          lastHistoryUpdate = now;
        }
        break;
      }
      case 8: 
        // Confirmation screen updates handled in checkButton
        break;
    }
    
    // Обновляем экран только если данные изменились
    if (dataChanged) {
      drawStartTime = micros();
      screenUpdated = true;
      
      switch(currentScreen) {
        case 1: 
          if (mainScreen != nullptr) {
            TenZillaMainScreen::updateLVGL(mainScreen, currentWeight, maxWeight, opticalCount);
            // Обновляем кэш после обновления
            lastDisplayedWeight = currentWeight;
            lastDisplayedCount = opticalCount;
          }
          break;
        case 2: 
          if (breakScreen != nullptr) {
            TenZillaBreakScreen::updateLVGL(breakScreen, currentWeight, maxWeight, opticalCount);
            // Обновляем кэш после обновления
            lastDisplayedWeight = currentWeight;
            lastDisplayedCount = opticalCount;
          }
          break;
        case 3: 
          if (wifiScreen != nullptr) {
            TenZillaWifiScreen::updateLVGL(wifiScreen, wifiConnected, wifiSSID, wifiIP, wifiRSSI, wifiClients);
            // Обновляем кэш после обновления
            lastDisplayedWiFi = wifiConnected;
            lastDisplayedSSID = wifiSSID;
            lastDisplayedRSSI = wifiRSSI;
          }
          break;
        case 4: 
          if (scaleSettingsScreen != nullptr) {
            TenZillaScaleSettingsScreen::updateLVGL(scaleSettingsScreen, currentWeight);
            lastDisplayedWeight = currentWeight;
          }
          break;
        case 5: 
          if (motorSettingsScreen != nullptr) {
            TenZillaMotorSettingsScreen::updateLVGL(motorSettingsScreen);
          }
          break;
        case 6: 
          if (otherSettingsScreen != nullptr) {
            TenZillaOtherSettingsScreen::updateLVGL(otherSettingsScreen);
          }
          break;
        case 7: 
          if (historyScreen != nullptr) {
            TenZillaHistoryScreen::updateLVGL(historyScreen);
          }
          break;
        case 8: 
          // Confirmation screen updates handled in checkButton
          break;
      }
      
      // Измеряем время отрисовки только при реальных обновлениях
      if (drawStartTime > 0) {
        unsigned long drawTime = micros() - drawStartTime;
        frameCount++; // Считаем только реальные обновления экрана
        
        if (drawTime > 100) {
          totalDrawTime += drawTime;
          if (drawTime > maxDrawTime) {
            maxDrawTime = drawTime;
          }
        }
      }
    }
  }
  
  
  // Расчет FPS и загрузки CPU каждую секунду
  // Считаем только реальные обновления экрана (когда screenUpdated == true)
  if (now - lastFPSUpdate >= 1000) {
    unsigned long elapsed = now - lastFPSUpdate;
    
    // FPS рассчитываем на основе реальных обновлений экрана
    // frameCount увеличивается только при реальных обновлениях данных
    if (frameCount > 0) {
      currentFPS = (frameCount * 1000.0f) / elapsed;
      // Ограничиваем максимальное значение FPS разумным пределом (60 FPS)
      if (currentFPS > 60.0f) currentFPS = 60.0f;
    } else {
      currentFPS = 0.0f;
    }
    
    if (totalDrawTime > 0) {
      unsigned long totalDrawTimeMs = totalDrawTime / 1000;
      cpuLoadPercent = (totalDrawTimeMs * 100.0f) / elapsed;
      if (cpuLoadPercent > 100.0f) cpuLoadPercent = 100.0f;
    } else {
      cpuLoadPercent = 0.0f;
    }
    
    lastFPSUpdate = now;
    frameCount = 0;
    totalDrawTime = 0;
    maxDrawTime = 0;
  }
  
  lastLoopTime = micros() - loopStartTime;
}

void TenZillaDisplay::checkButton() {
  if (!displayInitialized) return;
  
  unsigned long now = millis();
  bool buttonState = digitalRead(BUTTON_PIN);
  
  const unsigned long SHORT_PRESS_TIME = 600;
  const unsigned long LONG_PRESS_TIME = 2000;
  const unsigned long DEBOUNCE_TIME = 50;
  
  // Обработка экрана меню (MENU: START / RESET MOV / RESET ZERO / EXIT)
  if (currentScreen == 8) {
    bool showStart = (previousScreenBeforeConfirmation == 1 || previousScreenBeforeConfirmation == 2);
    int maxSel = showStart ? 3 : 2;

    if (buttonState == LOW && lastButtonState == HIGH) {
      buttonPressed = true;
      buttonPressStartTime = now;
      buttonProcessed = false;
    } else if (buttonState == LOW && buttonPressed && !buttonProcessed) {
      unsigned long pressDuration = now - buttonPressStartTime;

      // Обновляем градиентную закраску во время длинного нажатия
      if (pressDuration < LONG_PRESS_TIME) {
        updateConfirmationHighlight();
      }

      if (pressDuration >= LONG_PRESS_TIME) {
        buttonProcessed = true;
        buttonPressed = false;
        buttonPressStartTime = 0;
        int action = showStart ? confirmationSelection : (confirmationSelection + 1);
        /* 0=START, 1=RESET MOV, 2=RESET ZERO, 3=EXIT */

        // Сбрасываем градиент перед выходом
        updateConfirmationHighlight();

        currentScreen = previousScreenBeforeConfirmation;
        switch (previousScreenBeforeConfirmation) {
          case 1: showMainScreen(); break;
          case 2: showBreakScreen(); break;
          case 3: showWiFiScreen(); break;
          case 4: showScaleSettingsScreen(); break;
          case 5: showMotorSettingsScreen(); break;
          case 6: showOtherSettingsScreen(); break;
          case 7: showHistoryScreen(); break;
          default: showMainScreen(); break;
        }
        showOverlay(true);
        lv_timer_handler();
        lv_obj_t* act = lv_scr_act();
        if (act != nullptr) lv_obj_invalidate(act);
        if (overlayContainer != nullptr) lv_obj_invalidate(overlayContainer);
        lv_refr_now(lvglDisplay);
        
        // ВАЖНО: Полный сброс состояния кнопки после выхода из меню
        // Это предотвращает зависание навигации
        lastNavigationTime = now;  // Устанавливаем текущее время, а не 0
        buttonPressed = false;
        buttonPressStartTime = 0;
        buttonProcessed = false;
        lastButtonState = HIGH;  // Сбрасываем в HIGH для корректной обработки следующего нажатия

        if (action == 0) {
          if (previousScreenBeforeConfirmation == 1) TenZillaProgram::startCompressionProgram();
          else if (previousScreenBeforeConfirmation == 2) TenZillaProgram::startBreakProgram();
        } else if (action == 1) {
          TenZillaScale::setPendingMenuAction(1);
        } else if (action == 2) {
          TenZillaScale::setPendingMenuAction(2);
        }
        /* action == 3: EXIT — только возврат */
        
        // Возвращаемся сразу после обработки, чтобы не обрабатывать отпускание кнопки
        lastButtonCheck = now;
        return;
      }
    } else if (buttonState == HIGH && buttonPressed && !buttonProcessed) {
      unsigned long pressDuration = now - buttonPressStartTime;

      if (pressDuration >= DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME) {
        confirmationSelection = (confirmationSelection + 1) % (maxSel + 1);
        updateConfirmationHighlight();  // Сбрасываем градиент при переключении
        buttonProcessed = true;
      }

      buttonPressed = false;
      buttonPressStartTime = 0;
      // Сбрасываем градиент при отпускании кнопки
      updateConfirmationHighlight();
    } else if (buttonState == HIGH && buttonPressed && buttonProcessed) {
      buttonPressed = false;
      buttonPressStartTime = 0;
      buttonProcessed = false;
      // Сбрасываем градиент при отпускании кнопки
      updateConfirmationHighlight();
    }

    lastButtonState = buttonState;
    lastButtonCheck = now;
    return;
  }

  // Обработка основных экранов (1–6)
  if (buttonState == LOW && lastButtonState == HIGH) {
    buttonPressed = true;
    buttonPressStartTime = now;
    buttonProcessed = false;
  } else if (buttonState == LOW && buttonPressed && !buttonProcessed) {
    unsigned long pressDuration = now - buttonPressStartTime;

    if (TenZillaProgram::isRunning() && pressDuration >= LONG_PRESS_TIME) {
      TenZillaProgram::stopProgram();
      TenZillaProgram::beepShort();
      buttonProcessed = true;
      buttonPressed = false;
      buttonPressStartTime = 0;
      lastButtonState = buttonState;
      lastButtonCheck = now;
      return;
    }

    if (!TenZillaProgram::isRunning() && currentScreen >= 1 && currentScreen <= 7 && pressDuration >= LONG_PRESS_TIME) {
      previousScreenBeforeConfirmation = currentScreen;
      buttonProcessed = true;
      buttonPressed = false;
      buttonPressStartTime = 0;
      showConfirmationScreen();
      lastButtonState = buttonState;
      lastButtonCheck = now;
      return;
    }
  } else if (buttonState == HIGH && buttonPressed && !buttonProcessed) {
    unsigned long pressDuration = now - buttonPressStartTime;
    
    if (pressDuration >= DEBOUNCE_TIME && pressDuration < SHORT_PRESS_TIME) {
      if (TenZillaProgram::isRunning()) {
        buttonProcessed = true;
      } else if (now - lastNavigationTime > 300) {
        nextScreen();
        lastNavigationTime = now;
        buttonProcessed = true;
      }
    }
    
    buttonPressed = false;
    buttonPressStartTime = 0;
  } else if (buttonState == HIGH && buttonPressed && buttonProcessed) {
    buttonPressed = false;
    buttonPressStartTime = 0;
    buttonProcessed = false;
  }
  
  lastButtonState = buttonState;
  lastButtonCheck = now;
}

// ============================================
// OVERLAY (дата, CPU, FPS, WiFi) — общий для всех экранов, не перерисовывается при переходах
// ============================================

void TenZillaDisplay::createOverlay() {
  if (!displayInitialized || s_overlay.created) return;
  lv_obj_t* top = lv_layer_top();
  if (top == nullptr) return;

  overlayContainer = lv_obj_create(top);
  lv_obj_set_size(overlayContainer, 480, 320);
  lv_obj_set_pos(overlayContainer, 0, 0);
  lv_obj_set_style_bg_opa(overlayContainer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(overlayContainer, 0, 0);
  lv_obj_set_style_pad_all(overlayContainer, 0, 0);
  lv_obj_clear_flag(overlayContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(overlayContainer, LV_OBJ_FLAG_CLICKABLE);

  const lv_font_t* font14 = &lv_font_montserrat_14;
  const lv_font_t* font18 = &lv_font_montserrat_18;

  // Статусы прижаты к правому краю (не налезают на заголовки слева)
  s_overlay.labelStatsWiFi = lv_label_create(overlayContainer);
  lv_label_set_text(s_overlay.labelStatsWiFi, "DBi \x2D\x2D\x2D");
  lv_obj_set_style_text_font(s_overlay.labelStatsWiFi, font18, 0);
  lv_obj_set_style_text_color(s_overlay.labelStatsWiFi, lv_color_make(0, 255, 0), 0);
  lv_obj_align(s_overlay.labelStatsWiFi, LV_ALIGN_TOP_RIGHT, -140, 4);

  s_overlay.labelStatsFPS = lv_label_create(overlayContainer);
  lv_label_set_text(s_overlay.labelStatsFPS, "FPS:0");
  lv_obj_set_style_text_font(s_overlay.labelStatsFPS, font18, 0);
  lv_obj_set_style_text_color(s_overlay.labelStatsFPS, lv_color_make(0, 255, 0), 0);
  lv_obj_align(s_overlay.labelStatsFPS, LV_ALIGN_TOP_RIGHT, -80, 4);

  s_overlay.labelStatsCPU = lv_label_create(overlayContainer);
  lv_label_set_text(s_overlay.labelStatsCPU, "CPU:0%");
  lv_obj_set_style_text_font(s_overlay.labelStatsCPU, font18, 0);
  lv_obj_set_style_text_color(s_overlay.labelStatsCPU, lv_color_make(0, 255, 0), 0);
  lv_obj_align(s_overlay.labelStatsCPU, LV_ALIGN_TOP_RIGHT, -10, 4);

  s_overlay.labelStatsCPU1 = nullptr;

  s_overlay.labelDateTime = lv_label_create(overlayContainer);
  lv_label_set_text(s_overlay.labelDateTime, "\x2D\x2D.\x2D\x2D.\x2D\x2D\x2D\x2D \x2D\x2D:\x2D\x2D:\x2D\x2D");
  lv_obj_set_style_text_font(s_overlay.labelDateTime, font14, 0);
  lv_obj_set_style_text_color(s_overlay.labelDateTime, lv_color_make(170, 170, 170), 0);
  lv_obj_align(s_overlay.labelDateTime, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

  s_overlay.labelUptime = lv_label_create(overlayContainer);
  lv_label_set_text(s_overlay.labelUptime, "UPTIME: 0:00:00:00");
  lv_obj_set_style_text_font(s_overlay.labelUptime, font14, 0);
  lv_obj_set_style_text_color(s_overlay.labelUptime, lv_color_make(170, 170, 170), 0);
  lv_obj_align(s_overlay.labelUptime, LV_ALIGN_BOTTOM_MID, 0, -10);

  s_overlay.created = true;
}

void TenZillaDisplay::updateOverlay() {
  if (!s_overlay.created || overlayContainer == nullptr) return;
  if (lv_obj_has_flag(overlayContainer, LV_OBJ_FLAG_HIDDEN)) return;

  static unsigned long lastOverlayUpdate = 0;
  unsigned long now = millis();
  if (lastOverlayUpdate != 0 && now - lastOverlayUpdate < 150) return;
  lastOverlayUpdate = now;

  char buffer[32];
  float cpuLoad = getCPULoad();
  float fps = getFPS();
  int rssi = getWiFiRSSI();

  if (s_overlay.labelStatsCPU != nullptr) {
    snprintf(buffer, sizeof(buffer), "CPU:%.0f%%", cpuLoad);
    lv_label_set_text(s_overlay.labelStatsCPU, buffer);
    float cpuRatio = (cpuLoad - 50.0f) / 30.0f;
    if (cpuRatio < 0.0f) cpuRatio = 0.0f;
    if (cpuRatio > 1.0f) cpuRatio = 1.0f;
    uint8_t r = (uint8_t)(255 * cpuRatio);
    uint8_t g = (uint8_t)(255 * (1.0f - cpuRatio));
    uint8_t b = 0;
    lv_obj_set_style_text_color(s_overlay.labelStatsCPU, lv_color_make(b, g, r), 0);
  }

  if (s_overlay.labelStatsFPS != nullptr) {
    snprintf(buffer, sizeof(buffer), "FPS:%.0f", fps);
    lv_label_set_text(s_overlay.labelStatsFPS, buffer);
    float fpsRatio = (15.0f - fps) / 15.0f;
    if (fpsRatio < 0.0f) fpsRatio = 0.0f;
    if (fpsRatio > 1.0f) fpsRatio = 1.0f;
    uint8_t r = (uint8_t)(255 * fpsRatio);
    uint8_t g = (uint8_t)(255 * (1.0f - fpsRatio));
    uint8_t b = 0;
    lv_obj_set_style_text_color(s_overlay.labelStatsFPS, lv_color_make(b, g, r), 0);
  }
  if (s_overlay.labelStatsWiFi != nullptr) {
    if (rssi > -100) snprintf(buffer, sizeof(buffer), "DBi %d", rssi);
    else snprintf(buffer, sizeof(buffer), "DBi \x2D\x2D\x2D");
    lv_label_set_text(s_overlay.labelStatsWiFi, buffer);
    float wifiRatio = 0.0f;
    if (rssi > -100) {
      wifiRatio = (-85.0f - rssi) / 15.0f;
      if (wifiRatio < 0.0f) wifiRatio = 0.0f;
      if (wifiRatio > 1.0f) wifiRatio = 1.0f;
    } else wifiRatio = 1.0f;
    uint8_t r = (uint8_t)(255 * wifiRatio);
    uint8_t g = (uint8_t)(255 * (1.0f - wifiRatio));
    uint8_t b = 0;
    lv_obj_set_style_text_color(s_overlay.labelStatsWiFi, lv_color_make(b, g, r), 0);
  }

  if (s_overlay.labelDateTime != nullptr &&
      (s_overlayLastDateTimeUpdate == 0 || now - s_overlayLastDateTimeUpdate >= 1000)) {
    s_overlayLastDateTimeUpdate = now;
    struct tm timeinfo;
    bool timeValid = false;
    
    // Используем только NTP для получения времени
    if (TenZillaNTP::isTimeSynced()) {
#ifdef ESP32
      timeValid = getLocalTime(&timeinfo, 0);
#endif
    }
    
    if (timeValid) {
#ifdef ESP32
      snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d %02d:%02d:%02d",
               (int)timeinfo.tm_mday, (int)timeinfo.tm_mon + 1, (int)timeinfo.tm_year + 1900,
               (int)timeinfo.tm_hour, (int)timeinfo.tm_min, (int)timeinfo.tm_sec);
#else
      snprintf(buffer, sizeof(buffer), "--.--.---- --:--:--");
#endif
    } else {
      // Если время не синхронизировано, показываем статус
      snprintf(buffer, sizeof(buffer), "NTP: Not synced");
    }
    lv_label_set_text(s_overlay.labelDateTime, buffer);
  }

  // Обновление uptime (внизу по центру)
  if (s_overlay.labelUptime != nullptr &&
      (s_overlayLastUptimeUpdate == 0 || now - s_overlayLastUptimeUpdate >= 1000)) {
    s_overlayLastUptimeUpdate = now;
    String uptimeStr = TenZillaNTP::getUptimeString();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "UPTIME: %s", uptimeStr.c_str());
    lv_label_set_text(s_overlay.labelUptime, buffer);
  }
}

void TenZillaDisplay::showOverlay(bool show) {
  if (overlayContainer == nullptr) return;
  if (show)
    lv_obj_clear_flag(overlayContainer, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(overlayContainer, LV_OBJ_FLAG_HIDDEN);
}

// ============================================
// ЭКРАН ПОДТВЕРЖДЕНИЯ
// ============================================

static const lv_color_t s_menuBtnColors[4] = {
  lv_color_make(0, 255, 0),    /* 0 START green */
  lv_color_make(0, 255, 255),  /* 1 RESET MOV cyan */
  lv_color_make(0, 136, 255),  /* 2 RESET ZERO orange */
  lv_color_make(0, 0, 255)     /* 3 EXIT red BGR */
};

void TenZillaDisplay::updateConfirmationHighlight() {
  if (confirmationScreen == nullptr) return;
  bool showStart = (previousScreenBeforeConfirmation == 1 || previousScreenBeforeConfirmation == 2);
  int btnIndex = showStart ? confirmationSelection : (confirmationSelection + 1);
  lv_obj_t* btns[4] = { s_confirmationUI.btnStart, s_confirmationUI.btnResetMov, s_confirmationUI.btnResetZero, s_confirmationUI.btnExit };
  
  // Вычисляем прогресс длинного нажатия (0-100%)
  float pressProgress = 0.0f;
  if (buttonPressed && buttonPressStartTime > 0 && currentScreen == 8) {
    unsigned long now = millis();
    unsigned long pressDuration = now - buttonPressStartTime;
    const unsigned long LONG_PRESS_TIME = 2000;
    if (pressDuration < LONG_PRESS_TIME) {
      pressProgress = (float)pressDuration / (float)LONG_PRESS_TIME;
      if (pressProgress > 1.0f) pressProgress = 1.0f;
    } else {
      pressProgress = 1.0f;
    }
  }
  
  for (int i = 0; i < 4; i++) {
    if (!btns[i]) continue;
    if (i == 0 && !showStart) continue;
    
    bool isSelected = (i == btnIndex);
    lv_obj_set_style_border_width(btns[i], isSelected ? 4 : 2, 0);
    lv_obj_set_style_border_color(btns[i], isSelected ? lv_color_make(0, 255, 255) : s_menuBtnColors[i], 0);
    
    // Градиентная закраска выбранной кнопки во время длинного нажатия
    if (isSelected && pressProgress > 0.0f) {
      // Получаем цвет кнопки и используем его напрямую
      lv_color_t btnColor = s_menuBtnColors[i];
      
      // Вычисляем прозрачность фона с градиентом: от прозрачного (0%) до непрозрачного (100%)
      uint8_t bgOpacity = (uint8_t)(pressProgress * 255.0f);
      
      lv_obj_set_style_bg_color(btns[i], btnColor, 0);
      lv_obj_set_style_bg_opa(btns[i], bgOpacity, 0);
    } else {
      // Сбрасываем фон к прозрачному, если кнопка не выбрана или нажатие не активно
      lv_obj_set_style_bg_opa(btns[i], LV_OPA_TRANSP, 0);
    }
  }
}

void TenZillaDisplay::showConfirmationScreen() {
  if (!displayInitialized) return;
  currentScreen = 8;
  confirmationSelection = 0;
  showOverlay(false);

  if (confirmationScreen == nullptr) {
    Confirmation_ui_create(&confirmationScreen, &s_confirmationUI);
  }

  bool showStart = (previousScreenBeforeConfirmation == 1 || previousScreenBeforeConfirmation == 2);
  if (s_confirmationUI.btnStart) {
    if (showStart)
      lv_obj_clear_flag(s_confirmationUI.btnStart, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(s_confirmationUI.btnStart, LV_OBJ_FLAG_HIDDEN);
  }

  updateConfirmationHighlight();
  lv_scr_load(confirmationScreen);
}

// ============================================
// МЕТОДЫ ОБНОВЛЕНИЯ ДАННЫХ
// ============================================

void TenZillaDisplay::updateWeight(float weight) {
  currentWeight = weight;
}

void TenZillaDisplay::updateMaxWeight(float maxWeight) {
  TenZillaDisplay::maxWeight = maxWeight;
}

void TenZillaDisplay::updateOpticalCount(int count) {
  opticalCount = count;
}

void TenZillaDisplay::updateWiFiStatus(bool connected, String ssid) {
  wifiConnected = connected;
  wifiSSID = ssid;
}

void TenZillaDisplay::updateWiFiIP(String ip) {
  wifiIP = ip;
}

void TenZillaDisplay::updateWiFiClients(int clients) {
  wifiClients = clients;
}

void TenZillaDisplay::updateMotorStatus(bool running, int direction) {
  motorRunning = running;
  motorDirection = direction;
}

void TenZillaDisplay::updateRSSI(int rssi) {
  wifiRSSI = rssi;
}

void TenZillaDisplay::forceUpdateScreen() {
  if (!displayInitialized) return;
  
  // Принудительно обновляем активный экран без проверки интервала
  unsigned long now = millis();
  unsigned long drawStartTime = micros();
  
  switch(currentScreen) {
    case 1: 
      if (mainScreen != nullptr) {
        TenZillaMainScreen::updateLVGL(mainScreen, currentWeight, maxWeight, opticalCount);
      }
      break;
    case 2: 
      if (breakScreen != nullptr) {
        TenZillaBreakScreen::updateLVGL(breakScreen, currentWeight, maxWeight, opticalCount);
      }
      break;
    case 3: 
      if (wifiScreen != nullptr) {
        TenZillaWifiScreen::updateLVGL(wifiScreen, wifiConnected, wifiSSID, wifiIP, wifiRSSI, wifiClients);
      }
      break;
    case 4: 
      if (scaleSettingsScreen != nullptr) {
        TenZillaScaleSettingsScreen::updateLVGL(scaleSettingsScreen, currentWeight);
      }
      break;
    case 5: 
      if (motorSettingsScreen != nullptr) {
        TenZillaMotorSettingsScreen::updateLVGL(motorSettingsScreen);
      }
      break;
    case 6: 
      if (otherSettingsScreen != nullptr) {
        TenZillaOtherSettingsScreen::updateLVGL(otherSettingsScreen);
      }
      break;
    case 7: 
      if (historyScreen != nullptr) {
        TenZillaHistoryScreen::updateLVGL(historyScreen);
      }
      break;
  }
  
  lv_refr_now(lvglDisplay);
  
  // lastUpdate больше не используется - обновление данных происходит всегда
}

void TenZillaDisplay::resetMainScreenMax() {
  TenZillaMainScreen::resetMaxWeight();
}

void TenZillaDisplay::resetBreakScreenMax() {
  TenZillaBreakScreen::resetMaxWeight();
}

float TenZillaDisplay::getMainScreenMax() {
  return TenZillaMainScreen::getMaxWeight();
}

float TenZillaDisplay::getBreakScreenMax() {
  return TenZillaBreakScreen::getMaxWeight();
}

int TenZillaDisplay::getCurrentScreen() {
  return currentScreen;
}

lv_disp_t* TenZillaDisplay::getDisplay() {
  return lvglDisplay;
}

float TenZillaDisplay::getCPULoad() {
  return cpuLoadPercent;
}

// Загрузка Core 1 (веб-сервер). vTaskGetRunTimeStats в Arduino-ESP32 по умолчанию
// отключён, точная метрика недоступна. Возвращаем 0.
float TenZillaDisplay::getCPULoadCore1() {
  (void)0;
  return 0.0f;
}

float TenZillaDisplay::getFPS() {
  return currentFPS;
}

int TenZillaDisplay::getWiFiRSSI() {
  return wifiRSSI;
}

void TenZillaDisplay::drawPerformanceStats() {
  // Реализация мониторинга производительности через LVGL
  // Можно добавить overlay label для FPS и CPU
}

// ============================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ДЛЯ ЧАСТИЧНОГО ОБНОВЛЕНИЯ
// ============================================

void TenZillaDisplay::invalidateObject(lv_obj_t* obj) {
  if (obj == nullptr || !displayInitialized) return;
  
  // Инвалидируем объект - LVGL обновит только его область при следующем refresh
  // Это значительно быстрее полного обновления экрана
  lv_obj_invalidate(obj);
}

void TenZillaDisplay::invalidateArea(lv_obj_t* obj, int x1, int y1, int x2, int y2) {
  (void)x1; (void)y1; (void)x2; (void)y2; // Параметры координат не используются в упрощенной версии
  if (!displayInitialized || lvglDisplay == nullptr) return;
  
  // В LVGL v8 функция lv_inv_area() недоступна напрямую (приватная)
  // Используем упрощенный подход: инвалидируем активный экран
  // LVGL автоматически оптимизирует обновление только измененных областей
  if (obj != nullptr) {
    // Если передан объект, инвалидируем его
    lv_obj_invalidate(obj);
  } else {
    // Если объект не передан, инвалидируем активный экран
    lv_obj_t* act = lv_scr_act();
    if (act != nullptr) {
      lv_obj_invalidate(act);
    }
  }
  
  // Примечание: Для более точного контроля области можно создать временный объект
  // с нужными координатами и инвалидировать его, но это избыточно для большинства случаев
}
