#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"

#define UI_DIAGNOSTICS_ROOT_ITEM_COUNT 5

// Root Diagnostics menu (PLAN.md items 22-26, phase-4-diagnostics.md step 2)
// -- purely Submenu rows into the sub-screens built alongside it. Same
// shape as Screen_Settings: a thin MenuScreen subclass, base class does all
// the work, this only builds the row table. Bumped 4->5 in the home-dashboard
// phase for "Recently Heard" (implementation-plan.md Phase 4) -- Screen_Recents
// itself is unchanged, this just gives it a new home now that it's no longer
// a flat Home row.
class Screen_Diagnostics : public MenuScreen {
  MenuItem _rows[UI_DIAGNOSTICS_ROOT_ITEM_COUNT];

public:
  Screen_Diagnostics(NavStack& nav, ToastOverlay& toast,
                      UIScreen* radio, UIScreen* packets, UIScreen* core, UIScreen* event_log,
                      UIScreen* recents);
};
