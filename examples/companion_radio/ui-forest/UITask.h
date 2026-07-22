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
#include "Screen_Splash.h"
#include "Screen_Status.h"
#include "Screen_Recents.h"
#include "Screen_RadioInfo.h"
#include "Screen_Bluetooth.h"
#include "Screen_Advert.h"
#if ENV_INCLUDE_GPS == 1
  #include "Screen_Gps.h"
#endif
#if UI_SENSORS_PAGE == 1
  #include "Screen_Sensors.h"
#endif
#include "Screen_Shutdown.h"
#include "Screen_MsgPreview.h"

// Number of real top-level Home entries. Phase 1 pushes each screen straight
// off Home as a Submenu-kind MenuItem (proper menu restructuring -- multiple
// nesting levels, Contacts/Channels -- is Phase 2). GPS/Sensors entries are
// conditionally compiled in, so the count/array is sized for the worst case
// and only the first `_home_item_count` slots are used.
#define UI_FOREST_HOME_ITEM_COUNT 8

// Entry point -- same public contract main.cpp already expects from
// ui-new/ui-tiny (construct with board+serial, begin(display, sensors,
// node_prefs), loop()). Phase 1 brings ui-forest to parity with ui-new's
// feature set (PLAN.md Phase 1 / phases/phase-1-parity-with-ui-new.md).
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

  Screen_Splash* _splash;
  MenuScreen* _home;
  Screen_Status* _status;
  Screen_Recents* _recents;
  Screen_RadioInfo* _radio_info;
  Screen_Bluetooth* _bluetooth;
  Screen_Advert* _advert;
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
      _splash(NULL), _home(NULL), _status(NULL), _recents(NULL), _radio_info(NULL),
      _bluetooth(NULL), _advert(NULL),
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
