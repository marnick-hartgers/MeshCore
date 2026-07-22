#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "../NodePrefs.h"

// Straight port of ui-new's HomePage::RADIO content (item 2, radio portion)
// (examples/companion_radio/ui-new/UITask.cpp:262-280): freq/sf/bw/cr from
// NodePrefs, tx power, and noise floor from radio_driver -- all already
// public, zero new API (PLAN.md 3.4 table row "Radio params / TX power" and
// "Radio stats").
class Screen_RadioInfo : public UIScreen {
  NavStack& _nav;
  NodePrefs* _node_prefs;

public:
  Screen_RadioInfo(NavStack& nav, NodePrefs* node_prefs) : _nav(nav), _node_prefs(node_prefs) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
