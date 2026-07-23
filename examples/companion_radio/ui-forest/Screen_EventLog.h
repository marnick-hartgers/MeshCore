#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"
#include "EventLog.h"

// In-RAM ring buffer viewer (PLAN.md item 26, phase-4-diagnostics.md step 7
// -- rescoped from a file-backed log, companion_radio has no such subsystem).
// Takes a reference to the single EventLog instance UITask owns and feeds
// (see UITask::logEvent()); this screen only reads it. Newest entry first,
// scrolled the same Layout-driven way as the other Diagnostics screens.
class Screen_EventLog : public UIScreen {
  NavStack& _nav;
  EventLog& _log;
  int _scroll_offset;

public:
  Screen_EventLog(NavStack& nav, EventLog& log) : _nav(nav), _log(log), _scroll_offset(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
