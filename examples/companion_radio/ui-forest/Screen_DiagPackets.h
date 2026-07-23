#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Packet stats (PLAN.md item 23, phase-4-diagnostics.md step 4): 7 read-only
// counters -- radio_driver.getPacketsRecv()/getPacketsSent()/
// getPacketsRecvErrors() (already used by ui-new today) plus
// the_mesh.getNumSentFlood()/getNumSentDirect()/getNumRecvFlood()/
// getNumRecvDirect() (public on Dispatcher, src/Dispatcher.h:186-189,
// inherited by MyMesh -- confirmed the direct-path counterparts exist
// alongside the flood ones, resolving PLAN.md 8's open question). This is the
// same 7-field set the companion app's CMD_GET_STATS/STATS_TYPE_PACKETS reply
// sends (MyMesh.cpp), so values shown here should match the phone app's stats
// view exactly. Layout-scrolled label/value list -- 7 rows don't fit a 64px
// OLED's ~4 visible rows at once, same reasoning as Screen_ContactDetail.
class Screen_DiagPackets : public UIScreen {
  NavStack& _nav;
  int _scroll_offset;

public:
  Screen_DiagPackets(NavStack& nav) : _nav(nav), _scroll_offset(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
