#include "UITask.h"

void UITask::placeholderAction(void* ctx) {
  PlaceholderCtx* p = (PlaceholderCtx*)ctx;
  p->toast->show(p->label, 1000);
}

void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
  _display = display;
  _sensors = sensors;
  _node_prefs = node_prefs;

  _input.begin();

  if (_display != NULL) {
    _display->turnOn();
    _status_bar.begin(_display->width());
    _status_bar.setText(*_display, "ui-forest phase 0");
  }

  static const char* labels[UI_FOREST_HOME_ITEM_COUNT] = { "Item A", "Item B", "Item C" };
  for (int i = 0; i < UI_FOREST_HOME_ITEM_COUNT; i++) {
    _home_item_ctx[i].toast = &_toast;
    _home_item_ctx[i].label = labels[i];

    _home_items[i].label = labels[i];
    _home_items[i].icon = NULL;
    _home_items[i].kind = MenuItemKind::Action;
    _home_items[i].action = placeholderAction;
    _home_items[i].action_ctx = &_home_item_ctx[i];
    _home_items[i].submenu = NULL;
  }

  // constructed once here, matching the "no allocation outside setup" rule
  // ui-new already follows (PLAN.md 3.2/3.6) -- NavStack only ever moves
  // these pointers around afterward.
  _home = new MenuScreen(_nav, _toast, "Home", _home_items, UI_FOREST_HOME_ITEM_COUNT, /*status_bar_shown=*/true);
  _splash = new Screen_Splash(_nav, _home);
  _nav.reset(_splash);

  _next_refresh = 0;
}

void UITask::msgRead(int msgcount) {
  _msgcount = msgcount;
}

void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msgcount = msgcount;
  // Screen_MsgPreview lands in Phase 1 -- Phase 0 has no message screens yet.
}

void UITask::notify(UIEventType t) {
  // buzzer/vibration notification wiring lands in Phase 1.
}

void UITask::loop() {
  char c = _input.poll();

  if (c != 0 && _nav.current()) {
    _nav.current()->handleInput(c);
    _next_refresh = 0;   // trigger refresh
  }

  if (_nav.current()) _nav.current()->poll();

  if (_display != NULL && millis() >= _next_refresh) {
    _display->startFrame();

    int delay_millis = 1000;
    UIScreen* curr = _nav.current();
    if (curr) delay_millis = curr->render(*_display);

    if (_status_bar.needsRedraw()) _status_bar.render(*_display);
    _toast.composite(*_display);

    _display->endFrame();
    _next_refresh = millis() + delay_millis;
  }
}
