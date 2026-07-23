#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Channels list (PLAN.md Phase 2, item 18). Iterates
// the_mesh.getChannel(idx, ChannelDetails&) over 0..MAX_GROUP_CHANNELS-1,
// skipping empty slots (name[0]==0) -- same pattern as today's recents list
// (PLAN.md 3.4). Read-only browsing only; no per-channel detail/edit screen
// in this phase (that's a stretch goal per PLAN.md 1's non-goals).
class Screen_Channels : public UIScreen {
  NavStack& _nav;
  int _selected;       // index into the filtered (non-empty-only) list
  int _scroll_offset;

public:
  Screen_Channels(NavStack& nav) : _nav(nav), _selected(0), _scroll_offset(0) { }

  void resetCursor() { _selected = 0; _scroll_offset = 0; }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
