#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

// Straight port of ui-new's HomePage::RECENT content (item 2, recents portion)
// (examples/companion_radio/ui-new/UITask.cpp:237-261). Reads through
// the_mesh.getRecentlyHeard() on demand, same as ui-new -- never copies the
// whole contact/advert-path table into a new buffer (PLAN.md 3.6). The small
// fixed-size AdvertPath[] window is refreshed on the stack each render() call
// rather than held as a member, so this header doesn't need MyMesh.h's
// AdvertPath definition.
class Screen_Recents : public UIScreen {
  NavStack& _nav;

public:
  Screen_Recents(NavStack& nav) : _nav(nav) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
