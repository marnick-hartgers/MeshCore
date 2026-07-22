#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

class UITask;   // avoid circular include with UITask.h, see Screen_Status.cpp

// Straight port of ui-new's HomePage::FIRST content (item 2, status portion):
// message count, connection state, BLE pin
// (examples/companion_radio/ui-new/UITask.cpp:214-236). Battery icon + mute
// overlay moved into StatusBar (item 3) rather than being drawn per-screen.
class Screen_Status : public UIScreen {
  NavStack& _nav;
  UITask* _task;

public:
  Screen_Status(NavStack& nav, UITask* task) : _nav(nav), _task(task) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
