#include "Screen_Advert.h"
#include "UITask.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "InputRouter.h"
#include "icons.h"
#include <stdio.h>

int Screen_Advert::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  // Phase 6 (item 32): built from InputRouter's per-board gesture vocabulary
  // instead of the ui-new-ported, compile-time-only PRESS_LABEL macro.
  char hint[40];
  snprintf(hint, sizeof(hint), "advert: %s", InputRouter::activateHint());
  display.setColor(DisplayDriver::GREEN);
  display.drawXbm((display.width() - 32) / 2, top + 4, advert_icon, 32, 32);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 1000;
}

bool Screen_Advert::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    // Phase 7: notify() only fires on a successful send now (UIEventType::
    // advertSent, independently configurable/mutable via
    // Screen_NotificationSettings) -- previously this unconditionally played
    // the generic "ack" confirmation tone before knowing the result, which
    // doesn't match "advert" being its own distinguishable notification type.
    if (the_mesh.advert()) {
      _task->notify(UIEventType::advertSent);
      _toast.show("Advert sent!", 1000);
      _task->logEvent("Advert sent");
      _task->logRecentEvent("Advert sent");
    } else {
      _toast.show("Advert failed..", 1000);
      _task->logEvent("Advert failed");
    }
    return true;
  }
  return false;
}
