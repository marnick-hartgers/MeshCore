#include "Screen_DiagRadio.h"
#include "Layout.h"
#include "target.h"
#include <stdio.h>

#define DIAG_RADIO_ROW_COUNT 3

int Screen_DiagRadio::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Radio");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  char noise_buf[16], rssi_buf[16], snr_buf[16];
  sprintf(noise_buf, "%d dBm", radio_driver.getNoiseFloor());
  sprintf(rssi_buf, "%.0f dBm", radio_driver.getLastRSSI());
  sprintf(snr_buf, "%.1f dB", radio_driver.getLastSNR());

  const char* labels[DIAG_RADIO_ROW_COUNT] = { "Noise Floor", "RSSI", "SNR" };
  const char* values[DIAG_RADIO_ROW_COUNT] = { noise_buf, rssi_buf, snr_buf };

  int visible = Layout::visibleRows(display, true);
  int max_offset = DIAG_RADIO_ROW_COUNT - visible;
  if (max_offset < 0) max_offset = 0;
  if (_scroll_offset > max_offset) _scroll_offset = max_offset;
  if (_scroll_offset < 0) _scroll_offset = 0;

  Layout::drawCard(display, top + Layout::headerHeight());

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int i = _scroll_offset + row;
    if (i >= DIAG_RADIO_ROW_COUNT) break;

    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(2, y, labels[i]);
    display.setColor(DisplayDriver::YELLOW);
    display.drawTextRightAlign(display.width() - 3, y, values[i]);

    y += row_h;
  }

  return 1000;
}

bool Screen_DiagRadio::handleInput(char c) {
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _scroll_offset++;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _scroll_offset--;
    return true;
  }
  if (c == KEY_CANCEL || c == KEY_ENTER || c == KEY_SELECT) {
    _nav.pop();
    return true;
  }
  return false;
}
