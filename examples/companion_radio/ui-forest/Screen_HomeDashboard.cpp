#include "Screen_HomeDashboard.h"
#include "UITask.h"
#include "Layout.h"
#include "icons.h"
#include "../MyMesh.h"
#include <stdio.h>
#include <string.h>

#define TILE_GPS          0
#define TILE_BUZZER       1
#define TILE_CONTACTS     2
#define TILE_CHANNELS     3
#define TILE_SIGNAL       4
#define TILE_DIAGNOSTICS  5
#define TILE_SETTINGS     6
#define TILE_ADVERT       7
#define TILE_COUNT        8

// ~2.5s before the caption reverts from the focused tile's label/value back
// to the device name (design-4-icon-tiles.md's "Interaction" section: "for a
// short idle timeout").
#define CAPTION_HOLD_MILLIS 2500

static const char* const TILE_LABELS[TILE_COUNT] = {
  "GPS", "Buzzer", "Contacts", "Channels", "Signal", "Diagnostics", "Settings", "Advert"
};

// Drops every byte that's part of a multi-byte UTF-8 sequence (emoji, accented
// characters, etc.) instead of substituting a placeholder glyph -- moved here
// from UITask.cpp (Phase 2 of the home-dashboard build): the caption's device
// name is the only remaining consumer now that the status bar itself is
// icon-only.
static void stripNonAscii(char* dest, const char* src, size_t dest_size) {
  size_t j = 0;
  for (size_t i = 0; src[i] != 0 && j < dest_size - 1; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c < 0x80) dest[j++] = (char)c;   // ASCII byte -- keep
    // else: continuation/lead byte of a multi-byte UTF-8 sequence -- drop it
  }
  dest[j] = 0;
}

// Same small linear-scan helper Screen_Channels.cpp already uses to count
// populated channel slots (MAX_GROUP_CHANNELS is small -- pilot envs set it to
// 40 -- so a full scan per caption update is cheap, and never materializes the
// whole table). Kept as its own file-local copy rather than shared, since
// it's a 6-line scan and Screen_Channels.cpp's own copy is `static` (internal
// linkage, not reachable from here) -- not worth a new shared header for this.
static int countPopulatedChannels() {
  ChannelDetails tmp;
  int n = 0;
  for (int i = 0; i < MAX_GROUP_CHANNELS; i++) {
    if (the_mesh.getChannel(i, tmp) && tmp.name[0] != 0) n++;
  }
  return n;
}

const uint8_t* Screen_HomeDashboard::iconFor(int idx) {
  switch (idx) {
    case TILE_GPS:
      switch (_task->getGpsFixState()) {
        case GpsState::Fixed: return gps_fix_icon;
        case GpsState::NoFix: return gps_nofix_icon;
        default: return gps_off_icon;
      }
    case TILE_BUZZER:
      return _task->isBuzzerQuiet() ? buzzer_off_icon : buzzer_on_icon;
    case TILE_CONTACTS:
      return contact_icon;
    case TILE_CHANNELS:
      return channel_icon;
    case TILE_SIGNAL:
      // implementation-plan.md Phase 3.4: signal_bars' RSSI "no packet yet"
      // sentinel hasn't been verified against real hardware for every radio
      // backend (icons.h's own comment on signal_bars), so this deliberately
      // doesn't wire a live level here -- ship a safe static placeholder
      // instead. signal_bars[0] (no bars lit) rather than the plan's other
      // suggested option (reusing event_log_icon) -- reusing that icon here
      // would make the Signal and Diagnostics tiles indistinguishable at a
      // glance, and an empty meter never overstates signal quality the way a
      // full one would.
      return signal_bars[0];
    case TILE_DIAGNOSTICS:
      return event_log_icon;
    case TILE_SETTINGS:
      return settings_icon;
    case TILE_ADVERT:
      return advert_icon_16;
    default:
      return NULL;
  }
}

void Screen_HomeDashboard::captionFor(int idx, char* buf, size_t size) {
  switch (idx) {
    case TILE_GPS: {
      const char* v = "Off";
      switch (_task->getGpsFixState()) {
        case GpsState::Fixed: v = "Fix"; break;
        case GpsState::NoFix: v = "No Fix"; break;
        default: break;
      }
      snprintf(buf, size, "%s: %s", TILE_LABELS[idx], v);
      break;
    }
    case TILE_BUZZER:
      snprintf(buf, size, "%s: %s", TILE_LABELS[idx], _task->isBuzzerQuiet() ? "Off" : "On");
      break;
    case TILE_CONTACTS:
      snprintf(buf, size, "%s: %d", TILE_LABELS[idx], the_mesh.getNumContacts());
      break;
    case TILE_CHANNELS:
      snprintf(buf, size, "%s: %d", TILE_LABELS[idx], countPopulatedChannels());
      break;
    default:
      // Signal/Diagnostics/Settings/Advert have no natural "live value" --
      // just show the tile's name.
      strncpy(buf, TILE_LABELS[idx], size - 1);
      buf[size - 1] = 0;
      break;
  }
}

void Screen_HomeDashboard::renderTile(DisplayDriver& display, int idx, int x, int y, int w, int h) {
  // Reuses MenuScreen's exact selected-row convention (fillRect in the
  // background color, then draw the icon in DARK on top so it reads as a
  // cutout) since there's no invertRect primitive on DisplayDriver -- routed
  // through Layout::accentColor() rather than a raw color, unlike
  // MenuScreen's own direct GREEN (no need to replicate that inconsistency
  // here).
  if (idx == _focus) {
    display.setColor(Layout::accentColor(display, DisplayDriver::LIGHT));
    display.fillRect(x, y, w, h);
    display.setColor(DisplayDriver::DARK);
  } else {
    display.setColor(DisplayDriver::LIGHT);
  }

  const uint8_t* icon = iconFor(idx);
  if (icon != NULL) {
    int icon_x = x + (w - 16) / 2;
    int icon_y = y + (h - 16) / 2;
    display.drawXbm(icon_x, icon_y, icon, 16, 16);
  }
}

int Screen_HomeDashboard::render(DisplayDriver& display) {
  // Layout math (implementation-plan.md Phase 3.2): formulas, not hardcoded
  // pixels, so bigger panels degrade gracefully even before the deferred
  // bigger-grid work.
  int content_top = Layout::statusBarHeight(true);
  int caption_h = Layout::rowHeight(1);
  int grid_h = display.height() - content_top - caption_h;
  int cell_w = display.width() / 4;
  int cell_h = grid_h / 2;

  for (int idx = 0; idx < TILE_COUNT; idx++) {
    int col = idx % 4;
    int row = idx / 4;
    renderTile(display, idx, col * cell_w, content_top + row * cell_h, cell_w, cell_h);
  }

  // Caption line: device name by default, swapping to the focused tile's
  // label + live value for a short window after focus last changed.
  char caption[48];
  if (_caption_revert_at != 0 && millis() < _caption_revert_at) {
    captionFor(_focus, caption, sizeof(caption));
  } else {
    _caption_revert_at = 0;
    char name_buf[32];
    stripNonAscii(name_buf, _task->getNodeName(), sizeof(name_buf));
    strncpy(caption, name_buf, sizeof(caption) - 1);
    caption[sizeof(caption) - 1] = 0;
  }

  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextEllipsized(0, content_top + grid_h, display.width(), caption);

  return 1000;
}

bool Screen_HomeDashboard::handleInput(char c) {
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _focus = (_focus + 1) % TILE_COUNT;
    _caption_revert_at = millis() + CAPTION_HOLD_MILLIS;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _focus = (_focus + TILE_COUNT - 1) % TILE_COUNT;
    _caption_revert_at = millis() + CAPTION_HOLD_MILLIS;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    switch (_focus) {
      case TILE_GPS:
        _task->toggleGPS();
        _caption_revert_at = millis() + CAPTION_HOLD_MILLIS;
        break;
      case TILE_BUZZER:
        _task->toggleBuzzer();
        _caption_revert_at = millis() + CAPTION_HOLD_MILLIS;
        break;
      case TILE_CONTACTS:
        _nav.push(_contacts);
        break;
      case TILE_CHANNELS:
        _nav.push(_channels);
        break;
      case TILE_SIGNAL:
        // Glance-only tile, matches design-4-icon-tiles.md's original intent
        // for stat tiles that aren't Advert -- no action on ENTER.
        break;
      case TILE_DIAGNOSTICS:
        _nav.push(_diagnostics);
        break;
      case TILE_SETTINGS:
        _nav.push(_settings);
        break;
      case TILE_ADVERT:
        _nav.push(_advert);
        break;
    }
    return true;
  }
  // KEY_HOME needs no handling here -- already intercepted globally by
  // UITask::loop() before dispatch. KEY_CANCEL falls through to false --
  // Home is the nav root, and NavStack::pop() already refuses to pop below
  // depth 1, so there's nothing for it to do here.
  return false;
}
