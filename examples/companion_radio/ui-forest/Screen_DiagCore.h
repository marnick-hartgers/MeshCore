#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Core/queue stats + transport indicator (PLAN.md items 24-25,
// phase-4-diagnostics.md step 5): outbound queue length via the new
// MyMesh::getQueueLen() passthrough, uptime via millis()/1000 (same source
// the companion app's STATS_TYPE_CORE reply uses, MyMesh.cpp:
// "_ms->getMillis() / 1000" -- not rtc_clock, so this matches the phone app
// even before the wall clock is set), and which transport this build was
// compiled for (Transport.h). Same Layout-scrolled label/value shape as the
// other Diagnostics screens.
class Screen_DiagCore : public UIScreen {
  NavStack& _nav;
  int _scroll_offset;

public:
  Screen_DiagCore(NavStack& nav) : _nav(nav), _scroll_offset(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
