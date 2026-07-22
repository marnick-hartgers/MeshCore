#include "Screen_Gps.h"
#include "UITask.h"
#include "Layout.h"
#include "target.h"
#include <helpers/SensorManager.h>
#include <stdio.h>
#include <string.h>

int Screen_Gps::render(DisplayDriver& display) {
  int y = Layout::statusBarHeight(true) + 2;
  char buf[50];
  bool gps_state = _task->getGPSState();

#ifdef PIN_GPS_SWITCH
  bool hw_gps_state = digitalRead(PIN_GPS_SWITCH);
  if (gps_state != hw_gps_state) {
    strcpy(buf, gps_state ? "gps off(hw)" : "gps off(sw)");
  } else {
    strcpy(buf, gps_state ? "gps on" : "gps off");
  }
#else
  strcpy(buf, gps_state ? "gps on" : "gps off");
#endif
  display.drawTextLeftAlign(0, y, buf);

  LocationProvider* nmea = sensors.getLocationProvider();
  if (nmea == NULL) {
    y += 12;
    display.drawTextLeftAlign(0, y, "Can't access GPS");
  } else {
    strcpy(buf, nmea->isValid() ? "fix" : "no fix");
    display.drawTextRightAlign(display.width() - 1, y, buf);
    y += 12;
    display.drawTextLeftAlign(0, y, "sat");
    sprintf(buf, "%d", nmea->satellitesCount());
    display.drawTextRightAlign(display.width() - 1, y, buf);
    y += 12;
    display.drawTextLeftAlign(0, y, "pos");
    sprintf(buf, "%.4f %.4f", nmea->getLatitude() / 1000000., nmea->getLongitude() / 1000000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);
    y += 12;
    display.drawTextLeftAlign(0, y, "alt");
    sprintf(buf, "%.2f", nmea->getAltitude() / 1000.);
    display.drawTextRightAlign(display.width() - 1, y, buf);
  }

  return 1000;
}

bool Screen_Gps::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    _task->toggleGPS();
    return true;
  }
  return false;
}
