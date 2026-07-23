#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "FormField.h"
#include "NotificationPrefs.h"

// Row count is compile-time, not a runtime "used slots of N" pattern (unlike
// e.g. UITask's Home items) -- PIN_VIBRATION is a board macro fixed at compile
// time, same as ENV_INCLUDE_GPS/UI_SENSORS_PAGE's #if-gating elsewhere in this
// codebase, so the array can just be sized exactly right instead of needing a
// worst-case capacity + a separate "how many are actually used" count. This
// matters here specifically because MenuScreen's own constructor (which this
// class's constructor delegates to) reads the count immediately and uses it
// for every render()/handleInput() bounds check -- an unused trailing MenuItem
// slot would be a zero-initialized row (NULL label) that render() would still
// try to print.
#ifdef PIN_VIBRATION
  #define UI_NOTIF_EVENT_ITEM_COUNT 2
#else
  #define UI_NOTIF_EVENT_ITEM_COUNT 1
#endif

// One reusable per-event-type config sub-screen (phase-7-notifications.md step
// 2), instantiated once per event type by Screen_NotificationSettings --
// Buzzer + (if PIN_VIBRATION) Vibration toggle rows, both operating directly
// on the NotificationTypeConfig struct this instance was built with (in-RAM
// only, see NotificationPrefs.h -- no the_mesh.savePrefs() call, unlike every
// other Toggle field in this codebase, since there's no persisted field to
// write).
class Screen_NotificationEventConfig : public MenuScreen {
  NotificationTypeConfig& _cfg;

  MenuItem _rows[UI_NOTIF_EVENT_ITEM_COUNT];
  FormField::ToggleFieldSpec _spec_buzzer;
#ifdef PIN_VIBRATION
  FormField::ToggleFieldSpec _spec_vibration;
#endif

  static bool getBuzzer(void* ctx);
  static void setBuzzer(void* ctx, bool value);
#ifdef PIN_VIBRATION
  static bool getVibration(void* ctx);
  static void setVibration(void* ctx, bool value);
#endif

public:
  Screen_NotificationEventConfig(NavStack& nav, ToastOverlay& toast, ToggleField& toggleField,
                                  const char* title, NotificationTypeConfig& cfg);
};
