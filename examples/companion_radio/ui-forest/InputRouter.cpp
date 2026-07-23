#include "InputRouter.h"
#include "UITask.h"
#include <Arduino.h>
#include <stdio.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/ui/MomentaryButton.h>
#include "target.h"

#if defined(UI_HAS_ROTARY_INPUT)
  #include <helpers/ui/RotaryInput.h>
#endif

// Minimum finger travel (px, in the display's own logical coordinate space)
// before a touch is treated as a swipe instead of a tap (Phase 6, item 33).
#ifndef TOUCH_SWIPE_THRESHOLD
  #define TOUCH_SWIPE_THRESHOLD 12
#endif

InputRouter::InputRouter() {
#if defined(PIN_USER_BTN_ANA)
  _analog_next_check = 0;
#endif
#if defined(HAS_TOUCH)
  _touch_active = false;
  _touch_start_x = _touch_start_y = 0;
  _touch_last_x = _touch_last_y = 0;
#endif
#if defined(LILYGO_TDECK)
  _kbd_next_check = 0;
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

#if defined(HAS_TOUCH)
  // Capacitive touch: tap -> KEY_ENTER, swipe -> KEY_NEXT/KEY_PREV (Phase 6,
  // item 33). This board (sensecap_indicator-espnow) also still has its
  // PIN_USER_BTN=38 fallback button, handled by the #elif defined(PIN_USER_BTN)
  // branch above -- touch is purely additive, so that single-button escape
  // hatch (and KEY_HOME via its triple-click, see UITask::handleTripleClick())
  // keeps working unchanged. Touch state has to be tracked on every poll()
  // regardless of whether a button already produced a key this frame -- a
  // quick tap's touch-down and touch-up can both land between two polls
  // otherwise -- but the resulting key is only kept if nothing else already
  // won this frame (`if (c == 0)` below), same precedence rule
  // rotary/analog/torch already follow.
  {
    int tx, ty;
    bool touching = task.getTouch(&tx, &ty);
    if (touching && !_touch_active) {
      _touch_active = true;
      _touch_start_x = _touch_last_x = tx;
      _touch_start_y = _touch_last_y = ty;
    } else if (touching && _touch_active) {
      _touch_last_x = tx;
      _touch_last_y = ty;
    } else if (!touching && _touch_active) {
      _touch_active = false;
      int dx = _touch_last_x - _touch_start_x;
      int dy = _touch_last_y - _touch_start_y;
      int adx = dx < 0 ? -dx : dx;
      int ady = dy < 0 ? -dy : dy;
      char touch_key;
      if (adx < TOUCH_SWIPE_THRESHOLD && ady < TOUCH_SWIPE_THRESHOLD) {
        touch_key = KEY_ENTER;                          // tap
      } else if (adx > ady) {
        touch_key = (dx > 0) ? KEY_NEXT : KEY_PREV;      // horizontal swipe
      } else {
        touch_key = (dy < 0) ? KEY_NEXT : KEY_PREV;      // vertical swipe (up = next, matching a scroll-the-list-up gesture)
      }
      if (c == 0) c = task.checkDisplayOn(touch_key);
    }
  }
#endif

#if defined(LILYGO_TDECK)
  // Trackball directional movement (Phase 6, item 34) -- additive to the
  // existing PIN_USER_BTN=0 click-only wiring handled by the
  // #elif defined(PIN_USER_BTN) branch above; only consulted if that branch
  // didn't already produce a key this frame, same "extra input source, lowest
  // priority" rule the rotary/analog blocks already follow. trackball_up/
  // down/left/right are declared in target.h/.cpp (see PROGRESS.md's Phase 6
  // section for the pin assignments and their provenance).
  if (c == 0) {
    int ev = trackball_up.check();
    if (ev == BUTTON_EVENT_CLICK) c = task.checkDisplayOn(KEY_UP);
  }
  if (c == 0) {
    int ev = trackball_down.check();
    if (ev == BUTTON_EVENT_CLICK) c = task.checkDisplayOn(KEY_DOWN);
  }
  if (c == 0) {
    int ev = trackball_left.check();
    if (ev == BUTTON_EVENT_CLICK) c = task.checkDisplayOn(KEY_LEFT);
  }
  if (c == 0) {
    int ev = trackball_right.check();
    if (ev == BUTTON_EVENT_CLICK) c = task.checkDisplayOn(KEY_RIGHT);
  }

  // Keyboard (Phase 6, item 34) -- feeds a raw, plain-ASCII byte straight
  // through as the returned "key" code instead of translating it to a KEY_*
  // value. This never collides with the KEY_* vocabulary: printable ASCII is
  // 32-126, KEY_LEFT..KEY_CONTEXT_MENU are 0xB4-0xF3, and the three control
  // codes already in play (10/13/27 = SELECT/ENTER/CANCEL) are reused
  // on purpose -- a keyboard's own Enter/Escape keys naturally send those
  // same byte values, so they already do the right thing with zero extra
  // code. FormField's TextField is the only screen that interprets a raw
  // typed character (see its handleInput()); every other screen's
  // handleInput() ignores unrecognized codes by construction (returns false
  // for anything it doesn't explicitly check), so typing on any non-text
  // screen is a safe no-op, not a bug needing extra guarding here.
  if (c == 0 && (long)(millis() - _kbd_next_check) >= 0) {
    char kc = tdeck_keyboard.poll();
    if (kc != 0) c = task.checkDisplayOn(kc);
    _kbd_next_check = millis() + 20;
  }
#endif

  return c;
}

const char* InputRouter::activateHint() {
#if defined(HAS_TOUCH)
  return "tap";
#elif defined(LILYGO_TDECK)
  return "click";
#elif defined(UI_HAS_JOYSTICK)
  return "press Enter";
#elif defined(UI_HAS_ROTARY_INPUT)
  return "press";
#else
  return "long press";
#endif
}

const char* InputRouter::moveHint() {
#if defined(HAS_TOUCH)
  return "swipe";
#elif defined(LILYGO_TDECK)
  return "trackball";
#elif defined(UI_HAS_JOYSTICK)
  return "stick";
#elif defined(UI_HAS_ROTARY_INPUT)
  return "turn";
#else
  return "click/dbl-click";
#endif
}

void InputRouter::textEntryHint(char* buf, size_t size) {
#if defined(LILYGO_TDECK)
  // Keyboard is a second, additive input path (see phase-6.md) -- the
  // increment-picker gesture (PREV: next char) still works too, so both are
  // shown rather than only advertising the new one.
  snprintf(buf, size, "Type, or PREV: next char. %s: save", activateHint());
#else
  snprintf(buf, size, "PREV: next char, %s: save", activateHint());
#endif
}
