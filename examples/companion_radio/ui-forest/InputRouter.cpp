#include "InputRouter.h"
#include "UITask.h"
#include <Arduino.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/ui/MomentaryButton.h>
#include "target.h"

#if defined(UI_HAS_ROTARY_INPUT)
  #include <helpers/ui/RotaryInput.h>
#endif

InputRouter::InputRouter() {
#if defined(PIN_USER_BTN_ANA)
  _analog_next_check = 0;
#endif
}

void InputRouter::begin() {
#if defined(PIN_USER_BTN)
  user_btn.begin();
#endif
#if defined(PIN_USER_BTN_ANA)
  analog_btn.begin();
#endif
}

char InputRouter::poll(UITask& task) {
  char c = 0;

#if defined(UI_HAS_JOYSTICK)
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_ENTER);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_ENTER);
  }

  ev = joystick_left.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_LEFT);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_LEFT);
  }

  ev = joystick_right.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_RIGHT);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_RIGHT);
  }

  // Some joystick boards (e.g. wio-tracker-l1) wire a real 4-way pad with
  // dedicated up/down pins rather than only left/right -- use them directly
  // when the board's target.h/.cpp declares them, instead of making the user
  // navigate a vertical menu with the left/right axis.
#if defined(JOYSTICK_UP) && defined(JOYSTICK_DOWN)
  ev = joystick_up.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_UP);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_UP);
  }

  ev = joystick_down.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_DOWN);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_DOWN);
  }
#endif

  ev = back_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_CANCEL);
  } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
    c = task.handleTripleClick(KEY_SELECT);
  }
#elif defined(PIN_USER_BTN)
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    c = task.checkDisplayOn(KEY_NEXT);
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
    c = task.handleLongPress(KEY_ENTER);
  } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
    c = task.handleDoubleClick(KEY_PREV);
  } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
    c = task.handleTripleClick(KEY_SELECT);
  }
#endif

#if defined(UI_HAS_ROTARY_INPUT)
  if (c == 0 && task.isDisplayOn()) {
    RotaryInputEvent rotaryEv = rotary_input.poll();
    if (rotaryEv == RotaryInputEvent::Next) c = KEY_NEXT;
    else if (rotaryEv == RotaryInputEvent::Prev) c = KEY_PREV;
  }
#endif

#if defined(PIN_USER_BTN_ANA)
  // throttled like ui-new's analog read, to avoid hammering the ADC every loop()
  if (c == 0 && (long)(millis() - _analog_next_check) >= 0) {
    int ev = analog_btn.check();
    if (ev == BUTTON_EVENT_CLICK) {
      c = task.checkDisplayOn(KEY_NEXT);
    } else if (ev == BUTTON_EVENT_LONG_PRESS) {
      c = task.handleLongPress(KEY_ENTER);
    } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
      c = task.handleDoubleClick(KEY_PREV);
    } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
      c = task.handleTripleClick(KEY_SELECT);
    }
    _analog_next_check = millis() + 10;
  }
#endif

#if defined(HAS_TORCH)
  // Second physical button present alongside PIN_USER_BTN on torch-equipped
  // boards (e.g. lilygo_techo_card) -- not one of the Phase 1 pilots, ported
  // for compile-completeness per phase-1.md item 17, not hardware-verified.
  ev = back_btn.check();
  if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
    board.toggleTorch();
    c = 0;
  } else if (c == 0 && ev == BUTTON_EVENT_CLICK) {
    c = KEY_CANCEL;
  }
#endif

  return c;
}
