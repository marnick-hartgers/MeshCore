#pragma once

#include <Arduino.h>
#include <helpers/ui/DisplayDriver.h>
#include <string.h>

// Generalizes ui-new's ad hoc _alert/_alert_expiry block (the bordered box
// drawn over whatever curr->render() just drew) into its own small class.
// UITask::loop() calls show() to trigger it and composite() right after
// NavStack::current()->render() to draw it on top, every frame.
class ToastOverlay {
  char _text[80];
  unsigned long _expiry;

public:
  ToastOverlay() : _expiry(0) { _text[0] = 0; }

  void show(const char* text, int duration_millis) {
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = 0;
    _expiry = millis() + duration_millis;
  }

  bool isShowing() const {
    return millis() < _expiry;
  }

  // Returns the millis timestamp the toast expires at (0 if not showing), so
  // the caller can use it to schedule the next render.
  unsigned long expiryMillis() const { return _expiry; }

  void composite(DisplayDriver& display) {
    if (!isShowing()) return;

    display.setTextSize(1);
    int y = display.height() / 3;
    int p = display.height() / 32;
    display.setColor(DisplayDriver::DARK);
    display.fillRect(p, y, display.width() - p * 2, y);
    display.setColor(DisplayDriver::LIGHT);
    display.drawRect(p, y, display.width() - p * 2, y);
    display.drawTextCentered(display.width() / 2, y + p * 3, _text);
  }
};
