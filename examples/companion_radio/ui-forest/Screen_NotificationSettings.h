#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "FormField.h"
#include "NotificationPrefs.h"
#include "Screen_NotificationEventConfig.h"

#define UI_NOTIFICATION_SETTINGS_ITEM_COUNT 4

// Root Notifications menu (PLAN.md item 36, phase-7-notifications.md step 2)
// -- one Submenu row per event type, each pushing a Screen_NotificationEventConfig
// instance configured for that event's NotificationTypeConfig. This is what
// Screen_SettingsDevice's "Notifications" stub (Phase 3) now pushes, replacing
// the placeholder toast.
//
// The four Screen_NotificationEventConfig sub-screens are held as plain member
// objects (not UITask-owned pointers the way every other screen in this
// codebase is), constructed once as part of this screen's own construction --
// since Screen_NotificationSettings itself is `new`'d exactly once in
// UITask::begin() and never moved afterward, these four sub-objects get
// stable addresses for the process lifetime, satisfying the same "no
// allocation outside setup" rule as everything else, just one level removed
// from UITask::begin()'s usual "new Screen_X(...)" call sites. Keeps UITask.h
// from needing four more screen pointers for something that's purely internal
// to this one menu.
class Screen_NotificationSettings : public MenuScreen {
  MenuItem _rows[UI_NOTIFICATION_SETTINGS_ITEM_COUNT];

  Screen_NotificationEventConfig _cfg_message;
  Screen_NotificationEventConfig _cfg_channel;
  Screen_NotificationEventConfig _cfg_ack;
  Screen_NotificationEventConfig _cfg_advert;

public:
  Screen_NotificationSettings(NavStack& nav, ToastOverlay& toast, ToggleField& toggleField,
                              NotificationPrefs& prefs);
};
