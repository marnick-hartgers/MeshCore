#include "Screen_Contacts.h"
#include "Screen_ContactDetail.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "icons.h"

int Screen_Contacts::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);
  int count = the_mesh.getNumContacts();

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Contacts");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  if (count == 0) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(0, top + Layout::headerHeight() + 2, "No contacts");
    return 1000;
  }

  if (_selected >= count) _selected = count - 1;
  if (_selected < 0) _selected = 0;

  // iconRowHeight/visibleIconRows (Phase 5) -- every row here draws a 16x16
  // contact_icon, which doesn't fit in the plain 11px rowHeight() without
  // bleeding into the next row (see Layout.h's comment).
  int visible = Layout::visibleIconRows(display, true);
  if (_selected < _scroll_offset) {
    _scroll_offset = _selected;
  } else if (_selected >= _scroll_offset + visible) {
    _scroll_offset = _selected - visible + 1;
  }

  int row_h = Layout::iconRowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int idx = _scroll_offset + row;
    if (idx >= count) break;

    // Read this one contact on demand -- never cache the whole table.
    ContactInfo contact;
    if (!the_mesh.getContactByIdx(idx, contact)) continue;

    if (idx == _selected) {
      display.setColor(DisplayDriver::GREEN);
      display.fillRect(0, y, display.width(), row_h);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }

    display.drawXbm(0, y + 1, contact_icon, 16, 16);

    char name_buf[32];
    display.translateUTF8ToBlocks(name_buf, contact.name, sizeof(name_buf));
    display.drawTextEllipsized(18, y + 1, display.width() - 18, name_buf);

    y += row_h;
  }

  return 1000;
}

bool Screen_Contacts::handleInput(char c) {
  int count = the_mesh.getNumContacts();

  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    if (count > 0) _selected = (_selected + 1) % count;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    if (count > 0) _selected = (_selected + count - 1) % count;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    if (count > 0 && _detail != NULL) {
      _detail->show(_selected);
      _nav.push(_detail);
    }
    return true;
  }
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  return false;
}
