#include "Screen_Sensors.h"
#include "UITask.h"
#include "Layout.h"
#include "target.h"
#include <helpers/SensorManager.h>
#include <stdio.h>
#include <string.h>

#ifndef AUTO_OFF_MILLIS
  #define AUTO_OFF_MILLIS 15000   // 15 seconds
#endif

void Screen_Sensors::refresh_sensors() {
  if (millis() > next_sensors_refresh) {
    sensors_lpp.reset();
    sensors_nb = 0;
    sensors_lpp.addVoltage(TELEM_CHANNEL_SELF, (float)_task->getBattMilliVolts() / 1000.0f);
    sensors.querySensors(0xFF, sensors_lpp);
    LPPReader reader(sensors_lpp.getBuffer(), sensors_lpp.getSize());
    uint8_t channel, type;
    while (reader.readHeader(channel, type)) {
      reader.skipData(type);
      sensors_nb++;
    }
    sensors_scroll = sensors_nb > UI_RECENT_LIST_SIZE;
#if AUTO_OFF_MILLIS > 0
    next_sensors_refresh = millis() + 5000;   // refresh sensor values every 5 sec
#else
    next_sensors_refresh = millis() + 60000;  // refresh sensor values every 1 min
#endif
  }
}

int Screen_Sensors::render(DisplayDriver& display) {
  int y = Layout::statusBarHeight(true) + 2;
  refresh_sensors();
  char buf[30];
  char name[30];
  LPPReader r(sensors_lpp.getBuffer(), sensors_lpp.getSize());

  for (int i = 0; i < sensors_scroll_offset; i++) {
    uint8_t channel, type;
    r.readHeader(channel, type);
    r.skipData(type);
  }

  for (int i = 0; i < (sensors_scroll ? UI_RECENT_LIST_SIZE : sensors_nb); i++) {
    uint8_t channel, type;
    if (!r.readHeader(channel, type)) {  // reached end, reset
      r.reset();
      r.readHeader(channel, type);
    }

    float v;
    switch (type) {
      case LPP_GPS: {
        float lat, lon, alt;
        r.readGPS(lat, lon, alt);
        strcpy(name, "gps"); sprintf(buf, "%.4f %.4f", lat, lon);
        break;
      }
      case LPP_VOLTAGE:
        r.readVoltage(v);
        strcpy(name, "voltage"); sprintf(buf, "%6.2f", v);
        break;
      case LPP_CURRENT:
        r.readCurrent(v);
        strcpy(name, "current"); sprintf(buf, "%.3f", v);
        break;
      case LPP_TEMPERATURE:
        r.readTemperature(v);
        strcpy(name, "temperature"); sprintf(buf, "%.2f", v);
        break;
      case LPP_RELATIVE_HUMIDITY:
        r.readRelativeHumidity(v);
        strcpy(name, "humidity"); sprintf(buf, "%.2f", v);
        break;
      case LPP_BAROMETRIC_PRESSURE:
        r.readPressure(v);
        strcpy(name, "pressure"); sprintf(buf, "%.2f", v);
        break;
      case LPP_ALTITUDE:
        r.readAltitude(v);
        strcpy(name, "altitude"); sprintf(buf, "%.0f", v);
        break;
      case LPP_POWER:
        r.readPower(v);
        strcpy(name, "power"); sprintf(buf, "%6.2f", v);
        break;
      default:
        r.skipData(type);
        strcpy(name, "unk"); buf[0] = 0;
    }

    display.setColor(DisplayDriver::YELLOW);
    display.setCursor(0, y);
    display.print(name);
    display.setCursor(display.width() - display.getTextWidth(buf) - 1, y);
    display.print(buf);
    y += Layout::rowHeight() + 1;
  }

  if (sensors_scroll) sensors_scroll_offset = (sensors_scroll_offset + 1) % sensors_nb;
  else sensors_scroll_offset = 0;

  return 1000;
}

bool Screen_Sensors::handleInput(char c) {
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    // Mirrors ui-new's HomePage::SENSORS ENTER handler exactly
    // (examples/companion_radio/ui-new/UITask.cpp:451-457) -- it really does
    // toggle GPS from the Sensors screen, not a ui-forest bug.
    _task->toggleGPS();
    next_sensors_refresh = 0;
    return true;
  }
  return false;
}
