#include "Screen_Recents.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "target.h"
#include <stdio.h>

int Screen_Recents::render(DisplayDriver& display) {
  char tmp[16];
  AdvertPath recent[UI_RECENT_LIST_SIZE];
  the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);

  display.setColor(DisplayDriver::GREEN);
  display.setTextSize(1);
  int y = Layout::statusBarHeight(true) + 2;
  for (int i = 0; i < UI_RECENT_LIST_SIZE; i++, y += Layout::rowHeight()) {
    auto a = &recent[i];
    if (a->name[0] == 0) continue;  // empty slot

    int secs = rtc_clock.getCurrentTime() - a->recv_timestamp;
    if (secs < 60) {
      sprintf(tmp, "%ds", secs);
    } else if (secs < 60 * 60) {
      sprintf(tmp, "%dm", secs / 60);
    } else {
      sprintf(tmp, "%dh", secs / (60 * 60));
    }

    int timestamp_width = display.getTextWidth(tmp);
    int max_name_width = display.width() - timestamp_width - 1;

    char filtered_recent_name[sizeof(a->name)];
    display.translateUTF8ToBlocks(filtered_recent_name, a->name, sizeof(filtered_recent_name));
    display.drawTextEllipsized(0, y, max_name_width, filtered_recent_name);
    display.setCursor(display.width() - timestamp_width - 1, y);
    display.print(tmp);
  }

  return 1000;
}

bool Screen_Recents::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  return false;
}
