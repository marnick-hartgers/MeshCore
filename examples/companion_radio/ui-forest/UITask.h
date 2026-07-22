#pragma once

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/SensorManager.h>
#include <helpers/BaseSerialInterface.h>
#include <Arduino.h>

#include "../AbstractUITask.h"
#include "../NodePrefs.h"

#include "NavStack.h"
#include "InputRouter.h"
#include "ToastOverlay.h"
#include "StatusBar.h"
#include "MenuScreen.h"
#include "ConfirmScreen.h"
#include "Screen_Splash.h"

#define UI_FOREST_HOME_ITEM_COUNT 3

// Entry point -- same public contract main.cpp already expects from
// ui-new/ui-tiny (construct with board+serial, begin(display, sensors,
// node_prefs), loop()). Phase 0 only wires the navigation/input/layout
// skeleton: splash -> a placeholder 3-item Home menu, no real feature screens
// yet (PLAN.md Phase 0).
class UITask : public AbstractUITask {
  DisplayDriver* _display;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;

  NavStack _nav;
  InputRouter _input;
  ToastOverlay _toast;
  StatusBar _status_bar;

  Screen_Splash* _splash;
  MenuScreen* _home;

  struct PlaceholderCtx {
    ToastOverlay* toast;
    const char* label;
  };
  PlaceholderCtx _home_item_ctx[UI_FOREST_HOME_ITEM_COUNT];
  MenuItem _home_items[UI_FOREST_HOME_ITEM_COUNT];

  unsigned long _next_refresh;
  int _msgcount;

  static void placeholderAction(void* ctx);

public:
  UITask(mesh::MainBoard* board, BaseSerialInterface* serial)
    : AbstractUITask(board, serial), _display(NULL), _sensors(NULL), _node_prefs(NULL),
      _splash(NULL), _home(NULL), _next_refresh(0), _msgcount(0) { }

  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);

  // from AbstractUITask
  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;
};
