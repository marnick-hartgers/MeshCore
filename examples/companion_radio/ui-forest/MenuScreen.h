#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "ToastOverlay.h"

enum class MenuItemKind : uint8_t {
  Action,    // invoke a callback
  Submenu,   // push another MenuScreen (or any UIScreen) onto the nav stack
  Toggle,    // push a Toggle field editor (Phase 3)
  Stepper,   // push a Stepper field editor (Phase 3)
  Enum,      // push an Enum field editor (Phase 3)
  Text,      // push a Text field editor (Phase 3)
  Info       // non-interactive row, e.g. a stat readout (Phase 4)
};

typedef void (*MenuActionFn)(void* context);

// label + optional icon + kind + payload, per PLAN.md 3.2. Phase 0 only
// exercises Action/Submenu; the rest of the enum exists now so later phases
// don't need to touch this struct's shape.
struct MenuItem {
  const char* label;
  const uint8_t* icon;      // optional 16x16 XBM, NULL if none
  MenuItemKind kind;
  MenuActionFn action;      // used when kind == Action
  void* action_ctx;
  UIScreen* submenu;        // used when kind == Submenu

  // Optional per-row color override (Phase 5, item 30's color convention),
  // used for the row's *unselected* color instead of the default LIGHT --
  // e.g. Screen_SettingsDanger tints its destructive rows RED so the warning
  // reads even before the row is highlighted. has_tint defaults false via
  // this struct's own default member initializers, so every existing
  // MenuItem-building call site (which sets the other fields individually,
  // not via aggregate init) is unaffected without needing to touch any of
  // them -- the implicit default constructor zero-initializes this pair for
  // every array element.
  DisplayDriver::Color tint = DisplayDriver::LIGHT;
  bool has_tint = false;
};

// Generic list-menu widget: the single class that implements Home, Settings,
// Diagnostics, Contacts list, Channels list, and every settings sub-menu
// (PLAN.md 3.2). Handles NEXT/PREV to move the selection cursor with
// scroll-into-view via Layout::visibleRows(), ENTER to activate the selected
// item, CANCEL to nav.pop().
class MenuScreen : public UIScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  const char* _title;
  MenuItem* _items;
  int _count;
  int _selected;
  int _scroll_offset;
  bool _status_bar_shown;
  bool _has_icons;

  void activate(int index);

public:
  // status_bar_shown reserves a Layout::statusBarHeight() strip at the top of
  // the screen (PLAN.md 3.2/3.3) so this menu's header/rows don't collide
  // with StatusBar's always-rendered strip.
  MenuScreen(NavStack& nav, ToastOverlay& toast, const char* title, MenuItem* items, int count, bool status_bar_shown = false)
    : _nav(nav), _toast(toast), _title(title), _items(items), _count(count),
      _selected(0), _scroll_offset(0), _status_bar_shown(status_bar_shown), _has_icons(false) {
    // Cached once (item arrays are static/built-once tables, never mutated
    // after construction -- PLAN.md 3.6) so render() doesn't need to rescan
    // every frame just to pick a row height (Phase 5, see Layout::iconRowHeight()).
    for (int i = 0; i < _count; i++) {
      if (_items[i].icon != NULL) { _has_icons = true; break; }
    }
  }

  // Reset cursor/scroll to the top -- call when (re)entering this screen from
  // a different menu, so it doesn't retain a stale selection.
  void resetCursor() { _selected = 0; _scroll_offset = 0; }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
