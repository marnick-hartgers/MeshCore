#include "MenuScreen.h"
#include "Layout.h"

void MenuScreen::activate(int index) {
  if (index < 0 || index >= _count) return;
  MenuItem& item = _items[index];

  switch (item.kind) {
    case MenuItemKind::Action:
      if (item.action) item.action(item.action_ctx);
      break;
    case MenuItemKind::Submenu:
      if (item.submenu) _nav.push(item.submenu);
      break;
    case MenuItemKind::Toggle:
    case MenuItemKind::Stepper:
    case MenuItemKind::Enum:
    case MenuItemKind::Text:
      // field editors land in Phase 3 -- no-op for now
      break;
    case MenuItemKind::Info:
      // non-interactive row
      break;
  }
}

int MenuScreen::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(_status_bar_shown);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print(_title);
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  int visible = Layout::visibleRows(display, _status_bar_shown);
  if (_selected < _scroll_offset) {
    _scroll_offset = _selected;
  } else if (_selected >= _scroll_offset + visible) {
    _scroll_offset = _selected - visible + 1;
  }

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int idx = _scroll_offset + row;
    if (idx >= _count) break;

    MenuItem& item = _items[idx];
    if (idx == _selected) {
      display.setColor(DisplayDriver::GREEN);
      display.fillRect(0, y, display.width(), row_h);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }

    int x = 0;
    if (item.icon != NULL) {
      display.drawXbm(x, y, item.icon, 16, 16);
      x += 18;
    }
    display.setCursor(x, y + 1);
    display.print(item.label);

    y += row_h;
  }

  return 1000;
}

bool MenuScreen::handleInput(char c) {
  // NEXT/PREV covers single-button and rotary boards; LEFT/RIGHT covers
  // joystick boards repurposing their horizontal axis to move the cursor
  // through this vertical list (PLAN.md 3.1 -- joystick boards keep their
  // native gestures rather than being forced through NEXT/PREV).
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _selected = (_selected + 1) % _count;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _selected = (_selected + _count - 1) % _count;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    activate(_selected);
    return true;
  }
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  return false;
}
