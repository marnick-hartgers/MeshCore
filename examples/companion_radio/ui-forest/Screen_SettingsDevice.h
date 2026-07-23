#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"
#include "FormField.h"

class UITask;   // avoid circular include with UITask.h, see Screen_SettingsDevice.cpp

#define UI_SETTINGS_DEVICE_ITEM_COUNT 3

// Buzzer, notifications entry-point stub, restore defaults
// (phase-3-settings.md step 6). "Device name" is intentionally NOT
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
  static void showNotificationsStub(void* ctx);

  static void confirmRestoreDefaults(void* ctx);
  static void doRestoreDefaults(void* ctx);

public:
  Screen_SettingsDevice(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, UITask* task,
                        ToggleField& toggleField);
};
