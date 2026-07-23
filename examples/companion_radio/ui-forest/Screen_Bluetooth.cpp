#include "Screen_Bluetooth.h"
#include "UITask.h"
#include "Layout.h"
#include "InputRouter.h"
#include "icons.h"
#include <stdio.h>

int Screen_Bluetooth::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);
  bool enabled = _task->isSerialEnabled();

  // Color convention (Phase 5, item 30): green=connected/good, red=off --
  // this is the one Phase 1 icon screen whose icon bitmap already changes
  // with state (bluetooth_on/off) but whose color didn't follow it.
  // Layout::accentColor() downgrades to plain LIGHT on displays that can't
  // actually show red vs green (monochrome OLED/e-ink).
  display.setColor(Layout::accentColor(display, enabled ? DisplayDriver::GREEN : DisplayDriver::RED));
  display.drawXbm((display.width() - 32) / 2, top + 4,
      enabled ? bluetooth_on : bluetooth_off,
      32, 32);
  // Phase 6 (item 32): built from InputRouter's per-board gesture vocabulary
  // instead of the ui-new-ported, compile-time-only PRESS_LABEL macro.
  char hint[40];
  snprintf(hint, sizeof(hint), "toggle: %s", InputRouter::activateHint());
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 1000;
}

bool Screen_Bluetooth::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    if (_task->isSerialEnabled()) {
      _task->disableSerial();
    } else {
      _task->enableSerial();
    }
    return true;
  }
  return false;
}
