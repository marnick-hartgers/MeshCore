#include "InputRouter.h"
#include <Arduino.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/ui/MomentaryButton.h>
#include "target.h"

#if defined(UI_HAS_ROTARY_INPUT)
  #include <helpers/ui/RotaryInput.h>
#endif

// Maps one MomentaryButton::check() result onto the single-button gesture
// vocabulary (PLAN.md 3.1): click=NEXT, double=PREV, triple=SELECT,
// long=ENTER-ish. Shared by the digital (PIN_USER_BTN) and analog
// (PIN_USER_BTN_ANA) button paths, which use the exact same vocabulary in
// ui-new today.
static char keyForButtonEvent(int ev) {
  switch (ev) {
    case BUTTON_EVENT_CLICK: return KEY_NEXT;
    case BUTTON_EVENT_DOUBLE_CLICK: return KEY_PREV;
    case BUTTON_EVENT_TRIPLE_CLICK: return KEY_SELECT;
    case BUTTON_EVENT_LONG_PRESS: return KEY_ENTER;
    default: return 0;
  }
}

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

char InputRouter::poll() {
  char c = 0;

#if defined(UI_HAS_JOYSTICK)
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_ENTER;

  ev = joystick_left.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_LEFT;

  ev = joystick_right.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_RIGHT;

  // Some joystick boards (e.g. wio-tracker-l1) wire a real 4-way pad with
  // dedicated up/down pins rather than only left/right -- use them directly
  // when the board's target.h/.cpp declares them, instead of making the user
  // navigate a vertical menu with the left/right axis.
#if defined(JOYSTICK_UP) && defined(JOYSTICK_DOWN)
  ev = joystick_up.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_UP;

  ev = joystick_down.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_DOWN;
#endif

  ev = back_btn.check();
  if (ev == BUTTON_EVENT_CLICK) c = KEY_CANCEL;
#elif defined(PIN_USER_BTN)
  int ev = user_btn.check();
  char k = keyForButtonEvent(ev);
  if (k != 0) c = k;
#endif

#if defined(UI_HAS_ROTARY_INPUT)
  if (c == 0) {
    RotaryInputEvent rotaryEv = rotary_input.poll();
    if (rotaryEv == RotaryInputEvent::Next) c = KEY_NEXT;
    else if (rotaryEv == RotaryInputEvent::Prev) c = KEY_PREV;
  }
#endif

#if defined(PIN_USER_BTN_ANA)
  // throttled like ui-new's analog read, to avoid hammering the ADC every loop()
  if (c == 0 && (long)(millis() - _analog_next_check) >= 0) {
    int ev = analog_btn.check();
    char k = keyForButtonEvent(ev);
    if (k != 0) c = k;
    _analog_next_check = millis() + 10;
  }
#endif

  return c;
}
