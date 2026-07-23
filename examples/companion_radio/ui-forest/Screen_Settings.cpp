#include "Screen_Settings.h"

Screen_Settings::Screen_Settings(NavStack& nav, ToastOverlay& toast,
                                  UIScreen* radio, UIScreen* advert, UIScreen* network, UIScreen* device, UIScreen* danger)
  : MenuScreen(nav, toast, "Settings", _rows, UI_SETTINGS_ROOT_ITEM_COUNT, /*status_bar_shown=*/true) {

  int i = 0;
  _rows[i].label = "Radio"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = radio;
  i++;

  _rows[i].label = "Advert"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = advert;
  i++;

  _rows[i].label = "Network"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = network;
  i++;

  _rows[i].label = "Device"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = device;
  i++;

  _rows[i].label = "Danger Zone"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = danger;
  i++;
}
