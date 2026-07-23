#pragma once

#include <stddef.h>

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
#if defined(HAS_TOUCH)
  // Touch-down/touch-up edge tracking (Phase 6, item 33) -- a tap/swipe is
  // only decidable once the finger lifts, so the start point and most recent
  // point both have to persist across poll() calls while the finger is down.
  bool _touch_active;
  int _touch_start_x, _touch_start_y;
  int _touch_last_x, _touch_last_y;
#endif
#if defined(LILYGO_TDECK)
  // Throttles the keyboard's I2C read the same way PIN_USER_BTN_ANA already
  // throttles its ADC read -- no point hammering the bus every loop().
  unsigned long _kbd_next_check;
#endif

public:
  InputRouter();

  void begin();

  // Returns a KEY_* code (see src/helpers/ui/UIScreen.h), or 0 if nothing
  // happened or the event was fully consumed as a side effect (display wake,
  // CLI rescue, buzzer mute).
  char poll(UITask& task);

  // Phase 6 (item 32): short, this-board's-real-gesture label for a logical
  // action, replacing ui-new's compile-time-only PRESS_LABEL macro (a single
  // #if UI_HAS_JOYSTICK / #else, ported verbatim into three ui-forest leaf
  // screens in Phase 1 -- see Screen_Bluetooth/Advert/Shutdown.cpp before
  // this phase). Static/stateless: which label applies depends only on which
  // board macros are compiled in, not on anything InputRouter tracks at
  // runtime, so every screen can call these directly without holding an
  // InputRouter reference.
  static const char* activateHint();               // gesture for KEY_ENTER/KEY_SELECT
  static const char* moveHint();                     // gesture for KEY_NEXT/KEY_PREV list movement

  // FormField's TextField shows a two-clause hint (how to move the cursor /
  // pick a character, and how to save) that differs enough between boards
  // (keyboard boards get a real second sentence) that it needs its own
  // builder rather than a single short label -- see TextField::render().
  static void textEntryHint(char* buf, size_t size);
};
