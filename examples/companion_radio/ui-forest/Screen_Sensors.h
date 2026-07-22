#pragma once

#include <helpers/ui/UIScreen.h>
#include <helpers/sensors/LPPDataHelpers.h>
#include <CayenneLPP.h>
#include "NavStack.h"

#ifndef UI_RECENT_LIST_SIZE
  #define UI_RECENT_LIST_SIZE 4
#endif

class UITask;   // avoid circular include with UITask.h, see Screen_Sensors.cpp

// Straight port of ui-new's HomePage::SENSORS content (item 2, sensors
// portion), #if UI_SENSORS_PAGE == 1 gated same as ui-new
// (examples/companion_radio/ui-new/UITask.cpp:331-402, 451-457). Scrolls
// through whatever CayenneLPP entries the board's SensorManager reports.
class Screen_Sensors : public UIScreen {
  NavStack& _nav;
  UITask* _task;

  CayenneLPP sensors_lpp;
  int sensors_nb = 0;
  bool sensors_scroll = false;
  int sensors_scroll_offset = 0;
  unsigned long next_sensors_refresh = 0;

  void refresh_sensors();

public:
  Screen_Sensors(NavStack& nav, UITask* task) : _nav(nav), _task(task), sensors_lpp(200) { }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
