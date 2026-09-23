#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

class UITask;   // avoid circular include with UITask.h, see Screen_HomeDashboard.cpp

// New Home screen (home-dashboard phase, Design 4 "Icon Tile Launcher" --
// phases/home-dashboard/design-4-icon-tiles.md / implementation-plan.md Phase
// 3). Replaces the flat MenuItem[] list UITask::begin() used to build: an
// 8-tile icon grid (GPS, Buzzer, Contacts, Channels, Signal, Diagnostics,
// Settings, Advert) with a shared caption line below it. Deliberately a
// hardcoded 8-tile array, not a generic configurable-tile-count system --
// that's all v1 of this feature needs (the bigger-panel/4x3-grid expansion is
// explicitly deferred, see implementation-plan.md's "Deferred / future work").
class Screen_HomeDashboard : public UIScreen {
  NavStack& _nav;
  UITask* _task;

  // Existing shared screen instances (constructed once in UITask::begin()) --
  // this class only holds references to them, same as MenuScreen-based
  // screens hold pointers to their submenu targets.
  UIScreen* _contacts;
  UIScreen* _channels;
  UIScreen* _diagnostics;
  UIScreen* _settings;
  UIScreen* _advert;

  int _focus;                          // 0..7, row-major
  unsigned long _caption_revert_at;     // 0 = showing the device name

  void renderTile(DisplayDriver& display, int idx, int x, int y, int w, int h);
  const uint8_t* iconFor(int idx);
  void captionFor(int idx, char* buf, size_t size);

public:
  Screen_HomeDashboard(NavStack& nav, UITask* task, UIScreen* contacts,
                        UIScreen* channels, UIScreen* diagnostics,
                        UIScreen* settings, UIScreen* advert)
    : _nav(nav), _task(task), _contacts(contacts), _channels(channels),
      _diagnostics(diagnostics), _settings(settings), _advert(advert),
      _focus(0), _caption_revert_at(0) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
