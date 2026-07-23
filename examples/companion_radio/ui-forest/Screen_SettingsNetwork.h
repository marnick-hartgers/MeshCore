#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"
#include "FormField.h"
#include "../NodePrefs.h"

#define UI_SETTINGS_NETWORK_ITEM_COUNT 10

// Repeat, RX boost, telemetry (base/location/environment), auto-add policy,
// duty cycle + RX delay factors (phase-3-settings.md step 5). Per-contact-
// type auto-add allow bits (AUTO_ADD_CHAT/REPEATER/ROOM_SERVER/SENSOR) and
// "overwrite oldest when full" are deliberately NOT exposed here -- those
// bit constants are #defined locally inside MyMesh.cpp (private implementation
// detail, not a header), so surfacing them would mean either duplicating
// private constants here or adding new MyMesh API neither phase-3-settings.md
// nor PLAN.md's data table calls for. "Auto-add contacts" (on/off) + "Auto-add
// max hops" cover the primary policy; the finer-grained per-type bits are left
// for a future phase if wanted.
class Screen_SettingsNetwork : public MenuScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  ConfirmScreen& _confirm;
  NodePrefs* _node_prefs;

  MenuItem _rows[UI_SETTINGS_NETWORK_ITEM_COUNT];
  FormField::ToggleFieldSpec _spec_repeat, _spec_rx_boost, _spec_autoadd;
  FormField::EnumFieldSpec _spec_telem_base, _spec_telem_loc, _spec_telem_env;
  FormField::StepperFieldSpec _spec_autoadd_hops, _spec_duty_cycle, _spec_rx_delay;

  static bool getRepeat(void* ctx);
  static void setRepeat(void* ctx, bool value);
  static bool getRxBoost(void* ctx);
  static void setRxBoost(void* ctx, bool value);
  static int  getTelemetryBase(void* ctx);
  static void setTelemetryBase(void* ctx, int index);
  static int  getTelemetryLoc(void* ctx);
  static void setTelemetryLoc(void* ctx, int index);
  static int  getTelemetryEnv(void* ctx);
  static void setTelemetryEnv(void* ctx, int index);
  static bool getAutoAdd(void* ctx);
  static void setAutoAdd(void* ctx, bool value);
  static float getAutoAddMaxHops(void* ctx);
  static void  setAutoAddMaxHops(void* ctx, float v);
  static float getDutyCycle(void* ctx);
  static void  setDutyCycle(void* ctx, float v);
  static float getRxDelay(void* ctx);
  static void  setRxDelay(void* ctx, float v);

  static void confirmRestoreDefaults(void* ctx);
  static void doRestoreDefaults(void* ctx);

public:
  Screen_SettingsNetwork(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                         ToggleField& toggleField, EnumField& enumField, StepperField& stepperField);
};
