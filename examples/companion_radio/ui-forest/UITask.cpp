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
    _status_bar.begin(_display->width(), _display->isEink());
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
  _contact_detail = new Screen_ContactDetail(_nav);
  _contacts = new Screen_Contacts(_nav, _contact_detail);
  _channels = new Screen_Channels(_nav);
  // Phase 3: sub-screens constructed before the root Screen_Settings, which
  // needs pointers to all five (same "leaf screens first" ordering _home
  // itself already follows).
  _settings_radio = new Screen_SettingsRadio(_nav, _toast, _confirm, node_prefs, _stepper_field, _enum_field, _event_log);
  _settings_advert = new Screen_SettingsAdvert(_nav, _toast, _confirm, node_prefs, _text_field, _toggle_field);
  _settings_network = new Screen_SettingsNetwork(_nav, _toast, _confirm, node_prefs, _toggle_field, _enum_field, _stepper_field);
  _settings_device = new Screen_SettingsDevice(_nav, _toast, _confirm, this, _toggle_field);
  _settings_danger = new Screen_SettingsDanger(_nav, _toast, _confirm, this);
  _settings = new Screen_Settings(_nav, _toast, _settings_radio, _settings_advert, _settings_network, _settings_device, _settings_danger);
  // Phase 4: sub-screens before the root Screen_Diagnostics, same "leaf
  // screens first" ordering as Settings above.
  _diag_radio = new Screen_DiagRadio(_nav);
  _diag_packets = new Screen_DiagPackets(_nav);
  _diag_core = new Screen_DiagCore(_nav);
  _event_log_screen = new Screen_EventLog(_nav, _event_log);
  _diagnostics = new Screen_Diagnostics(_nav, _toast, _diag_radio, _diag_packets, _diag_core, _event_log_screen);
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

  _home_items[i].label = "Contacts";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _contacts;
  i++;

  _home_items[i].label = "Channels";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _channels;
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

  _home_items[i].label = "Diagnostics";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _diagnostics;
  i++;

  _home_items[i].label = "Settings";
  _home_items[i].icon = NULL;
  _home_items[i].kind = MenuItemKind::Submenu;
  _home_items[i].action = NULL;
  _home_items[i].action_ctx = NULL;
  _home_items[i].submenu = _settings;
  i++;

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

  // Phase 4: this is the one call site MyMesh already routes contact/channel/
  // room messages, acks, and new-contact-message notice through (MyMesh.cpp),
  // so it doubles as the diagnostic event log's feed -- no new Mesh/MyMesh
  // callback plumbing added (phase-4-diagnostics.md step 7's explicit
  // constraint).
  switch (t) {
    case UIEventType::contactMessage:
      logEvent("Message received");
      break;
    case UIEventType::channelMessage:
      logEvent("Channel message received");
      break;
    case UIEventType::roomMessage:
      logEvent("Room message received");
      break;
    case UIEventType::newContactMessage:
      logEvent("New contact message");
      break;
    case UIEventType::ack:
      // Ack is also used for GPS/buzzer-toggle confirmation tones (see
      // toggleGPS()/toggleBuzzer() below), not just message acks -- too noisy
      // and not mesh-diagnostic in nature to log every occurrence.
      break;
    case UIEventType::none:
    default:
      break;
  }
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

// Drops every byte that's part of a multi-byte UTF-8 sequence (emoji, accented
// characters, etc.) instead of substituting a placeholder glyph the way
// DisplayDriver::translateUTF8ToBlocks() does for Contacts/Recents/message
// text -- in the status bar's single scrolling line, a run of block
// characters for a multi-codepoint emoji reads as clutter, so a user-chosen
// node name containing one should just have it removed, not replaced.
static void stripNonAscii(char* dest, const char* src, size_t dest_size) {
  size_t j = 0;
  for (size_t i = 0; src[i] != 0 && j < dest_size - 1; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c < 0x80) dest[j++] = (char)c;   // ASCII byte -- keep
    // else: continuation/lead byte of a multi-byte UTF-8 sequence -- drop it
  }
  dest[j] = 0;
}

void UITask::updateStatusBar() {
  if (_display == NULL) return;

  char name_buf[sizeof(_node_prefs->node_name)];
  stripNonAscii(name_buf, _node_prefs->node_name, sizeof(name_buf));

  char buf[160];
  snprintf(buf, sizeof(buf),
    "%s" STATUS_BAR_SEPARATOR
    "BUZ:%s" STATUS_BAR_SEPARATOR
    "GPS:%s" STATUS_BAR_SEPARATOR
    UI_FOREST_TRANSPORT_NAME ":%s"
    " - ",   // trailing gap before the text loops
    name_buf,
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

  // KEY_HOME (jump-to-root) is handled once here, centrally, rather than by
  // individual screens (PLAN.md 3.1/Phase 2 item 16) -- see handleTripleClick()
  // for where it's emitted.
  if (c == KEY_HOME) {
    _nav.popToRoot();
    c = 0;
    _auto_off = millis() + AUTO_OFF_MILLIS;
    _next_refresh = 100;
  }

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

      // startFrame() always wipes the whole buffer (every DisplayDriver
      // backend does a full clear/fill there, not a partial update), so the
      // status bar must be redrawn on every pass through this block, not just
      // the ones status_due itself triggered -- otherwise a content-only
      // refresh (or a toast-only one) wipes it and leaves it blank for that
      // frame, which reads as a flash/flicker of the status bar (real-device
      // finding, Phase 4). status_due still gates whether this block runs at
      // all; it just isn't the gate for whether the status bar draws once
      // we're already redrawing for some other reason.
      if (!showing_splash) _status_bar.render(*_display);
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
        logEvent("Low battery: shutting down");
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

  // Phase 2: below Home, this gesture (the only "extra" one single-button/
  // analog/rotary boards have spare -- see ARCHITECTURE.md) becomes the
  // long-press-anywhere-equivalent KEY_HOME jump-to-root (PLAN.md 3.1,
  // item 16). At Home itself there's nothing to jump home from, so it keeps
  // ui-new's original always-mute behavior there.
  if (_nav.depth() > 1) {
    return KEY_HOME;
  }
  toggleBuzzer();
  return 0;
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
