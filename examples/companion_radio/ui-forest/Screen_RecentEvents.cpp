#include "Screen_RecentEvents.h"
#include "Layout.h"
#include <stdio.h>
#include <Arduino.h>

static void formatAge(char* buf, size_t bufsz, unsigned long age_ms) {
  unsigned long secs = age_ms / 1000;
  if (secs < 60) {
    snprintf(buf, bufsz, "%lus", secs);
  } else if (secs < 3600) {
    snprintf(buf, bufsz, "%lum", secs / 60);
  } else if (secs < 86400) {
    snprintf(buf, bufsz, "%luh", secs / 3600);
  } else {
    snprintf(buf, bufsz, "%lud", secs / 86400);
  }
}

int Screen_RecentEvents::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Recent Events");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  int count = _log.count();
  if (count == 0) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(0, top + Layout::headerHeight() + 2, "No events yet");
    return 1000;
  }

  int visible = Layout::visibleRows(display, true);
  int max_offset = count - visible;
  if (max_offset < 0) max_offset = 0;
  if (_scroll_offset > max_offset) _scroll_offset = max_offset;
  if (_scroll_offset < 0) _scroll_offset = 0;

  Layout::drawCard(display, top + Layout::headerHeight());

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int idx_from_newest = _scroll_offset + row;
    const EventLogEntry* e = _log.getEntry(idx_from_newest);
    if (e == NULL) break;

    char age_buf[8];
    formatAge(age_buf, sizeof(age_buf), millis() - e->timestamp);

    char line[40];
    snprintf(line, sizeof(line), "%s %s", age_buf, e->text);

    display.setColor(DisplayDriver::LIGHT);
    display.drawTextEllipsized(2, y, display.width() - 4, line);

    y += row_h;
  }

  return 1000;
}

bool Screen_RecentEvents::handleInput(char c) {
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _scroll_offset++;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _scroll_offset--;
    return true;
  }
  if (c == KEY_CANCEL || c == KEY_ENTER || c == KEY_SELECT) {
    _nav.pop();
    return true;
  }
  return false;
}
