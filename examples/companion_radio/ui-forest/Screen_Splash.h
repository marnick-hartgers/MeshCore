#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

#ifndef BOOT_SCREEN_MILLIS
  #define BOOT_SCREEN_MILLIS 3000
#endif

// Minimal splash screen, dismisses (on timer or any key) into the placeholder
// home menu. Full parity with ui-new's splash visuals lands in Phase 1; this
// is a bare logo/version screen just to prove NavStack::reset() and the
// render loop end to end (PLAN.md Phase 0).
class Screen_Splash : public UIScreen {
  NavStack& _nav;
  UIScreen* _home;
  unsigned long _dismiss_after;
  char _version_info[12];

public:
  Screen_Splash(NavStack& nav, UIScreen* home);

  int render(DisplayDriver& display) override;
  void poll() override;
  bool handleInput(char c) override;
};
