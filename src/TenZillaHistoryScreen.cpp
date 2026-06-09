#include "TenZillaHistoryScreen.h"
#include "TenZillaMeasurements.h"
#include "ui/TenZillaHistoryScreen_ui.h"
#include "TenZillaLvglShim.h"

static TenZillaHistoryScreenUI s_ui;

void TenZillaHistoryScreen::createLVGL(lv_obj_t*& screen) {
  if (screen != nullptr) {
    lv_obj_del(screen);
    screen = nullptr;
  }
  TenZillaHistoryScreen_ui_create(&screen, &s_ui);
  TenZillaHistoryScreen_ui_fill_list(&s_ui);
}

void TenZillaHistoryScreen::updateLVGL(lv_obj_t* screen) {
  if (screen == nullptr || s_ui.scrollCont == nullptr) return;
  static int lastCount = -1;
  int n = TenZillaMeasurements::getCount();
  if (n != lastCount) {
    lastCount = n;
    TenZillaHistoryScreen_ui_fill_list(&s_ui);
  }
}
