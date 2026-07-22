#include "Screen_Status.h"
#include "UITask.h"
#include "../MyMesh.h"
#include "Layout.h"
#include <stdio.h>
#ifdef WIFI_SSID
  #include <WiFi.h>
#endif

int Screen_Status::render(DisplayDriver& display) {
  char tmp[80];
  int top = Layout::statusBarHeight(true);

  display.setColor(DisplayDriver::YELLOW);
  display.setTextSize(2);
  sprintf(tmp, "MSG: %d", _task->getMsgCount());
  display.drawTextCentered(display.width() / 2, top + 6, tmp);

#ifdef WIFI_SSID
  IPAddress ip = WiFi.localIP();
  snprintf(tmp, sizeof(tmp), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  display.setTextSize(1);
  display.drawTextCentered(display.width() / 2, display.height() - 10, tmp);
#endif

  if (_task->hasConnection()) {
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, top + 29, "< Connected >");
  } else if (the_mesh.getBLEPin() != 0) {
    display.setColor(DisplayDriver::RED);
    display.setTextSize(2);
    sprintf(tmp, "Pin:%d", the_mesh.getBLEPin());
    display.drawTextCentered(display.width() / 2, top + 29, tmp);
  }

  return 1000;
}

bool Screen_Status::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  return false;
}
