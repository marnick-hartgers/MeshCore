#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

class Screen_ContactDetail;

// Contacts list (PLAN.md Phase 2, item 17). Driven by the_mesh.getNumContacts()/
// getContactByIdx() -- reads a contact by index only for the rows currently on
// screen (per Layout::visibleRows), never copies the whole contact table into
// a buffer (PLAN.md 3.6, the specific correctness concern called out for this
// screen). Selecting a row pushes Screen_ContactDetail with that index.
class Screen_Contacts : public UIScreen {
  NavStack& _nav;
  Screen_ContactDetail* _detail;
  int _selected;
  int _scroll_offset;

public:
  Screen_Contacts(NavStack& nav, Screen_ContactDetail* detail)
    : _nav(nav), _detail(detail), _selected(0), _scroll_offset(0) { }

  // Call when (re)entering from Home, so a stale cursor position (e.g. from
  // before contacts were added/removed) doesn't carry over.
  void resetCursor() { _selected = 0; _scroll_offset = 0; }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
