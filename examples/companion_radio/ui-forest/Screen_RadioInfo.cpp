#include "Screen_RadioInfo.h"
#include "Layout.h"
#include "target.h"
#include <stdio.h>

int Screen_RadioInfo::render(DisplayDriver& display) {
  char tmp[40];
  int y = Layout::statusBarHeight(true) + 2;

  display.setColor(DisplayDriver::YELLOW);
  display.setTextSize(1);

  display.setCursor(0, y);
  sprintf(tmp, "FQ: %06.3f   SF: %d", _node_prefs->freq, _node_prefs->sf);
  display.print(tmp);
  y += 11;

  display.setCursor(0, y);
  sprintf(tmp, "BW: %03.2f     CR: %d", _node_prefs->bw, _node_prefs->cr);
  display.print(tmp);
  y += 11;

  display.setCursor(0, y);
  sprintf(tmp, "TX: %ddBm", _node_prefs->tx_power_dbm);
  display.print(tmp);
  y += 11;

  display.setCursor(0, y);
  sprintf(tmp, "Noise floor: %d", radio_driver.getNoiseFloor());
  display.print(tmp);

  return 1000;
}

bool Screen_RadioInfo::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  return false;
}
