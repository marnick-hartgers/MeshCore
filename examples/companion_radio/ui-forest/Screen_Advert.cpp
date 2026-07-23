#include "Screen_Advert.h"
#include "UITask.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "icons.h"

#if UI_HAS_JOYSTICK
  #define PRESS_LABEL "press Enter"
#else
  #define PRESS_LABEL "long press"
#endif

int Screen_Advert::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setColor(DisplayDriver::GREEN);
  display.drawXbm((display.width() - 32) / 2, top + 4, advert_icon, 32, 32);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), "advert: " PRESS_LABEL);

  return 1000;
}

bool Screen_Advert::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    _task->notify(UIEventType::ack);
    if (the_mesh.advert()) {
      _toast.show("Advert sent!", 1000);
      _task->logEvent("Advert sent");
    } else {
      _toast.show("Advert failed..", 1000);
      _task->logEvent("Advert failed");
    }
    return true;
  }
  return false;
}
