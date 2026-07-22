#pragma once

class UITask;   // see InputRouter.cpp -- avoids a circular include with UITask.h

// Centralizes the per-board gesture-to-key mapping that ui-new duplicates
// inline in UITask::loop() (examples/companion_radio/ui-new/UITask.cpp:713-785).
// Every other class in ui-forest only ever deals in the UIScreen.h KEY_*
// vocabulary; UITask::loop() is the only call site that feeds
// InputRouter::poll()'s return value into NavStack::current()->handleInput(c)
// (PLAN.md 3.1).
//
// Phase 1 also folds in the display-wake-on-press, boot-time-CLI-rescue and
// global-mute-on-triple-click side effects ui-new's UITask::loop() applies
// inline around the same raw button events (checkDisplayOn/handleLongPress/
// handleDoubleClick/handleTripleClick) -- they have to live here rather than
// in UITask because only InputRouter still has the raw click/double/triple/
// long-press event type at the point they need to apply; by the time a bare
// KEY_* code comes back out, that distinction is gone (e.g. a joystick click
// and a single-button long-press can both resolve to KEY_ENTER). The actual
// state (display on/off, ui_started_at, buzzer) still lives on UITask; this
// just calls back into it.
class InputRouter {
#if defined(PIN_USER_BTN_ANA)
  unsigned long _analog_next_check;
#endif

public:
  InputRouter();

  void begin();

  // Returns a KEY_* code (see src/helpers/ui/UIScreen.h), or 0 if nothing
  // happened or the event was fully consumed as a side effect (display wake,
  // CLI rescue, buzzer mute).
  char poll(UITask& task);
};
