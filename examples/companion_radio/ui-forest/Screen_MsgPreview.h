#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

#ifndef MAX_UNREAD_MSGS
  #define MAX_UNREAD_MSGS 32
#endif

// Straight port of ui-new's MsgPreviewScreen (item 4)
// (examples/companion_radio/ui-new/UITask.cpp:466-555). UITask pushes this
// over whatever screen is currently showing whenever a new message arrives
// (matching ui-new's setCurrScreen(msg_preview) -- a flat "replace everything"
// interrupt, via NavStack::reset() rather than push(), so dismissing goes
// straight back to Home root exactly like ui-new's gotoHomeScreen()).
class Screen_MsgPreview : public UIScreen {
  NavStack& _nav;
  UIScreen* _home;

  struct MsgEntry {
    uint32_t timestamp;
    char origin[62];
    char msg[78];
  };
  int num_unread;
  int head;
  MsgEntry unread[MAX_UNREAD_MSGS];

public:
  Screen_MsgPreview(NavStack& nav, UIScreen* home)
    : _nav(nav), _home(home), num_unread(0), head(MAX_UNREAD_MSGS - 1) { }

  void addPreview(uint8_t path_len, const char* from_name, const char* msg);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
