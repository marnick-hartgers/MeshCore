#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "ConfirmScreen.h"

class UITask;   // avoid circular include with UITask.h, see Screen_Shutdown.cpp

// Straight port of ui-new's HomePage::SHUTDOWN content (item 2, shutdown
// portion) (examples/companion_radio/ui-new/UITask.cpp:403-412, 458-461), but
// using ConfirmScreen's timed arm-then-confirm gate instead of ui-new's
// isButtonPressed()-hold pattern (see PROGRESS.md's Phase 0 "Deviations" --
// this is ConfirmScreen's first real caller).
class Screen_Shutdown : public UIScreen {
  NavStack& _nav;
  ConfirmScreen& _confirm;
  UITask* _task;

public:
  Screen_Shutdown(NavStack& nav, ConfirmScreen& confirm, UITask* task)
    : _nav(nav), _confirm(confirm), _task(task) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
