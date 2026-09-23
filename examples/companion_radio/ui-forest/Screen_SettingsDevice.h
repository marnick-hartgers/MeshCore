#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"
#include "FormField.h"

class UITask;   // avoid circular include with UITask.h, see Screen_SettingsDevice.cpp

// Worst case (all optional screens present): Buzzer, GPS, Sensors,
// Notifications, Shutdown, Restore Defaults. GPS/Sensors rows are
// conditionally built (see constructor), same #if ENV_INCLUDE_GPS==1 /
// #if UI_SENSORS_PAGE==1 gating UITask.h already uses for these two screens.
#define UI_SETTINGS_DEVICE_ITEM_COUNT 6

// Buzzer, notifications entry point, restore defaults (phase-3-settings.md
// step 6 / phase-7-notifications.md step 3). Phase 7 wires the "Notifications"
// row (a Phase 3 stub that toasted a placeholder) to push the real
// Screen_NotificationSettings, built and owned by UITask same as every other
// sub-screen this class points at. "Device name" is intentionally NOT
// duplicated here -- see Screen_SettingsAdvert.h's header comment: node_name
// is the same single field either bullet point could refer to, and it's
// implemented once, under Advert.
//
// "Vibration" (also listed in phase-3-settings.md step 6) is NOT implemented
// this phase: there is no persisted vibration-enable field in NodePrefs
// today (UITask::notify() triggers the vibration motor unconditionally
// whenever PIN_VIBRATION is defined, independent of buzzer_quiet). Adding one
// means appending a field to NodePrefs and to DataStore.cpp's
// loadPrefsInt()/savePrefs() -- a hand-maintained, unversioned fixed-byte-
// offset binary format with no length guard (see DataStore.cpp) -- which is
// real structural surgery on existing users' saved prefs files, not a UI-only
// change, and isn't something to do unverified in an environment with no
// compiler on PATH. Flagged here rather than building a toggle that can't
// actually persist (which would also fail the "survives a reboot" checklist
// item).
class Screen_SettingsDevice : public MenuScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  ConfirmScreen& _confirm;
  UITask* _task;

  MenuItem _rows[UI_SETTINGS_DEVICE_ITEM_COUNT];
  FormField::ToggleFieldSpec _spec_buzzer;

  static bool getBuzzer(void* ctx);
  static void setBuzzer(void* ctx, bool value);

  static void confirmRestoreDefaults(void* ctx);
  static void doRestoreDefaults(void* ctx);

public:
  // gps/sensors/shutdown: home-dashboard phase (implementation-plan.md Phase
  // 4) -- Screen_Gps/Screen_Sensors/Screen_Shutdown relocated here as plain
  // Submenu rows (all three unchanged themselves), now that they're no
  // longer flat Home rows. gps/sensors mirror UITask.h's own #if
  // ENV_INCLUDE_GPS==1 / #if UI_SENSORS_PAGE==1 gating -- callers only pass a
  // real pointer (or omit the argument entirely) when that screen exists in
  // this build.
  Screen_SettingsDevice(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, UITask* task,
                        ToggleField& toggleField, UIScreen* notificationSettings,
#if ENV_INCLUDE_GPS == 1
                        UIScreen* gps,
#endif
#if UI_SENSORS_PAGE == 1
                        UIScreen* sensors,
#endif
                        UIScreen* shutdown);
};
