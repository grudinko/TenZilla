#ifndef TENZILLA_BREAK_SCREEN_H
#define TENZILLA_BREAK_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaBreakScreen {
public:
  // LVGL методы
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen, float currentWeight, float maxWeight, int opticalCount);
  
  // Старые методы TFT_eSPI (для обратной совместимости)
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
  static float screenMaxWeight;  // Максимум на этом экране (РАЗРЫВ) - сохраняется между переключениями
  
  // Сообщения о статусе и предупреждениях (старый метод, не используется)
  static void drawStatusMessage(void* tft);
  
  // История для графика силы
  static const int GRAPH_HISTORY_SIZE = 100;
  static float forceHistory[GRAPH_HISTORY_SIZE];
  static int historyIndex;
  
  // Состояние двигателя для отслеживания изменений
  static bool lastMotorRunning;
  static int lastMotorDirection;
  
  // Дизайн: ui/TenZillaBreakScreen_ui.*
  
  // Старые методы (для обратной совместимости)
  static void drawGraph(void* tft);
  static void drawMotorIcon(void* tft);
};

#endif













