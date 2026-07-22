#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "ToastOverlay.h"

class UITask;   // avoid circular include with UITask.h, see Screen_Advert.cpp

// Straight port of ui-new's HomePage::ADVERT content (item 2, advert portion)
// (examples/companion_radio/ui-new/UITask.cpp:288-291, 436-444): sends an
// advert on ENTER, toasts the result.
class Screen_Advert : public UIScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  UITask* _task;

public:
  Screen_Advert(NavStack& nav, ToastOverlay& toast, UITask* task) : _nav(nav), _toast(toast), _task(task) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
