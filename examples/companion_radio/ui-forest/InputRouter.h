#pragma once

// Centralizes the per-board gesture-to-key mapping that ui-new duplicates
// inline in UITask::loop() (examples/companion_radio/ui-new/UITask.cpp:713-785).
// Every other class in ui-forest only ever deals in the UIScreen.h KEY_*
// vocabulary; UITask::loop() is the only call site that feeds
// InputRouter::poll()'s return value into NavStack::current()->handleInput(c)
// (PLAN.md 3.1).
class InputRouter {
#if defined(PIN_USER_BTN_ANA)
  unsigned long _analog_next_check;
#endif

public:
  InputRouter();

  void begin();

  // Returns a KEY_* code (see src/helpers/ui/UIScreen.h), or 0 if nothing happened.
  char poll();
};
