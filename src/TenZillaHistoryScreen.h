#ifndef TENZILLA_HISTORY_SCREEN_H
#define TENZILLA_HISTORY_SCREEN_H

#include <Arduino.h>
#include "TenZillaLvglShim.h"

class TenZillaHistoryScreen {
public:
  static void createLVGL(lv_obj_t*& screen);
  static void updateLVGL(lv_obj_t* screen);
};

#endif
