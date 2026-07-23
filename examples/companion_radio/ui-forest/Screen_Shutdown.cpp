#include "Screen_Shutdown.h"
#include "UITask.h"
#include "Layout.h"
#include "InputRouter.h"
#include "icons.h"
#include <stdio.h>

#ifndef SHUTDOWN_CONFIRM_MILLIS
  #define SHUTDOWN_CONFIRM_MILLIS 3000
#endif

static void shutdownAction(void* ctx) {
  ((UITask*)ctx)->shutdown();
}

int Screen_Shutdown::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  // Phase 6 (item 32): built from InputRouter's per-board gesture vocabulary
  // instead of the ui-new-ported, compile-time-only PRESS_LABEL macro.
  char hint[40];
  snprintf(hint, sizeof(hint), "hibernate: %s", InputRouter::activateHint());
  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  display.drawXbm((display.width() - 32) / 2, top + 4, power_icon, 32, 32);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 1000;
}

bool Screen_Shutdown::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    _confirm.begin("Shutdown?", "hibernating...", shutdownAction, _task, SHUTDOWN_CONFIRM_MILLIS);
    _nav.push(&_confirm);
    return true;
  }
  return false;
}
