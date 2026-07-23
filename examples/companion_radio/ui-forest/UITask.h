#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/SensorManager.h>
#include <helpers/BaseSerialInterface.h>
#include <Arduino.h>

#ifndef LED_STATE_ON
  #define LED_STATE_ON 1
#endif

#ifdef PIN_BUZZER
  #include <helpers/ui/buzzer.h>
#endif
#ifdef PIN_VIBRATION
  #include <helpers/ui/GenericVibration.h>
#endif

#include "../AbstractUITask.h"
#include "../NodePrefs.h"

#include "NavStack.h"
#include "InputRouter.h"
#include "ToastOverlay.h"
#include "StatusBar.h"
#include "MenuScreen.h"
#include "ConfirmScreen.h"
#include "FormField.h"
#include "EventLog.h"
#include "Transport.h"
#include "Screen_Splash.h"
#include "Screen_Status.h"
#include "Screen_Recents.h"
#include "Screen_RadioInfo.h"
#include "Screen_Bluetooth.h"
#include "Screen_Advert.h"
#include "Screen_Contacts.h"
#include "Screen_ContactDetail.h"
#include "Screen_Channels.h"
#include "Screen_Settings.h"
#include "Screen_SettingsRadio.h"
#include "Screen_SettingsAdvert.h"
#include "Screen_SettingsNetwork.h"
#include "Screen_SettingsDevice.h"
#include "Screen_SettingsDanger.h"
#include "Screen_Diagnostics.h"
#include "Screen_DiagRadio.h"
#include "Screen_DiagPackets.h"
#include "Screen_DiagCore.h"
#include "Screen_EventLog.h"
#if ENV_INCLUDE_GPS == 1
  #include "Screen_Gps.h"
#endif
#if UI_SENSORS_PAGE == 1
  #include "Screen_Sensors.h"
#endif
#include "Screen_Shutdown.h"
#include "Screen_MsgPreview.h"

// Number of real top-level Home entries. GPS/Sensors entries are
// conditionally compiled in, so the count/array is sized for the worst case
// (all optional screens present) and only the first `_home_item_count` slots
// are used. Bumped from 10 to 11 in Phase 3 for "Settings"; bumped to 12 in
// Phase 4 for "Diagnostics" -- phase-4-diagnostics.md's own stated
// prerequisite (that Phase 3 already added this slot as a placeholder) didn't
// hold (see PROGRESS.md/ARCHITECTURE.md), so this phase adds the Home entry
// and its real content in one step instead of two.
#define UI_FOREST_HOME_ITEM_COUNT 12

// Entry point -- same public contract main.cpp already expects from
// ui-new/ui-tiny (construct with board+serial, begin(display, sensors,
// node_prefs), loop()). Phase 1 brought ui-forest to parity with ui-new's
// feature set; Phase 2 restructures Home into a real top-level menu and adds
// Contacts/Channels browsing (PLAN.md Phase 2 /
// phases/phase-2-navigation-and-data-browsing.md).
class UITask : public AbstractUITask {
  DisplayDriver* _display;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;

#ifdef PIN_BUZZER
  genericBuzzer buzzer;
#endif
#ifdef PIN_VIBRATION
  GenericVibration vibration;
#endif

  NavStack _nav;
  InputRouter _input;
  ToastOverlay _toast;
  StatusBar _status_bar;
  ConfirmScreen _confirm;

  // Phase 3: the four shared field-editor instances every settings screen's
  // Toggle/Stepper/Enum/Text rows push (see FormField.h) -- one of each,
  // reused across every settings screen, same "single shared instance,
  // begin() reconfigures it" pattern _confirm already established.
  ToggleField _toggle_field;
  StepperField _stepper_field;
  EnumField _enum_field;
  TextField _text_field;

  // Phase 4: fed by notify()/logEvent() at the points UITask/Screen_* already
  // observe something worth logging (see EventLog.h). Shared with
  // Screen_EventLog by reference, same "UITask owns it, screens read/write
  // through a reference" shape as _toast/_status_bar.
  EventLog _event_log;

  Screen_Splash* _splash;
  MenuScreen* _home;
  Screen_Status* _status;
  Screen_Recents* _recents;
  Screen_RadioInfo* _radio_info;
  Screen_Bluetooth* _bluetooth;
  Screen_Advert* _advert;
  Screen_ContactDetail* _contact_detail;
  Screen_Contacts* _contacts;
  Screen_Channels* _channels;
  Screen_SettingsRadio* _settings_radio;
  Screen_SettingsAdvert* _settings_advert;
  Screen_SettingsNetwork* _settings_network;
  Screen_SettingsDevice* _settings_device;
  Screen_SettingsDanger* _settings_danger;
  Screen_Settings* _settings;
  Screen_DiagRadio* _diag_radio;
  Screen_DiagPackets* _diag_packets;
  Screen_DiagCore* _diag_core;
  Screen_EventLog* _event_log_screen;
  Screen_Diagnostics* _diagnostics;
#if ENV_INCLUDE_GPS == 1
  Screen_Gps* _gps;
#endif
#if UI_SENSORS_PAGE == 1
  Screen_Sensors* _sensors_screen;
#endif
  Screen_Shutdown* _shutdown_screen;
  Screen_MsgPreview* _msg_preview;

  MenuItem _home_items[UI_FOREST_HOME_ITEM_COUNT];
  int _home_item_count;

  unsigned long _next_refresh, _auto_off;
  unsigned long ui_started_at, next_batt_chck;
#ifdef PIN_STATUS_LED
  int led_state = 0;
  int next_led_change = 0;
  int last_led_increment = 0;
#endif
  int _msgcount;

  void userLedHandler();
  void updateStatusBar();

public:
  UITask(mesh::MainBoard* board, BaseSerialInterface* serial)
    : AbstractUITask(board, serial), _display(NULL), _sensors(NULL), _node_prefs(NULL),
      _confirm(_nav),   // declared after _nav, so this is safe to init here (see .h field order)
      _toggle_field(_nav), _stepper_field(_nav), _enum_field(_nav), _text_field(_nav),
      _splash(NULL), _home(NULL), _status(NULL), _recents(NULL), _radio_info(NULL),
      _bluetooth(NULL), _advert(NULL),
      _contact_detail(NULL), _contacts(NULL), _channels(NULL),
      _settings_radio(NULL), _settings_advert(NULL), _settings_network(NULL),
      _settings_device(NULL), _settings_danger(NULL), _settings(NULL),
      _diag_radio(NULL), _diag_packets(NULL), _diag_core(NULL),
      _event_log_screen(NULL), _diagnostics(NULL),
#if ENV_INCLUDE_GPS == 1
      _gps(NULL),
#endif
#if UI_SENSORS_PAGE == 1
      _sensors_screen(NULL),
#endif
      _shutdown_screen(NULL), _msg_preview(NULL), _home_item_count(0),
      _next_refresh(0), _auto_off(0), ui_started_at(0), next_batt_chck(0), _msgcount(0) { }

  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);

  void gotoHomeScreen() { _nav.reset(_home); }
  int  getMsgCount() const { return _msgcount; }
  bool hasDisplay() const { return _display != NULL; }
  bool isDisplayOn() const { return _display != NULL && _display->isOn(); }

  bool isBuzzerQuiet() {
#ifdef PIN_BUZZER
    return buzzer.isQuiet();
#else
    return true;
#endif
  }

  // Phase 4: the one place any screen/UITask code appends to the diagnostic
  // event ring buffer (EventLog.h) -- Screen_Advert/Screen_SettingsRadio call
  // this via their existing UITask*/reference, same shape as toggleBuzzer()
  // etc. Screen_EventLog is constructed with a direct reference to
  // `_event_log` (see begin()), so it reads the buffer without going through
  // UITask at all.
  void logEvent(const char* text) { _event_log.push(text); }

  void toggleBuzzer();
  bool getGPSState();
  void toggleGPS();

  // Called from InputRouter -- these mirror ui-new's UITask methods of the
  // same name (examples/companion_radio/ui-new/UITask.cpp:862-894), just
  // invoked from InputRouter::poll() instead of inline in loop(), since
  // InputRouter is where the per-board gesture branching now lives.
  char checkDisplayOn(char c);
  char handleLongPress(char c);
  char handleDoubleClick(char c);
  char handleTripleClick(char c);

  // from AbstractUITask
  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;

  void shutdown(bool restart = false);
};
