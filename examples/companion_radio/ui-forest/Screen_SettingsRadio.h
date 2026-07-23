#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"
#include "FormField.h"
#include "EventLog.h"
#include "../NodePrefs.h"

#define UI_SETTINGS_RADIO_ITEM_COUNT 6

// Radio params + TX power (PLAN.md 3.4's flagged easiest-bug-to-introduce
// row, phase-3-settings.md step 3): every field here writes _node_prefs AND
// calls the matching radio_driver.setParams()/setTxPower() live-apply call,
// mirroring MyMesh::handleCmdFrame's CMD_SET_RADIO_PARAMS/CMD_SET_RADIO_TX_POWER
// handlers (MyMesh.cpp) -- never just a prefs write.
//
// A thin MenuScreen subclass, same shape as every other settings sub-menu:
// the base class's render()/handleInput() are reused as-is (a settings
// sub-menu IS exactly generic list-of-labeled-rows behavior), this class only
// builds the MenuItem/FormField-spec tables and the per-field get/set/commit
// callbacks the specs point at.
class Screen_SettingsRadio : public MenuScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  ConfirmScreen& _confirm;
  NodePrefs* _node_prefs;
  EventLog& _event_log;

  MenuItem _rows[UI_SETTINGS_RADIO_ITEM_COUNT];
  FormField::StepperFieldSpec _spec_freq, _spec_sf, _spec_cr, _spec_txpower;
  FormField::EnumFieldSpec _spec_bw;

  void applyRadioParams();       // savePrefs() + radio_driver.setParams(current freq/bw/sf/cr)
  void checkRepeatStillValid();  // auto-disables client_repeat if the new freq can't support it

  static float getFrequency(void* ctx);
  static void  setFrequency(void* ctx, float v);
  static int   getBandwidthIndex(void* ctx);
  static void  setBandwidthIndex(void* ctx, int index);
  static float getSpreadingFactor(void* ctx);
  static void  setSpreadingFactor(void* ctx, float v);
  static float getCodingRate(void* ctx);
  static void  setCodingRate(void* ctx, float v);
  static float getTxPower(void* ctx);
  static void  setTxPower(void* ctx, float v);

  static void confirmRestoreDefaults(void* ctx);
  static void doRestoreDefaults(void* ctx);

public:
  Screen_SettingsRadio(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                        StepperField& stepper, EnumField& enumField, EventLog& event_log);
};
