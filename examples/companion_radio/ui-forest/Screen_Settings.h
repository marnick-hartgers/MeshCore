#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"

#define UI_SETTINGS_ROOT_ITEM_COUNT 5

// Root Settings menu (phase-3-settings.md step 2) -- purely Submenu rows
// into the five sub-screens built alongside it. Home's Settings entry
// (UITask::begin()) pushes this screen; this class exists as its own thin
// MenuScreen subclass (rather than being built inline in UITask::begin() the
// way Home's own item table is) only to match PLAN.md 4's file layout --
// there's nothing else screen-specific here, the base class does all the
// work, same as every other Screen_SettingsXxx in this phase.
class Screen_Settings : public MenuScreen {
  MenuItem _rows[UI_SETTINGS_ROOT_ITEM_COUNT];

public:
  Screen_Settings(NavStack& nav, ToastOverlay& toast,
                  UIScreen* radio, UIScreen* advert, UIScreen* network, UIScreen* device, UIScreen* danger);
};
