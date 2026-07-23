#include "Screen_SettingsRadio.h"
#include "../MyMesh.h"
#include <math.h>

// Standard LoRa bandwidth steps -- CMD_SET_RADIO_PARAMS's own wire validation
// only range-checks bw (7000..500000, in Hz*1000), it doesn't enforce a
// discrete set, but the radio hardware itself only supports these values, so
// this is an Enum (safe, predictable) rather than a free-form Stepper.
static const float BW_VALUES[] = { 7.8f, 10.4f, 15.6f, 20.8f, 31.25f, 41.7f, 62.5f, 125.0f, 250.0f, 500.0f };
static const char* const BW_LABELS[] = {
  "7.8 kHz", "10.4 kHz", "15.6 kHz", "20.8 kHz", "31.25 kHz",
  "41.7 kHz", "62.5 kHz", "125 kHz", "250 kHz", "500 kHz"
};
#define BW_VALUES_COUNT ((int)(sizeof(BW_VALUES) / sizeof(BW_VALUES[0])))

// Shared by every one of freq/bw/sf/cr's setters -- CMD_SET_RADIO_PARAMS sets
// all four together in one radio_driver.setParams() call (MyMesh.cpp), so
// editing any one of them here re-applies all four current values, not just
// the one the user just changed.
void Screen_SettingsRadio::applyRadioParams() {
  the_mesh.savePrefs();
  radio_driver.setParams(_node_prefs->freq, _node_prefs->bw, _node_prefs->sf, _node_prefs->cr);
  _event_log.push("Radio params changed");
}

// isValidClientRepeatFreq() only permits a handful of exact frequencies
// (MyMesh.cpp); if the user tunes away from one while "Repeat" (client_repeat)
// is on, silently turn it back off rather than leaving it enabled on a
// frequency the real CMD_SET_RADIO_PARAMS handler would have rejected.
void Screen_SettingsRadio::checkRepeatStillValid() {
  if (_node_prefs->client_repeat &&
      !the_mesh.isValidClientRepeatFreq((uint32_t)(_node_prefs->freq * 1000.0f + 0.5f))) {
    _node_prefs->client_repeat = 0;
    the_mesh.savePrefs();
    _toast.show("Repeat disabled (freq)", 1500);
  }
}

float Screen_SettingsRadio::getFrequency(void* ctx) {
  return ((Screen_SettingsRadio*)ctx)->_node_prefs->freq;
}
void Screen_SettingsRadio::setFrequency(void* ctx, float v) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->freq = v;
  self->applyRadioParams();
  self->checkRepeatStillValid();
}

int Screen_SettingsRadio::getBandwidthIndex(void* ctx) {
  NodePrefs* p = ((Screen_SettingsRadio*)ctx)->_node_prefs;
  int best = 0;
  float best_diff = fabsf(p->bw - BW_VALUES[0]);
  for (int i = 1; i < BW_VALUES_COUNT; i++) {
    float diff = fabsf(p->bw - BW_VALUES[i]);
    if (diff < best_diff) { best_diff = diff; best = i; }
  }
  return best;
}
void Screen_SettingsRadio::setBandwidthIndex(void* ctx, int index) {
  if (index < 0 || index >= BW_VALUES_COUNT) return;
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->bw = BW_VALUES[index];
  self->applyRadioParams();
}

float Screen_SettingsRadio::getSpreadingFactor(void* ctx) {
  return ((Screen_SettingsRadio*)ctx)->_node_prefs->sf;
}
void Screen_SettingsRadio::setSpreadingFactor(void* ctx, float v) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->sf = (uint8_t)(v + 0.5f);
  self->applyRadioParams();
}

float Screen_SettingsRadio::getCodingRate(void* ctx) {
  return ((Screen_SettingsRadio*)ctx)->_node_prefs->cr;
}
void Screen_SettingsRadio::setCodingRate(void* ctx, float v) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->cr = (uint8_t)(v + 0.5f);
  self->applyRadioParams();
}

float Screen_SettingsRadio::getTxPower(void* ctx) {
  return ((Screen_SettingsRadio*)ctx)->_node_prefs->tx_power_dbm;
}
void Screen_SettingsRadio::setTxPower(void* ctx, float v) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->tx_power_dbm = (int8_t)(v >= 0 ? v + 0.5f : v - 0.5f);
  the_mesh.savePrefs();
  radio_driver.setTxPower(self->_node_prefs->tx_power_dbm);   // CMD_SET_RADIO_TX_POWER is a separate command/call from setParams()
  self->_event_log.push("Radio params changed");
}

void Screen_SettingsRadio::doRestoreDefaults(void* ctx) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_node_prefs->freq = LORA_FREQ;
  self->_node_prefs->bw = LORA_BW;
  self->_node_prefs->sf = LORA_SF;
  self->_node_prefs->cr = LORA_CR;
  self->_node_prefs->tx_power_dbm = LORA_TX_POWER;
  self->applyRadioParams();
  radio_driver.setTxPower(self->_node_prefs->tx_power_dbm);
  self->checkRepeatStillValid();
  self->_toast.show("Radio defaults restored", 1200);
}

void Screen_SettingsRadio::confirmRestoreDefaults(void* ctx) {
  Screen_SettingsRadio* self = (Screen_SettingsRadio*)ctx;
  self->_confirm.begin("Restore radio defaults?", "Applies immediately", doRestoreDefaults, self, 3000);
  self->_nav.push(&self->_confirm);
}

Screen_SettingsRadio::Screen_SettingsRadio(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, NodePrefs* node_prefs,
                                            StepperField& stepper, EnumField& enumField, EventLog& event_log)
  : MenuScreen(nav, toast, "Radio", _rows, UI_SETTINGS_RADIO_ITEM_COUNT, /*status_bar_shown=*/true),
    _nav(nav), _toast(toast), _confirm(confirm), _node_prefs(node_prefs), _event_log(event_log) {

  _spec_freq.nav = &_nav;
  _spec_freq.field = &stepper;
  _spec_freq.title = "Frequency (MHz)";
  _spec_freq.unit = "";
  _spec_freq.get = getFrequency;
  _spec_freq.set = setFrequency;
  _spec_freq.ctx = this;
  _spec_freq.min = 150.0f;
  _spec_freq.max = 2500.0f;
  _spec_freq.step = 0.1f;
  _spec_freq.decimals = 3;

  _spec_bw.nav = &_nav;
  _spec_bw.field = &enumField;
  _spec_bw.title = "Bandwidth";
  _spec_bw.labels = BW_LABELS;
  _spec_bw.count = BW_VALUES_COUNT;
  _spec_bw.get = getBandwidthIndex;
  _spec_bw.set = setBandwidthIndex;
  _spec_bw.ctx = this;

  _spec_sf.nav = &_nav;
  _spec_sf.field = &stepper;
  _spec_sf.title = "Spreading Factor";
  _spec_sf.unit = "";
  _spec_sf.get = getSpreadingFactor;
  _spec_sf.set = setSpreadingFactor;
  _spec_sf.ctx = this;
  _spec_sf.min = 5.0f;
  _spec_sf.max = 12.0f;
  _spec_sf.step = 1.0f;
  _spec_sf.decimals = 0;

  _spec_cr.nav = &_nav;
  _spec_cr.field = &stepper;
  _spec_cr.title = "Coding Rate";
  _spec_cr.unit = "";
  _spec_cr.get = getCodingRate;
  _spec_cr.set = setCodingRate;
  _spec_cr.ctx = this;
  _spec_cr.min = 5.0f;
  _spec_cr.max = 8.0f;
  _spec_cr.step = 1.0f;
  _spec_cr.decimals = 0;

  _spec_txpower.nav = &_nav;
  _spec_txpower.field = &stepper;
  _spec_txpower.title = "TX Power";
  _spec_txpower.unit = " dBm";
  _spec_txpower.get = getTxPower;
  _spec_txpower.set = setTxPower;
  _spec_txpower.ctx = this;
  _spec_txpower.min = -9.0f;
  _spec_txpower.max = (float)MAX_LORA_TX_POWER;
  _spec_txpower.step = 1.0f;
  _spec_txpower.decimals = 0;

  int i = 0;
  _rows[i].label = "Frequency"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_freq; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Bandwidth"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Enum;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_bw; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Spreading Factor"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_sf; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Coding Rate"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_cr; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "TX Power"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Stepper;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_txpower; _rows[i].submenu = NULL;
  i++;

  _rows[i].label = "Restore Defaults"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Action;
  _rows[i].action = confirmRestoreDefaults; _rows[i].action_ctx = this; _rows[i].submenu = NULL;
  i++;
}
