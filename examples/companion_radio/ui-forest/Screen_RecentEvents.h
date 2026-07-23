#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "EventLog.h"

// User-facing notification history (PLAN.md item 35, phase-7-notifications.md
// step 1) -- distinct from Screen_EventLog (Phase 4), which is the nerd/debug
// feed of mesh/radio internals. This is the subset of events that also
// buzz/vibrate: last message, last channel message, last contact seen (via
// the newContactMessage proxy, see UITask::notify()), last advert sent. Reuses
// EventLog, the same ring-buffer class Screen_EventLog reads, but a second,
// separately-fed instance (UITask::_recent_events) -- same shape/render logic
// as Screen_EventLog, kept as its own class/file per PLAN.md 4's file list and
// its own explicit note that these stay separate screens for separate
// audiences even where the underlying buffer class is shared.
class Screen_RecentEvents : public UIScreen {
  NavStack& _nav;
  EventLog& _log;
  int _scroll_offset;

public:
  Screen_RecentEvents(NavStack& nav, EventLog& log) : _nav(nav), _log(log), _scroll_offset(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
