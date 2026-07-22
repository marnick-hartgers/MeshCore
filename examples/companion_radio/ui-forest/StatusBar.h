#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <stdint.h>

#ifndef STATUS_BAR_SCROLL_MS
  #define STATUS_BAR_SCROLL_MS 80
#endif

// Generalizes ui-tiny's ScrollingStatusBar
// (examples/companion_radio/ui-tiny/ScrollingStatusBar.h) into an optional,
// always-rendered top strip shared by every ui-forest screen. Phase 1 gives
// it real content: setText() is called every loop() with a freshly-built
// string (name/buzzer/GPS/BLE, mirroring ui-tiny's update()), and
// setBattery() feeds the battery-percentage icon + muted overlay that used to
// be drawn per-screen by ui-new's HomeScreen::renderBatteryIndicator()
// (examples/companion_radio/ui-new/UITask.cpp:112-150) -- now drawn once,
// here, so every screen gets it for free (PLAN.md 3.2 item 3).
class StatusBar {
  char _text[160];
  int _text_width;
  int _scroll_x;
  int _display_width;
  unsigned long _next_scroll;
  bool _needs_redraw;

  uint16_t _batt_mv;
  bool _muted;
  bool _batt_dirty;

  void renderBattery(DisplayDriver& display);

public:
  StatusBar();

  void begin(int display_width);

  // Rebuilds cached width only if the text actually changed.
  void setText(DisplayDriver& display, const char* text);

  // Caches battery/mute state; marks the bar dirty only if either changed.
  void setBattery(uint16_t milliVolts, bool muted);

  bool needsRedraw() const;
  void render(DisplayDriver& display);
};
