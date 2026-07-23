#include "Screen_SettingsNetwork.h"
#include "../MyMesh.h"

static const char* const TELEMETRY_LABELS[] = { "Deny", "Allow (flagged)", "Allow All" };
#define TELEMETRY_LABELS_COUNT 3

bool Screen_SettingsNetwork::getRepeat(void* ctx) {
  return ((Screen_SettingsNetwork*)ctx)->_node_prefs->client_repeat != 0;
}
// isValidClientRepeatFreq() (MyMesh.cpp) only allows a handful of exact
// frequencies -- mirrors CMD_SET_RADIO_PARAMS's own gate on the `repeat` wire
// field, just checked against the CURRENT frequency instead of a
// simultaneously-changing one, since here freq and repeat are edited on two
// separate screens rather than as one atomic command.
void Screen_SettingsNetwork::setRepeat(void* ctx, bool value) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  if (value) {
    uint32_t freq_key = (uint32_t)(self->_node_prefs->freq * 1000.0f + 0.5f);
    if (!the_mesh.isValidClientRepeatFreq(freq_key)) {
      self->_toast.show("Invalid freq for repeat", 1500);
      return;
    }
  }
  self->_node_prefs->client_repeat = value ? 1 : 0;
  the_mesh.savePrefs();
}

bool Screen_SettingsNetwork::getRxBoost(void* ctx) {
  return ((Screen_SettingsNetwork*)ctx)->_node_prefs->rx_boosted_gain != 0;
}
void Screen_SettingsNetwork::setRxBoost(void* ctx, bool value) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->rx_boosted_gain = value ? 1 : 0;
  the_mesh.savePrefs();
  radio_driver.setRxBoostedGainMode(self->_node_prefs->rx_boosted_gain);  // live-apply, mirrors begin()'s boot-time call
}

int Screen_SettingsNetwork::getTelemetryBase(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->telemetry_mode_base; }
void Screen_SettingsNetwork::setTelemetryBase(void* ctx, int index) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->telemetry_mode_base = (uint8_t)index;
  the_mesh.savePrefs();
}

int Screen_SettingsNetwork::getTelemetryLoc(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->telemetry_mode_loc; }
void Screen_SettingsNetwork::setTelemetryLoc(void* ctx, int index) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->telemetry_mode_loc = (uint8_t)index;
  the_mesh.savePrefs();
}

int Screen_SettingsNetwork::getTelemetryEnv(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->telemetry_mode_env; }
void Screen_SettingsNetwork::setTelemetryEnv(void* ctx, int index) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->telemetry_mode_env = (uint8_t)index;
  the_mesh.savePrefs();
}

// manual_add_contacts bit 0: 0 = auto-add enabled, 1 = manual-only
// (isAutoAddEnabled(), MyMesh.cpp) -- inverted for display so "ON" reads as
// "auto-add is happening".
bool Screen_SettingsNetwork::getAutoAdd(void* ctx) {
  return (((Screen_SettingsNetwork*)ctx)->_node_prefs->manual_add_contacts & 1) == 0;
}
void Screen_SettingsNetwork::setAutoAdd(void* ctx, bool value) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  uint8_t v = self->_node_prefs->manual_add_contacts;
  if (value) v &= ~1; else v |= 1;
  self->_node_prefs->manual_add_contacts = v;
  the_mesh.savePrefs();
}

float Screen_SettingsNetwork::getAutoAddMaxHops(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->autoadd_max_hops; }
void Screen_SettingsNetwork::setAutoAddMaxHops(void* ctx, float v) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->autoadd_max_hops = (uint8_t)(v + 0.5f);
  the_mesh.savePrefs();
}

float Screen_SettingsNetwork::getDutyCycle(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->airtime_factor; }
void Screen_SettingsNetwork::setDutyCycle(void* ctx, float v) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->airtime_factor = v;
  the_mesh.savePrefs();
}

float Screen_SettingsNetwork::getRxDelay(void* ctx) { return ((Screen_SettingsNetwork*)ctx)->_node_prefs->rx_delay_base; }
void Screen_SettingsNetwork::setRxDelay(void* ctx, float v) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_node_prefs->rx_delay_base = v;
  the_mesh.savePrefs();
}

void Screen_SettingsNetwork::doRestoreDefaults(void* ctx) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  NodePrefs* p = self->_node_prefs;

  p->client_repeat = 0;
#if defined(USE_SX1262) || defined(USE_SX1268)
#ifdef SX126X_RX_BOOSTED_GAIN
  p->rx_boosted_gain = SX126X_RX_BOOSTED_GAIN;
#else
  p->rx_boosted_gain = 1;
#endif
#else
  p->rx_boosted_gain = 1;
#endif
  p->telemetry_mode_base = TELEM_MODE_DENY;
  p->telemetry_mode_loc = TELEM_MODE_DENY;
  p->telemetry_mode_env = TELEM_MODE_DENY;
  p->manual_add_contacts = 0;
  p->autoadd_max_hops = 0;
  p->airtime_factor = 1.0f;
  p->rx_delay_base = 0.0f;

  the_mesh.savePrefs();
  radio_driver.setRxBoostedGainMode(p->rx_boosted_gain);
  self->_toast.show("Network defaults restored", 1200);
}

void Screen_SettingsNetwork::confirmRestoreDefaults(void* ctx) {
  Screen_SettingsNetwork* self = (Screen_SettingsNetwork*)ctx;
  self->_confirm.begin("Restore network defaults?", "Applies immediately", doRestoreDefaults, self, 3000);
  self->_nav.push(&self->_confirm);
}

Screen_SettingsNetwork::Screen_SettingsNetwork(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                                                ToggleField& toggleField, EnumField& enumField, StepperField& stepperField)
  : MenuScreen(nav, toast, "Network", _rows, UI_SETTINGS_NETWORK_ITEM_COUNT, /*status_bar_shown=*/true),
    _nav(nav), _toast(toast), _confirm(confirm), _node_prefs(node_prefs) {

  _spec_repeat.nav = &_nav;
  _spec_repeat.field = &toggleField;
  _spec_repeat.title = "Repeat";
  _spec_repeat.get = getRepeat;
  _spec_repeat.set = setRepeat;
  _spec_repeat.ctx = this;

  _spec_rx_boost.nav = &_nav;
  _spec_rx_boost.field = &toggleField;
  _spec_rx_boost.title = "RX Boost";
  _spec_rx_boost.get = getRxBoost;
  _spec_rx_boost.set = setRxBoost;
  _spec_rx_boost.ctx = this;

  _spec_autoadd.nav = &_nav;
  _spec_autoadd.field = &toggleField;
  _spec_autoadd.title = "Auto-add Contacts";
  _spec_autoadd.get = getAutoAdd;
  _spec_autoadd.set = setAutoAdd;
  _spec_autoadd.ctx = this;

  _spec_telem_base.nav = &_nav;
  _spec_telem_base.field = &enumField;
  _spec_telem_base.title = "Telemetry: Base";
  _spec_telem_base.labels = TELEMETRY_LABELS;
  _spec_telem_base.count = TELEMETRY_LABELS_COUNT;
  _spec_telem_base.get = getTelemetryBase;
  _spec_telem_base.set = setTelemetryBase;
  _spec_telem_base.ctx = this;

  _spec_telem_loc.nav = &_nav;
  _spec_telem_loc.field = &enumField;
  _spec_telem_loc.title = "Telemetry: Location";
  _spec_telem_loc.labels = TELEMETRY_LABELS;
  _spec_telem_loc.count = TELEMETRY_LABELS_COUNT;
  _spec_telem_loc.get = getTelemetryLoc;
  _spec_telem_loc.set = setTelemetryLoc;
  _spec_telem_loc.ctx = this;

  _spec_telem_env.nav = &_nav;
  _spec_telem_env.field = &enumField;
  _spec_telem_env.title = "Telemetry: Environment";
  _spec_telem_env.labels = TELEMETRY_LABELS;
  _spec_telem_env.count = TELEMETRY_LABELS_COUNT;
  _spec_telem_env.get = getTelemetryEnv;
  _spec_telem_env.set = setTelemetryEnv;
  _spec_telem_env.ctx = this;

  _spec_autoadd_hops.nav = &_nav;
  _spec_autoadd_hops.field = &stepperField;
  _spec_autoadd_hops.title = "Auto-add Max Hops";
  _spec_autoadd_hops.unit = "";
  _spec_autoadd_hops.get = getAutoAddMaxHops;
  _spec_autoadd_hops.set = setAutoAddMaxHops;
  _spec_autoadd_hops.ctx = this;
  _spec_autoadd_hops.min = 0.0f;
  _spec_autoadd_hops.max = 64.0f;
  _spec_autoadd_hops.step = 1.0f;
  _spec_autoadd_hops.decimals = 0;

  _spec_duty_cycle.nav = &_nav;
  _spec_duty_cycle.field = &stepperField;
  _spec_duty_cycle.title = "Duty Cycle Factor";
  _spec_duty_cycle.unit = "";
  _spec_duty_cycle.get = getDutyCycle;
  _spec_duty_cycle.set = setDutyCycle;
  _spec_duty_cycle.ctx = this;
  _spec_duty_cycle.min = 0.0f;
  _spec_duty_cycle.max = 9.0f;
  _spec_duty_cycle.step = 0.1f;
  _spec_duty_cycle.decimals = 2;

  _spec_rx_delay.nav = &_nav;
  _spec_rx_delay.field = &stepperField;
  _spec_rx_delay.title = "RX Delay Factor";
  _spec_rx_delay.unit = "";
  _spec_rx_delay.get = getRxDelay;
  _spec_rx_delay.set = setRxDelay;
  _spec_rx_delay.ctx = this;
  _spec_rx_delay.min = 0.0f;
  _spec_rx_delay.max = 20.0f;
  _spec_rx_delay.step = 0.5f;
  _spec_rx_delay.decimals = 2;

  int i = 0;
  _rows[i].label = "Repeat"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_repeat; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "RX Boost"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_rx_boost; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Telemetry: Base"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Enum;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_telem_base; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Telemetry: Location"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Enum;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_telem_loc; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Telemetry: Environment"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Enum;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_telem_env; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Auto-add Contacts"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_autoadd; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Auto-add Max Hops"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_autoadd_hops; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Duty Cycle Factor"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_duty_cycle; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "RX Delay Factor"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_rx_delay; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Restore Defaults"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmRestoreDefaults; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;
}
