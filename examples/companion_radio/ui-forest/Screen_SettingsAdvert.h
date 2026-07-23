#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"
#include "FormField.h"
#include "../NodePrefs.h"

#define UI_SETTINGS_ADVERT_ITEM_COUNT 3

// Advert name + share-location policy (phase-3-settings.md step 4). The
// "advert/flood-advert interval" fields phase-3-settings.md also asks for
// under this screen turn out not to exist for companion_radio at all --
// advert_interval/flood_advert_interval are CommonCLI-only fields
// (src/helpers/CommonCLI.h), used by simple_repeater/simple_room_server/
// simple_sensor. companion_radio's own NodePrefs.h has no equivalent field
// and doesn't use CommonCLI, so there is nothing to write or reschedule --
// this resolves PLAN.md 8's "advert-interval reschedule" open question by
// finding the premise doesn't apply to this app, not by guessing an answer.
//
// "Device name" (phase-3-settings.md step 6, under Screen_SettingsDevice) is
// the same underlying node_name field as "advert name" here -- NodePrefs.h
// has exactly one name field. Implemented once, here, since CMD_SET_ADVERT_NAME
// (whose validation this mirrors) frames it as the advertised name;
// Screen_SettingsDevice does not duplicate a second name editor.
class Screen_SettingsAdvert : public MenuScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  ConfirmScreen& _confirm;
  NodePrefs* _node_prefs;

  MenuItem _rows[UI_SETTINGS_ADVERT_ITEM_COUNT];
  FormField::TextFieldSpec _spec_name;
  FormField::ToggleFieldSpec _spec_share_loc;

  static void getName(void* ctx, char* dest, int max_len);
  static void setName(void* ctx, const char* value);
  static bool getShareLocation(void* ctx);
  static void setShareLocation(void* ctx, bool value);

  static void confirmRestoreDefaults(void* ctx);
  static void doRestoreDefaults(void* ctx);

public:
  Screen_SettingsAdvert(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                        TextField& textField, ToggleField& toggleField);
};
