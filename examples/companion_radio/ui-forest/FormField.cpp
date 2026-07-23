#include "FormField.h"
#include "Layout.h"
#include "InputRouter.h"
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------- ToggleField

int ToggleField::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);
  bool value = _get ? _get(_ctx) : false;

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top + 2);
  display.print(_title);

  display.setTextSize(2);
  // Color convention (Phase 5, item 30): green=on, red=off, downgraded to
  // plain LIGHT via Layout::accentColor() on displays that can't show the
  // difference.
  display.setColor(Layout::accentColor(display, value ? DisplayDriver::GREEN : DisplayDriver::RED));
  display.drawTextCentered(display.width() / 2, top + 20, value ? "ON" : "OFF");

  // Phase 6 (item 32): built from the active board's real gesture vocabulary
  // (InputRouter::activateHint()) instead of a hardcoded "ENTER: toggle" --
  // "long press"/"tap"/"click" etc, matching whatever gesture actually
  // performs KEY_ENTER on this board.
  char hint[40];
  snprintf(hint, sizeof(hint), "%s: toggle", InputRouter::activateHint());
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 400;
}

bool ToggleField::handleInput(char c) {
  // PREV aliased to cancel/back here (same precedent as every Phase 1 leaf
  // screen, see ARCHITECTURE.md) -- a two-state toggle has no other use for
  // it.
  if (c == KEY_CANCEL || c == KEY_PREV) {
    _nav.pop();
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    if (_get && _set) {
      bool value = _get(_ctx);
      _set(_ctx, !value);
    }
    return true;
  }
  return false;
}

// --------------------------------------------------------------- StepperField

void StepperField::begin(const char* title, const char* unit, StepperGetFn get, StepperSetFn set, void* ctx,
                          float min, float max, float step, int decimals) {
  _title = title; _unit = unit; _get = get; _set = set; _ctx = ctx;
  _min = min; _max = max; _step = step; _decimals = decimals;
  _pending = _get ? _get(_ctx) : min;
  if (_pending < _min) _pending = _min;
  if (_pending > _max) _pending = _max;
}

int StepperField::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);
  char buf[32];

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top + 2);
  display.print(_title);

  const char* unit = _unit ? _unit : "";
  switch (_decimals) {
    // Static format literals only (no dynamically-built format string) --
    // every current caller uses 0, 2, or 3 decimals; fall back to 3 for
    // anything else rather than adding more cases speculatively.
    case 1:  snprintf(buf, sizeof(buf), "%.1f%s", _pending, unit); break;
    case 2:  snprintf(buf, sizeof(buf), "%.2f%s", _pending, unit); break;
    case 3:  snprintf(buf, sizeof(buf), "%.3f%s", _pending, unit); break;
    default:
      if (_decimals <= 0) {
        snprintf(buf, sizeof(buf), "%d%s", (int)(_pending >= 0 ? _pending + 0.5f : _pending - 0.5f), unit);
      } else {
        snprintf(buf, sizeof(buf), "%.3f%s", _pending, unit);
      }
      break;
  }
  display.setTextSize(2);
  display.setColor(DisplayDriver::YELLOW);
  display.drawTextCentered(display.width() / 2, top + 20, buf);

  // Phase 6 (item 32): "< adjust" doesn't make sense on a touch/rotary/
  // trackball board -- built from InputRouter::moveHint()/activateHint()
  // instead of the literal button-press wording.
  char hint[40];
  snprintf(hint, sizeof(hint), "%s: adjust, %s: save", InputRouter::moveHint(), InputRouter::activateHint());
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 200;
}

bool StepperField::handleInput(char c) {
  // No KEY_PREV alias to cancel here -- PREV is the decrement gesture.
  // KEY_HOME (three steps up, from any depth) is the single-button escape
  // hatch if the user wants out without saving, same as list screens.
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _pending += _step;
    if (_pending > _max) _pending = _max;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _pending -= _step;
    if (_pending < _min) _pending = _min;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    if (_set) _set(_ctx, _pending);
    _nav.pop();
    return true;
  }
  return false;
}

// ----------------------------------------------------------------- EnumField

void EnumField::begin(const char* title, const char* const* labels, int count, EnumGetFn get, EnumSetFn set, void* ctx) {
  _title = title; _labels = labels; _count = count; _get = get; _set = set; _ctx = ctx;
  _pending = _get ? _get(_ctx) : 0;
  if (_pending < 0) _pending = 0;
  if (_pending >= _count) _pending = _count > 0 ? _count - 1 : 0;
}

int EnumField::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top + 2);
  display.print(_title);

  display.setTextSize(2);
  display.setColor(DisplayDriver::YELLOW);
  display.drawTextCentered(display.width() / 2, top + 20, (_count > 0) ? _labels[_pending] : "");

  char buf[16];
  snprintf(buf, sizeof(buf), "%d / %d", _pending + 1, _count);
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), buf);

  return 200;
}

bool EnumField::handleInput(char c) {
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  if (_count <= 0) return false;
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    _pending = (_pending + 1) % _count;
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    _pending = (_pending + _count - 1) % _count;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    if (_set) _set(_ctx, _pending);
    _nav.pop();
    return true;
  }
  return false;
}

// ----------------------------------------------------------------- TextField

// Cycle order deliberately starts at space so an empty/append slot's first
// increment lands on 'A' (index 1), not an invisible no-op. No punctuation
// beyond '-'/'_' -- keeps the increment-only single-button path from being
// any more tedious than it already is.
static const char TEXT_FIELD_CHARSET[] =
  " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
#define TEXT_FIELD_CHARSET_LEN ((int)(sizeof(TEXT_FIELD_CHARSET) - 1))

static int textFieldCharsetIndexOf(char c) {
  for (int i = 0; i < TEXT_FIELD_CHARSET_LEN; i++) {
    if (TEXT_FIELD_CHARSET[i] == c) return i;
  }
  return 0;
}

void TextField::begin(const char* title, TextGetFn get, TextSetFn set, void* ctx, int max_len) {
  _title = title; _set = set; _ctx = ctx;
  _max_len = (max_len < TEXT_FIELD_BUF_SIZE - 1) ? max_len : TEXT_FIELD_BUF_SIZE - 1;
  _buf[0] = 0;
  // Pass _max_len (<= TEXT_FIELD_BUF_SIZE - 1), NOT sizeof(_buf) -- get()'s
  // contract (see TextGetFn's typedef comment) is "dest has at least
  // max_len+1 bytes", so passing the full buffer size here would let a
  // get() implementation write one byte past the end of _buf.
  if (get) get(ctx, _buf, _max_len);
  _len = strlen(_buf);
  if (_len > _max_len) _len = _max_len;
  _buf[_len] = 0;
  _cursor = 0;
}

int TextField::render(DisplayDriver& display) {
  int top = Layout::statusBarHeight(true);

  display.setTextSize(1);
  display.setColor(DisplayDriver::GREEN);
  display.setCursor(0, top + 2);
  display.print(_title);

  int y = top + 20;
  display.setTextSize(2);
  display.setColor(DisplayDriver::YELLOW);
  char shown[TEXT_FIELD_BUF_SIZE];
  strncpy(shown, _buf, sizeof(shown) - 1);
  shown[sizeof(shown) - 1] = 0;
  int text_w = display.getTextWidth(shown);
  int x = (display.width() - text_w) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(shown);

  // underline the character at the cursor, measuring the prefix width so this
  // lines up correctly even on variable-width fonts (see
  // DisplayDriver::drawTextEllipsized's own fixed-vs-variable-width heuristic)
  char prefix[TEXT_FIELD_BUF_SIZE];
  int n = _cursor;
  if (n > (int)sizeof(prefix) - 1) n = sizeof(prefix) - 1;
  memcpy(prefix, _buf, n);
  prefix[n] = 0;
  int caret_x = x + display.getTextWidth(prefix);
  char cur_char[2] = { (char)((_cursor < _len) ? _buf[_cursor] : ' '), 0 };
  int caret_w = display.getTextWidth(cur_char);
  if (caret_w < 6) caret_w = 10;
  display.setColor(DisplayDriver::RED);
  display.fillRect(caret_x, y + 18, caret_w, 2);

  // Phase 6 (item 32/34): on lilygo_tdeck this now advertises the real
  // keyboard path alongside the still-working increment picker (see
  // InputRouter::textEntryHint()); every other board keeps the original
  // "PREV: next char, ENTER: save" wording, just built from activateHint()
  // instead of a literal "ENTER".
  char hint[48];
  InputRouter::textEntryHint(hint, sizeof(hint));
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.drawTextCentered(display.width() / 2, display.height() - Layout::rowHeight(), hint);

  return 300;
}

bool TextField::handleInput(char c) {
  if (c == KEY_CANCEL) {
    _nav.pop();
    return true;
  }
  if (c == KEY_NEXT || c == KEY_DOWN || c == KEY_RIGHT) {
    char cur = (_cursor < _len) ? _buf[_cursor] : TEXT_FIELD_CHARSET[0];
    int idx = (textFieldCharsetIndexOf(cur) + 1) % TEXT_FIELD_CHARSET_LEN;
    _buf[_cursor] = TEXT_FIELD_CHARSET[idx];
    if (_cursor == _len && _len < _max_len) {
      _len++;
      _buf[_len] = 0;
    }
    return true;
  }
  if (c == KEY_PREV || c == KEY_UP || c == KEY_LEFT) {
    // Advance the cursor, NOT decrement -- see FormField.h's class comment
    // for why this screen repurposes PREV's meaning.
    int num_positions = (_len < _max_len) ? (_len + 1) : _len;
    if (num_positions < 1) num_positions = 1;
    _cursor = (_cursor + 1) % num_positions;
    return true;
  }
  if (c == KEY_ENTER || c == KEY_SELECT) {
    while (_len > 0 && _buf[_len - 1] == ' ') { _len--; }  // trim trailing spaces
    _buf[_len] = 0;
    if (_set) _set(_ctx, _buf);
    _nav.pop();
    return true;
  }
  // Phase 6 (item 34): second, richer input path for lilygo_tdeck's real
  // keyboard -- InputRouter feeds typed characters straight through as a
  // plain byte (see InputRouter.cpp's LILYGO_TDECK block), so this is simply
  // unreached on every board without one. Backspace (8) and Delete (127) both
  // erase; anything else printable is typed at the cursor and the cursor
  // advances, matching how a real keyboard is expected to behave -- unlike
  // the increment-picker gestures above, which deliberately overwrite in
  // place and require a separate PREV press to move.
  if (c == 8 || c == 127) {
    if (_cursor > 0) {
      _cursor--;
      if (_cursor == _len - 1) {
        _len--;             // erasing the last character -- actually shrink the string
        _buf[_len] = 0;
      } else {
        _buf[_cursor] = ' ';  // interior erase: blank in place, same overwrite model as the picker above
      }
    }
    return true;
  }
  if (c >= 32 && c < 127) {
    _buf[_cursor] = (char)c;
    if (_cursor == _len && _len < _max_len) {
      _len++;
      _buf[_len] = 0;
    }
    // Cap at _len-1 (not _len) once the buffer is full -- with no append slot
    // left, _len itself is one past the last real character and isn't a
    // valid cursor position (matches the invariant KEY_PREV's cursor-advance
    // above already relies on: num_positions == _len, not _len+1, once full).
    int max_cursor = (_len < _max_len) ? _len : _len - 1;
    if (_cursor < max_cursor) _cursor++;
    return true;
  }
  return false;
}

// ------------------------------------------------------------ FormField spec

namespace FormField {

  void openToggleField(void* ctx) {
    ToggleFieldSpec* spec = (ToggleFieldSpec*)ctx;
    spec->field->begin(spec->title, spec->get, spec->set, spec->ctx);
    spec->nav->push(spec->field);
  }

  void openStepperField(void* ctx) {
    StepperFieldSpec* spec = (StepperFieldSpec*)ctx;
    spec->field->begin(spec->title, spec->unit, spec->get, spec->set, spec->ctx, spec->min, spec->max, spec->step, spec->decimals);
    spec->nav->push(spec->field);
  }

  void openEnumField(void* ctx) {
    EnumFieldSpec* spec = (EnumFieldSpec*)ctx;
    spec->field->begin(spec->title, spec->labels, spec->count, spec->get, spec->set, spec->ctx);
    spec->nav->push(spec->field);
  }

  void openTextField(void* ctx) {
    TextFieldSpec* spec = (TextFieldSpec*)ctx;
    spec->field->begin(spec->title, spec->get, spec->set, spec->ctx, spec->max_len);
    spec->nav->push(spec->field);
  }

}
