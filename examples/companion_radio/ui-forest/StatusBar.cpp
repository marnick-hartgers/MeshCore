#include "StatusBar.h"
#include <Arduino.h>
#include <string.h>

StatusBar::StatusBar() : _text_width(0), _scroll_x(0), _display_width(72),
                          _next_scroll(0), _needs_redraw(true) {
  _text[0] = 0;
}

void StatusBar::begin(int display_width) {
  _display_width = display_width;
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

bool StatusBar::needsRedraw() const {
  if (_text_width <= _display_width) return _needs_redraw;  // static, no scrolling
  return millis() >= _next_scroll;
}

void StatusBar::render(DisplayDriver& display) {
  if (_text[0] == 0) return;

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);

  if (_text_width <= _display_width) {
    display.setCursor(0, 0);
    display.print(_text);
    _needs_redraw = false;
    return;
  }

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
