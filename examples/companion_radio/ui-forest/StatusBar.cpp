#include "StatusBar.h"
#include "icons.h"
#include <Arduino.h>
#include <string.h>

#ifndef BATT_MIN_MILLIVOLTS
  #define BATT_MIN_MILLIVOLTS 3000
#endif
#ifndef BATT_MAX_MILLIVOLTS
  #define BATT_MAX_MILLIVOLTS 4200
#endif

StatusBar::StatusBar() : _text_width(0), _scroll_x(0), _display_width(72),
                          _next_scroll(0), _needs_redraw(true), _is_eink(false),
                          _batt_mv(0), _muted(false), _batt_dirty(true) {
  _text[0] = 0;
}

void StatusBar::begin(int display_width, bool is_eink) {
  _display_width = display_width;
  _is_eink = is_eink;
  _scroll_x = 0;
  _next_scroll = 0;
}

void StatusBar::setText(DisplayDriver& display, const char* text) {
  if (strcmp(text, _text) == 0) return;  // unchanged, skip rebuild

  strncpy(_text, text, sizeof(_text) - 1);
  _text[sizeof(_text) - 1] = 0;

  display.setTextSize(1);
  _text_width = display.getTextWidth(_text);
  _needs_redraw = true;
}

void StatusBar::setBattery(uint16_t milliVolts, bool muted) {
  if (milliVolts == _batt_mv && muted == _muted) return;
  _batt_mv = milliVolts;
  _muted = muted;
  _batt_dirty = true;
}

bool StatusBar::needsRedraw() const {
  if (_batt_dirty) return true;
  if (_is_eink || _text_width <= _display_width) return _needs_redraw;  // static, no scrolling
  return millis() >= _next_scroll;
}

void StatusBar::renderBattery(DisplayDriver& display) {
  int batteryPercentage = ((int)_batt_mv - BATT_MIN_MILLIVOLTS) * 100 / (BATT_MAX_MILLIVOLTS - BATT_MIN_MILLIVOLTS);
  if (batteryPercentage < 0) batteryPercentage = 0;
  if (batteryPercentage > 100) batteryPercentage = 100;

  int iconWidth = 20;
  int iconHeight = 8;
  int iconX = display.width() - iconWidth - 4;
  int iconY = 1;

  // The battery gauge is drawn last specifically so the scrolling text never
  // paints over it -- but the gauge itself is only an outline + partial fill
  // + optional muted glyph, not a solid opaque block, so without clearing its
  // bounding box first, any marquee text pixels that land inside that box but
  // outside the gauge's own lit segments show through underneath it (reported
  // as "text mixed into the battery icon"). Clear the full box -- gauge plus
  // the muted-icon slot to its left -- before drawing anything on top of it.
  int clearX = iconX - 9;
  display.setColor(DisplayDriver::DARK);
  display.fillRect(clearX, 0, display.width() - clearX, iconHeight + 2);

  display.setColor(DisplayDriver::GREEN);

  display.drawRect(iconX, iconY, iconWidth, iconHeight);              // battery outline
  display.fillRect(iconX + iconWidth, iconY + (iconHeight / 4), 2, iconHeight / 2);  // "cap"

  int fillWidth = (batteryPercentage * (iconWidth - 4)) / 100;
  display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4);

#ifdef PIN_BUZZER
  if (_muted) {
    display.setColor(DisplayDriver::RED);
    display.drawXbm(iconX - 9, iconY, muted_icon, 8, 8);
  }
#endif
}

void StatusBar::render(DisplayDriver& display) {
  if (_text[0] != 0) {
    display.setTextSize(1);
    display.setColor(DisplayDriver::GREEN);

    if (_is_eink) {
      // No marquee on e-ink (PLAN.md 3.3) -- show the truncated/static form,
      // even if the full text would have scrolled on a non-eink display.
      display.drawTextEllipsized(0, 0, _display_width, _text);
      _needs_redraw = false;
    } else if (_text_width <= _display_width) {
      display.setCursor(0, 0);
      display.print(_text);
      _needs_redraw = false;
    } else {
      int x = _scroll_x;
      do {
        display.setCursor(x, 0);
        display.print(_text);
        x += _text_width;
      } while (x < _display_width);

      _scroll_x--;
      if (_scroll_x <= -_text_width) _scroll_x = 0;

      _next_scroll = millis() + STATUS_BAR_SCROLL_MS;
      _needs_redraw = false;
    }
  } else {
    _needs_redraw = false;
  }

  // drawn last so the scrolling text never paints over it
  renderBattery(display);
  _batt_dirty = false;
}
