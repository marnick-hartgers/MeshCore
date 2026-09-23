#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <stdint.h>

// 3-state indicators for the status bar's fixed icon slots (home-dashboard
// phase, phases/home-dashboard/implementation-plan.md Phase 2.2). Shared with
// Screen_HomeDashboard (Phase 3), which reuses UITask's same state helpers
// rather than re-deriving GPS-fix/link state itself.
enum class GpsState { Off, NoFix, Fixed };
enum class LinkState { Off, Disconnected, Connected };

// Fixed-width top strip of status icons, always rendered on top of whatever
// screen is currently showing. Replaces ui-tiny's ported marquee/scrolling
// text bar: with the node name moved to the dashboard's caption line (Phase 3)
// and every other indicator icon-based, the strip never needs variable-width
// text again -- no more scroll state, no e-ink-vs-non-eink text branch, no
// per-frame width math. Every board gets the same static bar.
class StatusBar {
  GpsState _gps;
  LinkState _link;
  bool _buzzer_on;
  int _unread;
  bool _dirty;
  unsigned long _unread_flash_until;   // Phase 5: ambient notification blink

  uint16_t _batt_mv;
  bool _muted;
  bool _batt_dirty;

  void renderBattery(DisplayDriver& display);

public:
  StatusBar();

  void begin();

  void setGpsState(GpsState s);
  void setLinkState(LinkState s);
  void setBuzzer(bool on);
  void setUnreadCount(int count);   // clamped to "9+" past 9

  // Phase 5 (ambient notification blink while connected): call when a new
  // message arrives while hasConnection() is true, instead of the full-screen
  // Screen_MsgPreview takeover. Briefly inverts the unread badge so the cue
  // is visible on whatever screen the user is currently on -- the status bar
  // is drawn on top of every screen -- without waking a sleeping display or
  // stealing focus from it.
  void flashUnread();

  // Caches battery/mute state; marks the bar dirty only if either changed.
  // Signature unchanged from Phase 1 -- muted no longer drives a drawn icon
  // here (setBuzzer() owns that in its own slot now), it's kept only as a
  // dirty-check input.
  void setBattery(uint16_t milliVolts, bool muted);

  bool needsRedraw() const;
  void render(DisplayDriver& display);
};
