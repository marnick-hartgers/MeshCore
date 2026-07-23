#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Radio stats (PLAN.md item 22, phase-4-diagnostics.md step 3): noise floor /
// RSSI / SNR via radio_driver.getNoiseFloor()/getLastRSSI()/getLastSNR() --
// already used by Screen_RadioInfo (Phase 1) and the companion app's
// STATS_TYPE_RADIO reply (MyMesh.cpp), zero new API. Read-only label/value
// list, same Layout-scrolled shape as Screen_ContactDetail (Phase 2) rather
// than a fixed y+=11 layout, so it generalizes the same way across OLED/TFT.
class Screen_DiagRadio : public UIScreen {
  NavStack& _nav;
  int _scroll_offset;

public:
  Screen_DiagRadio(NavStack& nav) : _nav(nav), _scroll_offset(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
