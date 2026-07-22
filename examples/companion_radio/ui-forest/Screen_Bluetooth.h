#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

class UITask;   // avoid circular include with UITask.h, see Screen_Bluetooth.cpp

// Straight port of ui-new's HomePage::BLUETOOTH content (item 2, bt portion)
// (examples/companion_radio/ui-new/UITask.cpp:281-287, 428-435): shows the
// bluetooth_on/off icon and toggles serial enable/disable on ENTER.
class Screen_Bluetooth : public UIScreen {
  NavStack& _nav;
  UITask* _task;

public:
  Screen_Bluetooth(NavStack& nav, UITask* task) : _nav(nav), _task(task) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
