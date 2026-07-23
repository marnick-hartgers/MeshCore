#include "Screen_SettingsDevice.h"
#include "UITask.h"

bool Screen_SettingsDevice::getBuzzer(void* ctx) {
  return !((Screen_SettingsDevice*)ctx)->_task->isBuzzerQuiet();
}
void Screen_SettingsDevice::setBuzzer(void* ctx, bool value) {
  (void)value;  // toggleBuzzer() flips based on current live state (and already
                // handles the ack tone + toast + savePrefs itself, see UITask.cpp);
                // ToggleField only ever calls set() with the opposite of what
                // get() just returned, so "flip" achieves the same result.
  ((Screen_SettingsDevice*)ctx)->_task->toggleBuzzer();
}

void Screen_SettingsDevice::showNotificationsStub(void* ctx) {
  ((Screen_SettingsDevice*)ctx)->_toast.show("Notifications: Phase 7", 1500);
}

void Screen_SettingsDevice::doRestoreDefaults(void* ctx) {
  Screen_SettingsDevice* self = (Screen_SettingsDevice*)ctx;
  if (self->_task->isBuzzerQuiet()) {
    self->_task->toggleBuzzer();  // un-quiet == default; toggleBuzzer() live-applies + saves
  }
  self->_toast.show("Device defaults restored", 1200);
}

void Screen_SettingsDevice::confirmRestoreDefaults(void* ctx) {
  Screen_SettingsDevice* self = (Screen_SettingsDevice*)ctx;
  self->_confirm.begin("Restore device defaults?", "Applies immediately", doRestoreDefaults, self, 3000);
  self->_nav.push(&self->_confirm);
}

Screen_SettingsDevice::Screen_SettingsDevice(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, UITask* task,
                                              ToggleField& toggleField)
  : MenuScreen(nav, toast, "Device", _rows, UI_SETTINGS_DEVICE_ITEM_COUNT, /*status_bar_shown=*/true),
    _nav(nav), _toast(toast), _confirm(confirm), _task(task) {

  _spec_buzzer.nav = &_nav;
  _spec_buzzer.field = &toggleField;
  _spec_buzzer.title = "Buzzer";
  _spec_buzzer.get = getBuzzer;
  _spec_buzzer.set = setBuzzer;
  _spec_buzzer.ctx = this;

  int i = 0;
  _rows[i].label = "Buzzer"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_buzzer; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Notifications"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = showNotificationsStub; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Restore Defaults"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmRestoreDefaults; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;
}
