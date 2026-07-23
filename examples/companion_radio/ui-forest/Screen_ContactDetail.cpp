#include "Screen_ContactDetail.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "target.h"
#include <stdio.h>
#include <string.h>

#define CONTACT_DETAIL_ROW_COUNT 5

static const char* advTypeToStr(uint8_t type) {
  switch (type) {
    case ADV_TYPE_CHAT: return "Chat";
    case ADV_TYPE_REPEATER: return "Repeater";
    case ADV_TYPE_ROOM: return "Room";
    case ADV_TYPE_SENSOR: return "Sensor";
    default: return "Unknown";
  }
}

int Screen_ContactDetail::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  ContactInfo contact;
  bool ok = _idx >= 0 && the_mesh.getContactByIdx(_idx, contact);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);

  if (!ok) {
    display.print("Contact");
    display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(0, top + Layout::headerHeight() + 2, "Not found");
    return 1000;
  }

  char name_buf[32];
  display.translateUTF8ToBlocks(name_buf, contact.name, sizeof(name_buf));
  display.drawTextEllipsized(0, top, display.width(), name_buf);
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  char path_buf[16];
  if (contact.out_path_len == OUT_PATH_UNKNOWN) {
    strcpy(path_buf, "Flood");
  } else {
    sprintf(path_buf, "%d hop%s", contact.out_path_len, contact.out_path_len == 1 ? "" : "s");
  }

  char seen_buf[16];
  if (contact.last_advert_timestamp == 0) {
    strcpy(seen_buf, "never");
  } else {
    uint32_t secs = rtc_clock.getCurrentTime() - contact.last_advert_timestamp;
    if (secs < 60) {
      sprintf(seen_buf, "%lds ago", (long)secs);
    } else if (secs < 60 * 60) {
      sprintf(seen_buf, "%ldm ago", (long)(secs / 60));
    } else {
      sprintf(seen_buf, "%ldh ago", (long)(secs / (60 * 60)));
    }
  }

  char gps_buf[24];
  if (contact.gps_lat == 0 && contact.gps_lon == 0) {
    strcpy(gps_buf, "no gps");
  } else {
    sprintf(gps_buf, "%.4f,%.4f", contact.gps_lat / 1e6, contact.gps_lon / 1e6);
  }

  char id_buf[13];
  mesh::Utils::toHex(id_buf, contact.id.pub_key, 6);

  const char* labels[CONTACT_DETAIL_ROW_COUNT] = { "Type", "Path", "Seen", "GPS", "ID" };
  const char* values[CONTACT_DETAIL_ROW_COUNT] = { advTypeToStr(contact.type), path_buf, seen_buf, gps_buf, id_buf };

  int visible = Layout::visibleRows(display, true);
  int max_offset = CONTACT_DETAIL_ROW_COUNT - visible;
  if (max_offset < 0) max_offset = 0;
  if (_scroll_offset > max_offset) _scroll_offset = max_offset;
  if (_scroll_offset < 0) _scroll_offset = 0;

  Layout::drawCard(display, top + Layout::headerHeight());

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int i = _scroll_offset + row;
    if (i >= CONTACT_DETAIL_ROW_COUNT) break;

    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(2, y, labels[i]);
    display.setColor(DisplayDriver::YELLOW);
    display.drawTextRightAlign(display.width() - 3, y, values[i]);

    y += row_h;
  }

  return 1000;
}

bool Screen_ContactDetail::handleInput(char c) {
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
