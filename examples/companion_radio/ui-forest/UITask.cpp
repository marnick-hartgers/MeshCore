#include "UITask.h"
#include <helpers/TxtDataHelpers.h>
#include "../MyMesh.h"
#include "target.h"
#include <stdio.h>
#include <string.h>

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS     15000   // 15 seconds
#endif

#ifdef PIN_STATUS_LED
#define LED_ON_MILLIS     20
#define LED_ON_MSG_MILLIS 200
#define LED_CYCLE_MILLIS  4000
#endif

#ifndef STATUS_BAR_SEPARATOR
  #define STATUS_BAR_SEPARATOR " | "
#endif

void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
  _display = display;
  _sensors = sensors;
  _node_prefs = node_prefs;
  _auto_off = millis() + AUTO_OFF_MILLIS;

  _input.begin();

#ifdef PIN_BUZZER
  buzzer.begin();
  buzzer.quiet(_node_prefs->buzzer_quiet);
  buzzer.startup();
#endif

#ifdef PIN_VIBRATION
  vibration.begin();
#endif

  if (_display != NULL) {
    _display->turnOn();
    _status_bar.begin(_display->width());
  }

  ui_started_at = millis();

  // Constructed once here, matching the "no allocation outside setup" rule
  // ui-new already follows (PLAN.md 3.2/3.6) -- NavStack only ever moves
  // these pointers around afterward. Leaf screens first, since Home's item
  // table (built next) needs pointers to all of them.
  _status = new Screen_Status(_nav, this);
  _recents = new Screen_Recents(_nav);
  _radio_info = new Screen_RadioInfo(_nav, node_prefs);
  _bluetooth = new Screen_Bluetooth(_nav, this);
  _advert = new Screen_Advert(_nav, _toast, this);
#if ENV_INCLUDE_GPS == 1
  _gps = new Screen_Gps(_nav, this);
#endif
#if UI_SENSORS_PAGE == 1
  _sensors_screen = new Screen_Sensors(_nav, this);
#endif
  _shutdown_screen = new Screen_Shutdown(_nav, _confirm, this);

  int i = 0;
  _home_items[i].label = "Status";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _status;
  i++;

  _home_items[i].label = "Recent";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _recents;
  i++;

  _home_items[i].label = "Radio";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _radio_info;
  i++;

  _home_items[i].label = "Bluetooth";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _bluetooth;
  i++;

  _home_items[i].label = "Advert";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _advert;
  i++;

#if ENV_INCLUDE_GPS == 1
  _home_items[i].label = "GPS";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _gps;
  i++;
#endif

#if UI_SENSORS_PAGE == 1
  _home_items[i].label = "Sensors";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _sensors_screen;
  i++;
#endif

  _home_items[i].label = "Shutdown";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _shutdown_screen;
  i++;

  _home_item_count = i;

  _home = new MenuScreen(_nav, _toast, "Home", _home_items, _home_item_count, /*status_bar_shown=*/true);
  _msg_preview = new Screen_MsgPreview(_nav, _home);
  _splash = new Screen_Splash(_nav, _home);
  _nav.reset(_splash);

  _next_refresh = 0;
}

void UITask::msgRead(int msgcount) {
  _msgcount = msgcount;
  if (msgcount == 0) {
    gotoHomeScreen();
  }
}

void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msgcount = msgcount;

  _msg_preview->addPreview(path_len, from_name, text);
  _nav.reset(_msg_preview);

  if (_display != NULL) {
    if (!_display->isOn() && !hasConnection()) {
      _display->turnOn();
    }
    if (_display->isOn()) {
      _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
      _next_refresh = 100;  // trigger refresh
    }
  }
}

void UITask::notify(UIEventType t) {
#if defined(PIN_BUZZER)
  switch (t) {
    case UIEventType::contactMessage:
      buzzer.play("MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7");
      break;
    case UIEventType::channelMessage:
      buzzer.play("kerplop:d=16,o=6,b=120:32g#,32c#");
      break;
    case UIEventType::ack:
      buzzer.play("ack:d=32,o=8,b=120:c");
      break;
    case UIEventType::roomMessage:
    case UIEventType::newContactMessage:
    case UIEventType::none:
    default:
      break;
  }
#endif

#ifdef PIN_VIBRATION
  if (t != UIEventType::none) {
    vibration.trigger();
  }
#endif
}

void UITask::userLedHandler() {
#ifdef PIN_STATUS_LED
  int cur_time = millis();
  if (cur_time > next_led_change) {
    if (led_state == 0) {
      led_state = 1;
      if (_msgcount > 0) {
        last_led_increment = LED_ON_MSG_MILLIS;
      } else {
        last_led_increment = LED_ON_MILLIS;
      }
      next_led_change = cur_time + last_led_increment;
    } else {
      led_state = 0;
      next_led_change = cur_time + LED_CYCLE_MILLIS - last_led_increment;
    }
    digitalWrite(PIN_STATUS_LED, led_state == LED_STATE_ON);
  }
#endif
}

void UITask::updateStatusBar() {
  if (_display == NULL) return;

  char buf[160];
  snprintf(buf, sizeof(buf),
    "%s" STATUS_BAR_SEPARATOR
    "BUZ:%s" STATUS_BAR_SEPARATOR
    "GPS:%s" STATUS_BAR_SEPARATOR
    "BLE:%s"
    " - ",   // trailing gap before the text loops
    _node_prefs->node_name,
    isBuzzerQuiet() ? "OFF" : "ON",
    getGPSState() ? "ON" : "OFF",
    isSerialEnabled() ? "ON" : "OFF"
  );
  _status_bar.setText(*_display, buf);
  _status_bar.setBattery(getBattMilliVolts(), isBuzzerQuiet());
}

/*
  hardware-agnostic pre-shutdown activity should be done here
*/
void UITask::shutdown(bool restart) {
#ifdef PIN_BUZZER
  buzzer.shutdown();
  uint32_t buzzer_timer = millis(); // fail-safe shutdown
  while (buzzer.isPlaying() && (millis() - 2500) < buzzer_timer)
    buzzer.loop();
#endif

  if (restart) {
    _board->reboot();
  } else {
    _board->powerOff();
  }
}

void UITask::loop() {
  char c = _input.poll(*this);

  if (c != 0 && _nav.current()) {
    _nav.current()->handleInput(c);
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
    _next_refresh = 100;  // trigger refresh
  }

  userLedHandler();

#ifdef PIN_BUZZER
  if (buzzer.isPlaying()) buzzer.loop();
#endif

  if (_nav.current()) _nav.current()->poll();

  if (_display != NULL && _display->isOn()) {
    // Splash draws its logo across the same top rows the status bar strip
    // occupies -- suppress the strip for that one screen rather than
    // overlapping the logo with scrolling name/buzzer/GPS/BLE text.
    bool showing_splash = (_nav.current() == _splash);
    if (!showing_splash) updateStatusBar();

    bool content_due = millis() >= _next_refresh && _nav.current();
    bool status_due = !showing_splash && _status_bar.needsRedraw();
    if (content_due || status_due || _toast.isShowing()) {
      _display->startFrame();

      UIScreen* curr = _nav.current();
      int delay_millis = 1000;
      if (curr) delay_millis = curr->render(*_display);

      if (status_due) _status_bar.render(*_display);
      _toast.composite(*_display);

      _display->endFrame();
      if (content_due) _next_refresh = millis() + delay_millis;
    }

#if AUTO_OFF_MILLIS > 0
#ifdef KEEP_DISPLAY_ON_USB
    // Opt-in: refresh the auto-off deadline while externally powered, so the
    // timer counts from the moment external power is removed. Off by default
    // because OLED panels burn in quickly; only enable for LCD targets or
    // where the display is replaceable.
    if (_board->isExternalPowered()) {
      _auto_off = millis() + AUTO_OFF_MILLIS;
    }
#endif
    if (millis() > _auto_off) {
      _display->turnOff();
    }
#endif
  }

#ifdef PIN_VIBRATION
  vibration.loop();
#endif

#ifdef AUTO_SHUTDOWN_MILLIVOLTS
  if (millis() > next_batt_chck) {
    uint16_t milliVolts = getBattMilliVolts();
    if (milliVolts > 0 && milliVolts < AUTO_SHUTDOWN_MILLIVOLTS) {
      if (!_board->isExternalPowered()) {
        if (_display != NULL) {
          _display->startFrame();
          _display->setTextSize(2);
          _display->setColor(DisplayDriver::RED);
          _display->drawTextCentered(_display->width() / 2, 20, "Low Battery.");
          _display->drawTextCentered(_display->width() / 2, 40, "Shutting Down!");
          _display->endFrame();
          if (_display->isEink() == false) { delay(3000); }
        }
        shutdown();
      }
    }
    next_batt_chck = millis() + 8000;
  }
#endif
}

char UITask::checkDisplayOn(char c) {
  if (_display != NULL) {
    if (!_display->isOn()) {
      _display->turnOn();   // turn display on and consume event
      c = 0;
    }
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
    _next_refresh = 0;  // trigger refresh
  }
  return c;
}

char UITask::handleLongPress(char c) {
  if (millis() - ui_started_at < 8000) {   // long press in first 8 seconds since startup -> CLI/rescue
    the_mesh.enterCLIRescue();
    c = 0;   // consume event
  }
  return c;
}

char UITask::handleDoubleClick(char c) {
  MESH_DEBUG_PRINTLN("UITask: double-click triggered");
  checkDisplayOn(c);
  return c;
}

char UITask::handleTripleClick(char c) {
  MESH_DEBUG_PRINTLN("UITask: triple click triggered");
  checkDisplayOn(c);
  toggleBuzzer();
  c = 0;
  return c;
}

bool UITask::getGPSState() {
  if (_sensors != NULL) {
    int num = _sensors->getNumSettings();
    for (int i = 0; i < num; i++) {
      if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
        return !strcmp(_sensors->getSettingValue(i), "1");
      }
    }
  }
  return false;
}

void UITask::toggleGPS() {
  if (_sensors != NULL) {
    int num = _sensors->getNumSettings();
    for (int i = 0; i < num; i++) {
      if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
        if (strcmp(_sensors->getSettingValue(i), "1") == 0) {
          _sensors->setSettingValue("gps", "0");
          _node_prefs->gps_enabled = 0;
          notify(UIEventType::ack);
        } else {
          _sensors->setSettingValue("gps", "1");
          _node_prefs->gps_enabled = 1;
          notify(UIEventType::ack);
        }
        the_mesh.savePrefs();
        _toast.show(_node_prefs->gps_enabled ? "GPS: Enabled" : "GPS: Disabled", 800);
        _next_refresh = 0;
        break;
      }
    }
  }
}

void UITask::toggleBuzzer() {
#ifdef PIN_BUZZER
  if (buzzer.isQuiet()) {
    buzzer.quiet(false);
    notify(UIEventType::ack);
  } else {
    buzzer.quiet(true);
  }
  _node_prefs->buzzer_quiet = buzzer.isQuiet();
  the_mesh.savePrefs();
  _toast.show(buzzer.isQuiet() ? "Buzzer: OFF" : "Buzzer: ON", 800);
  _next_refresh = 0;  // trigger refresh
#endif
}
