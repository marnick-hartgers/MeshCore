#include "Screen_Bluetooth.h"
#include "UITask.h"
#include "Layout.h"
#include "icons.h"

#if UI_HAS_JOYSTICK
  #define PRESS_LABEL "press Enter"
#else
  #define PRESS_LABEL "long press"
#endif

int Screen_Bluetooth::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setColor(DisplayDriver::GREEN);
  display.drawXbm((display.width() - 32) / 2, top + 4,
      _task->isSerialEnabled() ? bluetooth_on : bluetooth_off,
      32, 32);
  display.setTextSize(1);
  display.drawTextCentered(display.width() / 2, display.height() - 11, "toggle: " PRESS_LABEL);

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
