#include "Screen_NotificationEventConfig.h"

bool Screen_NotificationEventConfig::getBuzzer(void* ctx) {
  return ((Screen_NotificationEventConfig*)ctx)->_cfg.buzzer;
}
void Screen_NotificationEventConfig::setBuzzer(void* ctx, bool value) {
  ((Screen_NotificationEventConfig*)ctx)->_cfg.buzzer = value;
}
#ifdef PIN_VIBRATION
bool Screen_NotificationEventConfig::getVibration(void* ctx) {
  return ((Screen_NotificationEventConfig*)ctx)->_cfg.vibration;
}
void Screen_NotificationEventConfig::setVibration(void* ctx, bool value) {
  ((Screen_NotificationEventConfig*)ctx)->_cfg.vibration = value;
}
#endif

Screen_NotificationEventConfig::Screen_NotificationEventConfig(NavStack& nav, ToastOverlay& toast, ToggleField& toggleField,
                                                                const char* title, NotificationTypeConfig& cfg)
  : MenuScreen(nav, toast, title, _rows, UI_NOTIF_EVENT_ITEM_COUNT, /*status_bar_shown=*/true), _cfg(cfg) {

  int i = 0;

  _spec_buzzer.nav = &nav;
  _spec_buzzer.field = &toggleField;
  _spec_buzzer.title = "Buzzer";
  _spec_buzzer.get = getBuzzer;
  _spec_buzzer.set = setBuzzer;
  _spec_buzzer.ctx = this;

  _rows[i].label = "Buzzer"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_buzzer; _rows[i].submenu = NULL;
  i++;

#ifdef PIN_VIBRATION
  _spec_vibration.nav = &nav;
  _spec_vibration.field = &toggleField;
  _spec_vibration.title = "Vibration";
  _spec_vibration.get = getVibration;
  _spec_vibration.set = setVibration;
  _spec_vibration.ctx = this;

  _rows[i].label = "Vibration"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Toggle;
  _rows[i].action = NULL; _rows[i].action_ctx = &_spec_vibration; _rows[i].submenu = NULL;
  i++;
#endif
}
