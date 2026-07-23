#include "Screen_DiagCore.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "Transport.h"
#include <stdio.h>
#include <Arduino.h>

#define DIAG_CORE_ROW_COUNT 3

static void formatUptime(char* buf, size_t bufsz, uint32_t secs) {
  uint32_t d = secs / 86400;
  uint32_t h = (secs % 86400) / 3600;
  uint32_t m = (secs % 3600) / 60;
  uint32_t s = secs % 60;
  if (d > 0) {
    snprintf(buf, bufsz, "%lud %02lu:%02lu:%02lu", (unsigned long)d, (unsigned long)h, (unsigned long)m, (unsigned long)s);
  } else if (h > 0) {
    snprintf(buf, bufsz, "%lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
  } else {
    snprintf(buf, bufsz, "%lu:%02lu", (unsigned long)m, (unsigned long)s);
  }
}

int Screen_DiagCore::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Core");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  char queue_buf[12], uptime_buf[20];
  sprintf(queue_buf, "%lu", (unsigned long)the_mesh.getQueueLen());
  formatUptime(uptime_buf, sizeof(uptime_buf), millis() / 1000);

  const char* labels[DIAG_CORE_ROW_COUNT] = { "Queue", "Uptime", "Transport" };
  const char* values[DIAG_CORE_ROW_COUNT] = { queue_buf, uptime_buf, UI_FOREST_TRANSPORT_NAME };

  int visible = Layout::visibleRows(display, true);
  int max_offset = DIAG_CORE_ROW_COUNT - visible;
  if (max_offset < 0) max_offset = 0;
  if (_scroll_offset > max_offset) _scroll_offset = max_offset;
  if (_scroll_offset < 0) _scroll_offset = 0;

  Layout::drawCard(display, top + Layout::headerHeight());

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int i = _scroll_offset + row;
    if (i >= DIAG_CORE_ROW_COUNT) break;

    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(2, y, labels[i]);
    display.setColor(DisplayDriver::YELLOW);
    display.drawTextRightAlign(display.width() - 3, y, values[i]);

    y += row_h;
  }

  return 1000;
}

bool Screen_DiagCore::handleInput(char c) {
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
