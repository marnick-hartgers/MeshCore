#include "Screen_NotificationSettings.h"

Screen_NotificationSettings::Screen_NotificationSettings(NavStack& nav, ToastOverlay& toast, ToggleField& toggleField,
                                                          NotificationPrefs& prefs)
  : MenuScreen(nav, toast, "Notifications", _rows, UI_NOTIFICATION_SETTINGS_ITEM_COUNT, /*status_bar_shown=*/true),
    _cfg_message(nav, toast, toggleField, "Message", prefs.contactMessage),
    _cfg_channel(nav, toast, toggleField, "Channel Message", prefs.channelMessage),
    _cfg_ack(nav, toast, toggleField, "Ack", prefs.ack),
    _cfg_advert(nav, toast, toggleField, "Advert", prefs.advertSent) {

  int i = 0;
  _rows[i].label = "Message"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = &_cfg_message;
  i++;

  _rows[i].label = "Channel Message"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = &_cfg_channel;
  i++;

  _rows[i].label = "Ack"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = &_cfg_ack;
  i++;

  _rows[i].label = "Advert"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = &_cfg_advert;
  i++;
}
