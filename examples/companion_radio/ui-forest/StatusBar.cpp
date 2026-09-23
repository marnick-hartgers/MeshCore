#include "StatusBar.h"
#include "icons.h"
#include "Layout.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#ifndef BATT_MIN_MILLIVOLTS
  #define BATT_MIN_MILLIVOLTS 3000
#endif
#ifndef BATT_MAX_MILLIVOLTS
  #define BATT_MAX_MILLIVOLTS 4200
#endif

// Phase 5: how long the unread badge stays inverted after a message arrives
// while connected (design-4-icon-tiles.md-adjacent plan.md §4's "1-2s
// invert/flash" proposal).
#define UNREAD_FLASH_MILLIS 1500

StatusBar::StatusBar() : _gps(GpsState::Off), _link(LinkState::Off), _buzzer_on(false),
                          _unread(0), _dirty(true), _unread_flash_until(0),
                          _batt_mv(0), _muted(false), _batt_dirty(true) { }

void StatusBar::begin() { }

void StatusBar::setGpsState(GpsState s) {
  if (s == _gps) return;
  _gps = s;
  _dirty = true;
}

void StatusBar::setLinkState(LinkState s) {
  if (s == _link) return;
  _link = s;
  _dirty = true;
}

void StatusBar::setBuzzer(bool on) {
  if (on == _buzzer_on) return;
  _buzzer_on = on;
  _dirty = true;
}

void StatusBar::setUnreadCount(int count) {
  if (count == _unread) return;
  _unread = count;
  _dirty = true;
}

void StatusBar::flashUnread() {
  _unread_flash_until = millis() + UNREAD_FLASH_MILLIS;
}

void StatusBar::setBattery(uint16_t milliVolts, bool muted) {
  if (milliVolts == _batt_mv && muted == _muted) return;
  _batt_mv = milliVolts;
  _muted = muted;
  _batt_dirty = true;
}

bool StatusBar::needsRedraw() const {
  // While the flash window is active, keep redrawing every pass so the
  // un-invert frame fires the instant it expires, not whenever something
  // else next happens to trigger a redraw.
  if (millis() < _unread_flash_until) return true;
  return _dirty || _batt_dirty;
}

void StatusBar::renderBattery(DisplayDriver& display) {
  int batteryPercentage = ((int)_batt_mv - BATT_MIN_MILLIVOLTS) * 100 / (BATT_MAX_MILLIVOLTS - BATT_MIN_MILLIVOLTS);
  if (batteryPercentage < 0) batteryPercentage = 0;
  if (batteryPercentage > 100) batteryPercentage = 100;

  int iconWidth = 20;
  int iconHeight = 8;
  int iconX = display.width() - iconWidth - 4;
  int iconY = 1;

  // The battery gauge is drawn last specifically so nothing else in the bar
  // paints over it -- but the gauge itself is only an outline + partial fill,
  // not a solid opaque block, so without clearing its bounding box first, any
  // stray pixels drawn earlier this frame that land inside that box but
  // outside the gauge's own lit segments would show through underneath it
  // (this is what the pre-Phase-2 marquee text used to do, reported as "text
  // mixed into the battery icon"). Clear the gauge's own footprint before
  // drawing anything on top of it.
  int clearX = iconX - 9;
  display.setColor(DisplayDriver::DARK);
  display.fillRect(clearX, 0, display.width() - clearX, iconHeight + 2);

  display.setColor(DisplayDriver::GREEN);

  display.drawRect(iconX, iconY, iconWidth, iconHeight);              // battery outline
  display.fillRect(iconX + iconWidth, iconY + (iconHeight / 4), 2, iconHeight / 2);  // "cap"

  int fillWidth = (batteryPercentage * (iconWidth - 4)) / 100;
  display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4);
}

void StatusBar::render(DisplayDriver& display) {
  display.setColor(DisplayDriver::LIGHT);

  int x = 0;
  display.drawXbm(x, 1, gps_status_icons_8[(int)_gps], 8, 8);
  x += 9;
  display.drawXbm(x, 1, link_status_icons_8[(int)_link], 8, 8);
  x += 9;
#ifdef PIN_BUZZER
  // Boards with no buzzer hardware have isBuzzerQuiet() report true
  // unconditionally (UITask::isBuzzerQuiet()), so this slot is skipped
  // entirely there rather than showing a permanently-muted icon -- same
  // reasoning Phase 1's battery-gauge overlay already applied. The slot
  // itself stays reserved (x still advances) so the icons after it don't
  // shift position per-board.
  display.drawXbm(x, 1, _buzzer_on ? buzzer_on_8 : muted_icon, 8, 8);
#endif
  x += 9;
  display.setTextSize(1);
  char count_buf[4] = "";
  if (_unread > 0) {
    if (_unread > 9) strcpy(count_buf, "9+");
    else snprintf(count_buf, sizeof(count_buf), "%d", _unread);
  }

  // Phase 5 (ambient notification blink): while the flash window is active,
  // invert the badge (icon + count) -- the same fillRect-then-contrasting-
  // color trick used everywhere else in this codebase for "highlighted"
  // (MenuScreen's selected row, Screen_HomeDashboard's focused tile), no new
  // drawing primitive needed.
  bool flashing = millis() < _unread_flash_until;
  int badge_w = 9 + (count_buf[0] ? display.getTextWidth(count_buf) : 0);
  if (flashing) {
    display.setColor(Layout::accentColor(display, DisplayDriver::LIGHT));
    display.fillRect(x, 0, badge_w, 10);
    display.setColor(DisplayDriver::DARK);
  } else {
    display.setColor(DisplayDriver::LIGHT);
  }
  display.drawXbm(x, 1, unread_envelope_8, 8, 8);
  x += 9;
  if (count_buf[0]) {
    display.setCursor(x, 1);
    display.print(count_buf);
  }

  _dirty = false;

  // drawn last so nothing else in the bar paints over it
  renderBattery(display);
  _batt_dirty = false;
}
