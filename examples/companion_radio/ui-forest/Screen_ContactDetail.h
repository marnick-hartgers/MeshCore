#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Read-only detail view for a single contact (PLAN.md Phase 2, item 17).
// Pushed by Screen_Contacts via show(idx) + nav.push(this); re-reads the
// contact by index from the_mesh on every render() rather than caching a
// copy, so it never goes stale relative to the underlying contact table
// (same on-demand-by-index discipline as Screen_Contacts itself, PLAN.md 3.6).
class Screen_ContactDetail : public UIScreen {
  NavStack& _nav;
  int _idx;
  int _scroll_offset;

public:
  Screen_ContactDetail(NavStack& nav) : _nav(nav), _idx(-1), _scroll_offset(0) { }

  // Call right before nav.push(this).
  void show(int idx) { _idx = idx; _scroll_offset = 0; }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
