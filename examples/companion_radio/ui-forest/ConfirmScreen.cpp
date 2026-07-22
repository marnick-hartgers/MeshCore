#include "ConfirmScreen.h"
#include "Layout.h"
#include <Arduino.h>
#include <stdio.h>

void ConfirmScreen::begin(const char* title, const char* message, ConfirmActionFn on_confirm, void* ctx, int hold_millis) {
  _title = title;
  _message = message;
  _on_confirm = on_confirm;
  _ctx = ctx;
  _armed_until = millis() + hold_millis;
}

int ConfirmScreen::render(DisplayDriver& display) {
  display.setTextSize(1);
  display.setColor(DisplayDriver::YELLOW);
  display.drawTextCentered(display.width() / 2, Layout::headerHeight(), _title);

  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, Layout::headerHeight() + Layout::rowHeight() * 2, _message);

  long remaining = (long)(_armed_until - millis());
  if (remaining > 0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "confirm in %lds", remaining / 1000 + 1);
    display.setColor(DisplayDriver::RED);
    display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), buf);
  }

  return 200;
}

void ConfirmScreen::poll() {
  if (_armed_until != 0 && (long)(millis() - _armed_until) >= 0) {
    _armed_until = 0;
    ConfirmActionFn fn = _on_confirm;
    void* ctx = _ctx;
    _nav.pop();
    if (fn) fn(ctx);
  }
}

bool ConfirmScreen::handleInput(char c) {
  if (c == KEY_CANCEL) {
    _armed_until = 0;
    _nav.pop();
    return true;
  }
  return false;
}
