#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

class UITask;   // avoid circular include with UITask.h, see Screen_Gps.cpp

// Straight port of ui-new's HomePage::GPS content (item 2, gps portion),
// #if ENV_INCLUDE_GPS == 1 gated same as ui-new
// (examples/companion_radio/ui-new/UITask.cpp:292-330, 445-450). None of the
// Phase 1 pilot boards defined ENV_INCLUDE_GPS at the time phase-1.md was
// written, but heltec_rc32's forest env does define it -- see PROGRESS.md.
class Screen_Gps : public UIScreen {
  NavStack& _nav;
  UITask* _task;

public:
  Screen_Gps(NavStack& nav, UITask* task) : _nav(nav), _task(task) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
