#pragma once

#include <helpers/ui/DisplayDriver.h>

#ifndef STATUS_BAR_SCROLL_MS
  #define STATUS_BAR_SCROLL_MS 80
#endif

// Generalizes ui-tiny's ScrollingStatusBar
// (examples/companion_radio/ui-tiny/ScrollingStatusBar.h) into an optional,
// always-rendered top strip shared by every ui-forest screen. Phase 0: static
// placeholder content only, set once via setText() -- real watched fields
// (battery, buzzer, GPS, BLE, transport) get wired in Phase 1/4 (PLAN.md 3.2).
// Change-detection and marquee-scroll-if-too-wide behavior are kept intact
// since later phases need them, they just have nothing dynamic to watch yet.
class StatusBar {
  char _text[160];
  int _text_width;
  int _scroll_x;
  int _display_width;
  unsigned long _next_scroll;
  bool _needs_redraw;

public:
  StatusBar();

  void begin(int display_width);

  // Rebuilds cached width only if the text actually changed.
  void setText(DisplayDriver& display, const char* text);

  bool needsRedraw() const;
  void render(DisplayDriver& display);
};
