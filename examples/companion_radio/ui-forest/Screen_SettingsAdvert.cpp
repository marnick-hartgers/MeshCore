#include "Screen_SettingsAdvert.h"
#include "../MyMesh.h"
#include <string.h>

void Screen_SettingsAdvert::getName(void* ctx, char* dest, int max_len) {
  NodePrefs* p = ((Screen_SettingsAdvert*)ctx)->_node_prefs;
  strncpy(dest, p->node_name, max_len);
  dest[max_len] = 0;
}

// Mirrors CMD_SET_ADVERT_NAME's handler (MyMesh.cpp): the only validation
// rule it applies is truncating to sizeof(node_name)-1 and null-terminating
// -- no character allowlist. TextField's own max_len already keeps the
// working buffer within this limit, so the truncation below is defensive
// (matches the real handler's care) rather than load-bearing here.
void Screen_SettingsAdvert::setName(void* ctx, const char* value) {
  Screen_SettingsAdvert* self = (Screen_SettingsAdvert*)ctx;
  NodePrefs* p = self->_node_prefs;
  int n = strlen(value);
  if (n > (int)sizeof(p->node_name) - 1) n = sizeof(p->node_name) - 1;
  memcpy(p->node_name, value, n);
  p->node_name[n] = 0;
  the_mesh.savePrefs();
}

bool Screen_SettingsAdvert::getShareLocation(void* ctx) {
  return ((Screen_SettingsAdvert*)ctx)->_node_prefs->advert_loc_policy != ADVERT_LOC_NONE;
}
void Screen_SettingsAdvert::setShareLocation(void* ctx, bool value) {
  Screen_SettingsAdvert* self = (Screen_SettingsAdvert*)ctx;
  self->_node_prefs->advert_loc_policy = value ? ADVERT_LOC_SHARE : ADVERT_LOC_NONE;
  the_mesh.savePrefs();
}

// Deliberately does not touch node_name -- there's no reachable "default
// name" without a new MyMesh passthrough (the boot-time default is derived
// from self_id.pub_key, which is private to MyMesh), and resetting a user's
// chosen name as a side effect of "restore defaults" would be surprising
// regardless. ConfirmScreen's message says so explicitly.
void Screen_SettingsAdvert::doRestoreDefaults(void* ctx) {
  Screen_SettingsAdvert* self = (Screen_SettingsAdvert*)ctx;
  self->_node_prefs->advert_loc_policy = ADVERT_LOC_NONE;
  the_mesh.savePrefs();
  self->_toast.show("Advert defaults restored", 1200);
}

void Screen_SettingsAdvert::confirmRestoreDefaults(void* ctx) {
  Screen_SettingsAdvert* self = (Screen_SettingsAdvert*)ctx;
  self->_confirm.begin("Restore advert defaults?", "Name is not reset", doRestoreDefaults, self, 3000);
  self->_nav.push(&self->_confirm);
}

Screen_SettingsAdvert::Screen_SettingsAdvert(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                                              TextField& textField, ToggleField& toggleField)
  : MenuScreen(nav, toast, "Advert", _rows, UI_SETTINGS_ADVERT_ITEM_COUNT, /*status_bar_shown=*/true),
    _nav(nav), _toast(toast), _confirm(confirm), _node_prefs(node_prefs) {

  _spec_name.nav = &_nav;
  _spec_name.field = &textField;
  _spec_name.title = "Name";
  _spec_name.get = getName;
  _spec_name.set = setName;
  _spec_name.ctx = this;
  _spec_name.max_len = sizeof(node_prefs->node_name) - 1;

  _spec_share_loc.nav = &_nav;
  _spec_share_loc.field = &toggleField;
  _spec_share_loc.title = "Share Location";
  _spec_share_loc.get = getShareLocation;
  _spec_share_loc.set = setShareLocation;
  _spec_share_loc.ctx = this;

  int i = 0;
  _rows[i].label = "Name"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Text;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_name; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Share Location"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_share_loc; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Restore Defaults"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmRestoreDefaults; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;
}
