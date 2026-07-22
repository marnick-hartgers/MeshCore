#include "Screen_Splash.h"
#include "icons.h"
#include "../MyMesh.h"   // for FIRMWARE_VERSION / FIRMWARE_BUILD_DATE fallback defines
#include <Arduino.h>
#include <string.h>

Screen_Splash::Screen_Splash(NavStack& nav, UIScreen* home) : _nav(nav), _home(home) {
  const char* ver = FIRMWARE_VERSION;
  const char* dash = strchr(ver, '-');

  int len = dash ? dash - ver : strlen(ver);
  if (len >= (int)sizeof(_version_info)) len = sizeof(_version_info) - 1;
  memcpy(_version_info, ver, len);
  _version_info[len] = 0;

  _dismiss_after = millis() + BOOT_SCREEN_MILLIS;
}

int Screen_Splash::render(DisplayDriver& display) {
  display.setColor(DisplayDriver::BLUE);
  int logoWidth = 128;
  display.drawXbm((display.width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(1);
  display.drawTextCentered(display.width() / 2, 35, _version_info);
  display.drawTextCentered(display.width() / 2, 48, FIRMWARE_BUILD_DATE);

  return 1000;
}

void Screen_Splash::poll() {
  if (millis() >= _dismiss_after) {
    _nav.reset(_home);
  }
}

bool Screen_Splash::handleInput(char c) {
  _nav.reset(_home);
  return true;
}
