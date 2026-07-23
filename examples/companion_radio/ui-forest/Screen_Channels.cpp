#include "Screen_Channels.h"
#include "../MyMesh.h"
#include "Layout.h"
#include "icons.h"

// MAX_GROUP_CHANNELS is small (pilot envs set it to 40), so a full scan per
// lookup is cheap and avoids ever materializing the whole channel table in
// a buffer (PLAN.md 3.6) -- each scan only ever holds one ChannelDetails on
// the stack at a time.
static int countPopulatedChannels() {
  ChannelDetails tmp;
  int n = 0;
  for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
    if (the_mesh.getChannel(i, tmp) && tmp.name[0] != 0) n++;
  }
  return n;
}

// Maps a 0-based index into the filtered (non-empty-only) list back to its
// underlying channel slot, filling 'out' with that slot's details.
static int findNthPopulatedChannel(int visible_idx, ChannelDetails& out) {
  int n = 0;
  for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
    if (the_mesh.getChannel(i, out) && out.name[0] != 0) {
      if (n == visible_idx) return i;
      n++;
    }
  }
  return -1;
}

int Screen_Channels::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);
  int count = countPopulatedChannels();

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top);
  display.print("Channels");
  display.drawRect(0, top + Layout::headerHeight() - 2, display.width(), 1);

  if (count == 0) {
    display.setColor(DisplayDriver::LIGHT);
    display.drawTextLeftAlign(0, top + Layout::headerHeight() + 2, "No channels");
    return 1000;
  }

  if (_selected >= count) _selected = count - 1;
  if (_selected < 0) _selected = 0;

  // iconRowHeight/visibleIconRows (Phase 5) -- every row here draws a 16x16
  // channel_icon, which doesn't fit in the plain 11px rowHeight() without
  // bleeding into the next row (see Layout.h's comment).
  int visible = Layout::visibleIconRows(display, true);
  if (_selected < _scroll_offset) {
    _scroll_offset = _selected;
  } else if (_selected >= _scroll_offset + visible) {
    _scroll_offset = _selected - visible + 1;
  }

  int row_h = Layout::iconRowHeight();
  int y = top + Layout::headerHeight();
  for (int row = 0; row < visible; row++) {
    int idx = _scroll_offset + row;
    if (idx >= count) break;

    ChannelDetails ch;
    if (findNthPopulatedChannel(idx, ch) < 0) continue;

    if (idx == _selected) {
      display.setColor(DisplayDriver::GREEN);
      display.fillRect(0, y, display.width(), row_h);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }

    display.drawXbm(0, y + 1, channel_icon, 16, 16);

    char name_buf[32];
    display.translateUTF8ToBlocks(name_buf, ch.name, sizeof(name_buf));
    display.drawTextEllipsized(18, y + 1, display.width() - 18, name_buf);

    y += row_h;
  }

  return 1000;
}

bool Screen_Channels::handleInput(char c) {
  int count = countPopulatedChannels();

  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    if (count > 0) _selected = (_selected + 1) % count;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    if (count > 0) _selected = (_selected + count - 1) % count;
    return true;
  }
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  return false;
}
