#ifndef TENZILLA_MAIN_SCREEN_H
#define TENZILLA_MAIN_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaMainScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen, float currentWeight, float maxWeight, int opticalCount);
  
  // Старые методы TFT_eSPI (для обратной совместимости, можно удалить)
  static void drawStatic(void* tft);
  static void updateData(void* tft, bool forceUpdate, float currentWeight, float maxWeight, int opticalCount, float absoluteDisplacement, float workingDisplacement);
  
  static void resetMaxWeight();  // Сброс максимума экрана
  static float getMaxWeight();   // Получить текущий максимум экрана

private:
  static float lastDisplayedWeight;
  static float lastDisplayedMaxWeight;
  static int lastDisplayedDisplacement;
  static float smoothedWeight;
  static float smoothedMaxWeight;
  static float screenMaxWeight;  // Максимум на этом экране (СЖАТИЕ) - сохраняется между переключениями
  
  // История для графика силы
  static const int GRAPH_HISTORY_SIZE = 100;
  static float forceHistory[GRAPH_HISTORY_SIZE];
  static int historyIndex;
  
  // Состояние двигателя для отслеживания изменений
  static bool lastMotorRunning;
  static int lastMotorDirection;
  
  // Дизайн экрана: ui/TenZillaMainScreen_ui.* (виджеты хранятся внутри)
  
  // Старые методы (для обратной совместимости)
  static void drawGraph(void* tft);
  static void drawMotorIcon(void* tft);
  static void drawStatusMessage(void* tft);
  static void clearStatusMessage();
};

#endif