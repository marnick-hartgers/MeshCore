#include "Screen_SettingsDanger.h"
#include "UITask.h"
#include "../MyMesh.h"
#include "icons.h"

#ifndef DANGER_CONFIRM_MILLIS
  #define DANGER_CONFIRM_MILLIS 5000
#endif

// the_mesh.factoryReset() (MyMesh.cpp) disables serial, formats the
// filesystem, then reboots -- does not return on success.
void Screen_SettingsDanger::doErase(void* ctx) {
  Screen_SettingsDanger* self = (Screen_SettingsDanger*)ctx;
  if (!the_mesh.factoryReset()) {
    self->_toast.show("Erase failed", 1500);
  }
}
void Screen_SettingsDanger::confirmErase(void* ctx) {
  Screen_SettingsDanger* self = (Screen_SettingsDanger*)ctx;
  self->_confirm.begin("Erase ALL data?", "Cannot be undone", doErase, self, DANGER_CONFIRM_MILLIS);
  self->_nav.push(&self->_confirm);
}

// the_mesh.selfRekey() (MyMesh.cpp) generates a fresh random identity and
// reloads contacts from disk to invalidate ECDH secrets computed against the
// old one -- contacts themselves are not wiped, just re-synced.
void Screen_SettingsDanger::doRekey(void* ctx) {
  Screen_SettingsDanger* self = (Screen_SettingsDanger*)ctx;
  if (the_mesh.selfRekey()) {
    self->_toast.show("New identity created", 1500);
  } else {
    self->_toast.show("Rekey failed", 1500);
  }
}
void Screen_SettingsDanger::confirmRekey(void* ctx) {
  Screen_SettingsDanger* self = (Screen_SettingsDanger*)ctx;
  self->_confirm.begin("New identity?", "Contacts will re-sync", doRekey, self, DANGER_CONFIRM_MILLIS);
  self->_nav.push(&self->_confirm);
}

// UITask::shutdown(true) is the existing restart path (buzzer-drain then
// _board->reboot()) -- Screen_Shutdown only ever calls shutdown() with the
// default restart=false (power off), so this is that method's first real
// "restart" caller.
void Screen_SettingsDanger::doReboot(void* ctx) {
  ((Screen_SettingsDanger*)ctx)->_task->shutdown(true);
}
void Screen_SettingsDanger::confirmReboot(void* ctx) {
  Screen_SettingsDanger* self = (Screen_SettingsDanger*)ctx;
  self->_confirm.begin("Reboot now?", "", doReboot, self, 3000);
  self->_nav.push(&self->_confirm);
}

Screen_SettingsDanger::Screen_SettingsDanger(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, UITask* task)
  : MenuScreen(nav, toast, "Danger Zone", _rows, UI_SETTINGS_DANGER_ITEM_COUNT, /*status_bar_shown=*/true),
    _nav(nav), _toast(toast), _confirm(confirm), _task(task) {

  int i = 0;
  _rows[i].label = "Erase All Data"; _rows[i].icon = warning_icon; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmErase; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  _rows[i].tint = DisplayDriver::RED; _rows[i].has_tint = true;
  i++;

  _rows[i].label = "New Identity"; _rows[i].icon = warning_icon; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmRekey; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  _rows[i].tint = DisplayDriver::RED; _rows[i].has_tint = true;
  i++;

  // Reboot is disruptive but not data-destructive like the two above --
  // deliberately no warning_icon here, so the icon stays a meaningful signal
  // (irreversible data loss) rather than decorating every row in this menu.
  _rows[i].label = "Reboot"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmReboot; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;
}
