#include "Screen_Shutdown.h"
#include "UITask.h"
#include "Layout.h"
#include "icons.h"

#if UI_HAS_JOYSTICK
  #define PRESS_LABEL "press Enter"
#else
  #define PRESS_LABEL "long press"
#endif

#ifndef SHUTDOWN_CONFIRM_MILLIS
  #define SHUTDOWN_CONFIRM_MILLIS 3000
#endif

static void shutdownAction(void* ctx) {
  ((UITask*)ctx)->shutdown();
}

int Screen_Shutdown::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  display.drawXbm((display.width() - 32) / 2, top + 4, power_icon, 32, 32);
  display.drawTextCentered(display.width() / 2, display.height() - 11, "hibernate: " PRESS_LABEL);

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
