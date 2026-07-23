#pragma once

#include <helpers/ui/DisplayDriver.h>

// Adaptive layout helpers so screens never hardcode pixel y-offsets. The same
// MenuScreen code shows 4 rows on a 64px OLED and 10+ rows on a color TFT by
// asking Layout for row height / visible row count instead of using literal
// constants (PLAN.md 3.3).
namespace Layout {

  // DisplayDriver doesn't expose a font-height accessor, so this mirrors the
  // row spacing ui-new already uses by convention for text size 1 (y += 11,
  // e.g. examples/companion_radio/ui-new/UITask.cpp's RECENT page): 8px glyph
  // height + 3px padding, scaled per text size.
  inline int rowHeight(int textSize = 1) {
    return textSize * 8 + 3;
  }

  // Height of the one-row-tall screen header (icon + title).
  inline int headerHeight() {
    return rowHeight(1);
  }

  // Height of the optional top status bar strip, when present.
  inline int statusBarHeight(bool showing) {
    return showing ? rowHeight(1) : 0;
  }

  // Number of list rows that fit below the header (and status bar, if any).
  inline int visibleRows(DisplayDriver& display, bool statusBarShowing = false, int textSize = 1) {
    int usable = display.height() - headerHeight() - statusBarHeight(statusBarShowing);
    int rh = rowHeight(textSize);
    int rows = usable / rh;
    return rows > 0 ? rows : 1;
  }

  // Row height tall enough to also hold an optional 16x16 row icon (Phase 5,
  // PLAN.md 3.5) without it bleeding into the next row -- rowHeight(1) is
  // only 11px, shorter than a 16x16 icon, so any row that actually draws one
  // needs this instead. Screens/menus with no icons should keep using plain
  // rowHeight()/visibleRows() -- switching every list to this unconditionally
  // would shrink visible-row counts on small OLEDs for no reason on rows that
  // never draw an icon in the first place.
  inline int iconRowHeight(int textSize = 1) {
    int rh = rowHeight(textSize);
    return rh > 18 ? rh : 18;   // 16px icon + 1px margin top/bottom
  }

  inline int visibleIconRows(DisplayDriver& display, bool statusBarShowing = false, int textSize = 1) {
    int usable = display.height() - headerHeight() - statusBarHeight(statusBarShowing);
    int rh = iconRowHeight(textSize);
    int rows = usable / rh;
    return rows > 0 ? rows : 1;
  }

  // Color convention helper (Phase 5, item 30): screens pick a color to
  // convey meaning (green=good, red=warning, yellow=info), but on a
  // monochrome buffer (`!display.supportsColor()`) every non-DARK color
  // collapses to the same "on" pixel at the driver level -- picking RED vs
  // GREEN there doesn't fail, it just can't mean anything, so callers should
  // ask for plain LIGHT instead of relying on a hue nobody will see. Route
  // every semantic color choice through this instead of calling
  // display.setColor() with a raw RED/GREEN/YELLOW/etc directly.
  inline DisplayDriver::Color accentColor(DisplayDriver& display, DisplayDriver::Color wanted) {
    return display.supportsColor() ? wanted : DisplayDriver::LIGHT;
  }

  // Card-style grouping (Phase 5, item 28): left/right/bottom edges around a
  // screen's scrollable content block, drawn once below the header/separator
  // every label/value stat screen already draws -- that existing separator
  // line doubles as the card's top edge, so this deliberately doesn't add a
  // second one directly under it (which would sit right where the first
  // content row's text is drawn). Deliberately one border per screen, not a
  // line between every row -- on a 64px OLED at row_h=11, rows already sit
  // close together, and dividing each one would read as clutter, not polish,
  // at that size. Applied consistently via this one helper rather than each
  // screen hand-rolling its own drawRect/fillRect calls.
  inline void drawCard(DisplayDriver& display, int content_top) {
    display.setColor(DisplayDriver::LIGHT);
    int bottom = display.height() - 1;
    int h = bottom - content_top + 1;
    if (h <= 0) return;
    display.fillRect(0, content_top, 1, h);                  // left edge
    display.fillRect(display.width() - 1, content_top, 1, h); // right edge
    display.fillRect(0, bottom, display.width(), 1);           // bottom edge
  }

}
