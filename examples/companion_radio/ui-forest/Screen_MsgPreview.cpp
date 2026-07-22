#include "Screen_MsgPreview.h"
#include "target.h"
#include "Layout.h"
#include <helpers/TxtDataHelpers.h>
#include <stdio.h>

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS 15000   // 15 seconds
#endif

void Screen_MsgPreview::addPreview(uint8_t path_len, const char* from_name, const char* msg) {
  head = (head + 1) % MAX_UNREAD_MSGS;
  if (num_unread < MAX_UNREAD_MSGS) num_unread++;

  auto p = &unread[head];
  p->timestamp = rtc_clock.getCurrentTime();
  if (path_len == 0xFF) {
    sprintf(p->origin, "(D) %s:", from_name);
  } else {
    sprintf(p->origin, "(%d) %s:", (uint32_t)path_len, from_name);
  }
  StrHelper::strncpy(p->msg, msg, sizeof(p->msg));
}

int Screen_MsgPreview::render(DisplayDriver& display) {
  char tmp[16];
  int top = Layout::statusBarHeight(true);

  display.setCursor(0, top);
  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  sprintf(tmp, "Unread: %d", num_unread);
  display.print(tmp);

  auto p = &unread[head];

  int secs = rtc_clock.getCurrentTime() - p->timestamp;
  if (secs < 60) {
    sprintf(tmp, "%ds", secs);
  } else if (secs < 60 * 60) {
    sprintf(tmp, "%dm", secs / 60);
  } else {
    sprintf(tmp, "%dh", secs / (60 * 60));
  }
  display.setCursor(display.width() - display.getTextWidth(tmp) - 2, top);
  display.print(tmp);

  display.drawRect(0, top + 11, display.width(), 1);  // horiz line

  display.setCursor(0, top + 14);
  display.setColor(DisplayDriver::YELLOW);
  char filtered_origin[sizeof(p->origin)];
  display.translateUTF8ToBlocks(filtered_origin, p->origin, sizeof(filtered_origin));
  display.print(filtered_origin);

  display.setCursor(0, top + 25);
  display.setColor(DisplayDriver::LIGHT);
  char filtered_msg[sizeof(p->msg)];
  display.translateUTF8ToBlocks(filtered_msg, p->msg, sizeof(filtered_msg));
  display.printWordWrap(filtered_msg, display.width());

#if AUTO_OFF_MILLIS == 0   // probably e-ink
  return 10000;
#else
  return 1000;
#endif
}

bool Screen_MsgPreview::handleInput(char c) {
  if (c == KEY_NEXT || c == KEY_RIGHT) {
    head = (head + MAX_UNREAD_MSGS - 1) % MAX_UNREAD_MSGS;
    num_unread--;
    if (num_unread <= 0) {
      num_unread = 0;
      _nav.reset(_home);
    }
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT || c == KEY_CANCEL || c == KEY_PREV) {
    num_unread = 0;  // clear unread queue
    _nav.reset(_home);
    return true;
  }
  return false;
}
