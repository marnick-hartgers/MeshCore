#include "Screen_Diagnostics.h"

Screen_Diagnostics::Screen_Diagnostics(NavStack& nav, ToastOverlay& toast,
                                        UIScreen* radio, UIScreen* packets, UIScreen* core, UIScreen* event_log)
  : MenuScreen(nav, toast, "Diagnostics", _rows, UI_DIAGNOSTICS_ROOT_ITEM_COUNT, /*status_bar_shown=*/true) {

  int i = 0;
  _rows[i].label = "Radio"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = radio;
  i++;

  _rows[i].label = "Packets"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = packets;
  i++;

  _rows[i].label = "Core"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = core;
  i++;

  _rows[i].label = "Event Log"; _rows[i].icon = NULL; _rows[i].kind = MenuItemKind::Submenu;
  _rows[i].action = NULL; _rows[i].action_ctx = NULL; _rows[i].submenu = event_log;
  i++;
}
