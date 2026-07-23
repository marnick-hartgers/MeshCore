#include "Screen_DiagPackets.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "target.h"
#include <stdio.h>

#define DIAG_PACKETS_ROW_COUNT 7

int Screen_DiagPackets::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Packets");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  char recv_buf[12], sent_buf[12], flood_tx_buf[12], direct_tx_buf[12], flood_rx_buf[12], direct_rx_buf[12], errors_buf[12];
  sprintf(recv_buf, "%lu", (unsigned long)radio_driver.getPacketsRecv());
  sprintf(sent_buf, "%lu", (unsigned long)radio_driver.getPacketsSent());
  sprintf(flood_tx_buf, "%lu", (unsigned long)the_mesh.getNumSentFlood());
  sprintf(direct_tx_buf, "%lu", (unsigned long)the_mesh.getNumSentDirect());
  sprintf(flood_rx_buf, "%lu", (unsigned long)the_mesh.getNumRecvFlood());
  sprintf(direct_rx_buf, "%lu", (unsigned long)the_mesh.getNumRecvDirect());
  sprintf(errors_buf, "%lu", (unsigned long)radio_driver.getPacketsRecvErrors());

  const char* labels[DIAG_PACKETS_ROW_COUNT] = {
    "Received", "Sent", "Flood TX", "Direct TX", "Flood RX", "Direct RX", "RX Errors"
  };
  const char* values[DIAG_PACKETS_ROW_COUNT] = {
    recv_buf, sent_buf, flood_tx_buf, direct_tx_buf, flood_rx_buf, direct_rx_buf, errors_buf
  };

  int visible = Layout::visibleRows(display, true);
  int max_offset = DIAG_PACKETS_ROW_COUNT - visible;
  if (max_offset < 0) max_offset = 0;
  if (_scroll_offset > max_offset) _scroll_offset = max_offset;
  if (_scroll_offset < 0) _scroll_offset = 0;

  Layout::drawCard(display, top + Layout::headerHeight());

  int row_h = Layout::rowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int i = _scroll_offset + row;
    if (i >= DIAG_PACKETS_ROW_COUNT) break;

    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(2, y, labels[i]);
    // Color convention (Phase 5, item 30): RX errors are the one row here
    // that's a bad-news signal, not a neutral counter -- red once any have
    // been seen, matching red=warning; every other counter stays yellow=info.
    bool is_rx_errors = (i == DIAG_PACKETS_ROW_COUNT - 1);
    bool errors_present = is_rx_errors && radio_driver.getPacketsRecvErrors() > 0;
    display.setColor(Layout::accentColor(display, errors_present ? DisplayDriver::RED : DisplayDriver::YELLOW));
    display.drawTextRightAlign(display.width() - 3, y, values[i]);

    y += row_h;
  }

  return 1000;
}

bool Screen_DiagPackets::handleInput(char c) {
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
