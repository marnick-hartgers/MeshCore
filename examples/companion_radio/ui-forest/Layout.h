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

}
