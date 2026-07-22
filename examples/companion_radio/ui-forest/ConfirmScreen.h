#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

typedef void (*ConfirmActionFn)(void* context);

// Yes/no confirmation gate, reused for shutdown, factory erase, identity
// rekey, reboot, restore-defaults (PLAN.md 3.2). Modeled on ui-new's shutdown
// hold-to-confirm flow (examples/companion_radio/ui-new/UITask.cpp:184-188,
// the _shutdown_init pattern) but generalized as a timed arm-then-confirm
// window instead of raw isButtonPressed() polling, so it works the same way
// across single-button, joystick and rotary boards: once entered, the action
// fires automatically after `hold_millis` unless KEY_CANCEL aborts it first.
// Not exercised by anything in Phase 0 -- first caller is Phase 1's shutdown
// screen / Phase 3's danger-zone settings.
class ConfirmScreen : public UIScreen {
  NavStack& _nav;
  ConfirmActionFn _on_confirm;
  void* _ctx;
  const char* _title;
  const char* _message;
  unsigned long _armed_until;

public:
  ConfirmScreen(NavStack& nav) : _nav(nav), _on_confirm(NULL), _ctx(NULL),
    _title(""), _message(""), _armed_until(0) { }

  // Call right before nav.push(this) to arm the countdown.
  void begin(const char* title, const char* message, ConfirmActionFn on_confirm, void* ctx, int hold_millis);

  int render(DisplayDriver& display) override;
  void poll() override;
  bool handleInput(char c) override;
};
