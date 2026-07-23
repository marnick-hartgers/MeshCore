#pragma once

#include <helpers/ui/UIScreen.h>
#include "NavStack.h"

// Four reusable field-editor widgets (PLAN.md 3.2/Phase 3), pushed by
// MenuScreen when a MenuItem's kind is Toggle/Stepper/Enum/Text
// (MenuScreen::activate(), see FormField.cpp's opener functions below).
//
// Each editor class is a single shared instance (one ToggleField, one
// StepperField, one EnumField, one TextField -- owned by UITask, matching how
// ConfirmScreen is a single shared instance reused by every caller, see
// ARCHITECTURE.md). begin() reconfigures the instance right before
// nav.push(), same "no allocation outside setup" pattern as everything else.
//
// Every settings field this phase needs (radio params, advert name, network
// toggles, etc.) is a plain C get/set function pair operating through a
// void* ctx -- no std::function, no capturing lambdas, matching the rest of
// this codebase's C-style callback idiom (e.g. Screen_Shutdown.cpp's
// shutdownAction(void* ctx)).
//
// Commit behavior is deliberately staged, not live-per-keypress: adjusting a
// Stepper/Enum/Text value only mutates a local working copy; the get/set
// callback is invoked once, on KEY_ENTER/KEY_SELECT. This avoids a
// the_mesh.savePrefs() flash write (and, for radio params, a
// radio_driver.setParams() re-tune) on every single LEFT/RIGHT press while
// the user is still scrolling through values. KEY_CANCEL discards the
// working copy without ever calling set(). Toggle is the one exception --
// PLAN.md's phase-3.md explicitly calls for "ENTER flips it immediately", and
// with only two states there's no scrolling-through-many-values cost to
// avoid, so it commits on every ENTER (matching the existing
// toggleGPS()/toggleBuzzer() precedent, which also writes prefs immediately).

typedef bool (*ToggleGetFn)(void* ctx);
typedef void (*ToggleSetFn)(void* ctx, bool value);

typedef float (*StepperGetFn)(void* ctx);
typedef void (*StepperSetFn)(void* ctx, float value);

typedef int (*EnumGetFn)(void* ctx);
typedef void (*EnumSetFn)(void* ctx, int index);

// dest is a caller-owned buffer of at least max_len+1 bytes; get() must
// null-terminate.
typedef void (*TextGetFn)(void* ctx, char* dest, int max_len);
typedef void (*TextSetFn)(void* ctx, const char* value);

class ToggleField : public UIScreen {
  NavStack& _nav;
  const char* _title;
  ToggleGetFn _get;
  ToggleSetFn _set;
  void* _ctx;

public:
  ToggleField(NavStack& nav) : _nav(nav), _title(""), _get(NULL), _set(NULL), _ctx(NULL) { }

  void begin(const char* title, ToggleGetFn get, ToggleSetFn set, void* ctx) {
    _title = title; _get = get; _set = set; _ctx = ctx;
  }

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

class StepperField : public UIScreen {
  NavStack& _nav;
  const char* _title;
  const char* _unit;       // optional suffix, e.g. "dBm" -- NULL if none
  StepperGetFn _get;
  StepperSetFn _set;
  void* _ctx;
  float _min, _max, _step;
  int _decimals;           // 0 for integer-like fields, >0 for float fields
  float _pending;          // working value, only committed on ENTER

public:
  StepperField(NavStack& nav)
    : _nav(nav), _title(""), _unit(NULL), _get(NULL), _set(NULL), _ctx(NULL),
      _min(0), _max(0), _step(1), _decimals(0), _pending(0) { }

  void begin(const char* title, const char* unit, StepperGetFn get, StepperSetFn set, void* ctx,
             float min, float max, float step, int decimals);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

class EnumField : public UIScreen {
  NavStack& _nav;
  const char* _title;
  const char* const* _labels;
  int _count;
  EnumGetFn _get;
  EnumSetFn _set;
  void* _ctx;
  int _pending;            // working index, only committed on ENTER

public:
  EnumField(NavStack& nav)
    : _nav(nav), _title(""), _labels(NULL), _count(0), _get(NULL), _set(NULL), _ctx(NULL), _pending(0) { }

  void begin(const char* title, const char* const* labels, int count, EnumGetFn get, EnumSetFn set, void* ctx);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

#define TEXT_FIELD_BUF_SIZE 32

// Character-by-character increment picker (phase-3.md: "increment-based
// picker is fine for Phase 3; real keyboard input is Phase 6"). Gesture
// mapping is deliberately NOT the obvious NEXT=inc/PREV=dec/SELECT=move-cursor
// scheme: KEY_SELECT never actually reaches any screen on the current pilot
// boards (single-button/analog/joystick's triple-click is always intercepted
// by UITask::handleTripleClick() for mute/KEY_HOME first -- see
// ARCHITECTURE.md's InputRouter section), and single-button hardware only
// ever delivers NEXT/PREV/ENTER to a screen. So cursor movement is bound to
// KEY_PREV here (screen-local reinterpretation of PREV, same precedent as
// MenuScreen using PREV for "move up" and leaf screens aliasing it to
// "cancel" -- meaning is always screen-defined, not universal):
//   KEY_NEXT/DOWN/RIGHT -- increment the character at the cursor (wraps)
//   KEY_PREV/UP/LEFT    -- advance the cursor one position (wraps through the
//                          append slot back to 0); does NOT decrement
//   KEY_ENTER/SELECT    -- trim trailing spaces, commit, pop
//   KEY_CANCEL          -- discard, pop (joystick/rotary only -- single-button
//                          boards have no cancel here; KEY_HOME from three
//                          steps up is the escape hatch, same as Contacts/
//                          Channels, see ARCHITECTURE.md)
// To shorten a string, cycle trailing characters back to space (index 0 of
// the charset) and commit -- trailing spaces are trimmed automatically.
class TextField : public UIScreen {
  NavStack& _nav;
  const char* _title;
  TextSetFn _set;
  void* _ctx;
  int _max_len;                     // <= TEXT_FIELD_BUF_SIZE - 1
  char _buf[TEXT_FIELD_BUF_SIZE];
  int _len;
  int _cursor;                      // 0.._len, where _cursor==_len is the append slot

public:
  TextField(NavStack& nav)
    : _nav(nav), _title(""), _set(NULL), _ctx(NULL), _max_len(TEXT_FIELD_BUF_SIZE - 1), _len(0), _cursor(0) {
    _buf[0] = 0;
  }

  void begin(const char* title, TextGetFn get, TextSetFn set, void* ctx, int max_len);

  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

// Shared spec structs + opener functions, so MenuScreen::activate() can push
// the right (single, shared) field-editor instance from a generic switch on
// MenuItem::kind without knowing anything about any specific settings field.
// A MenuItem of kind Toggle/Stepper/Enum/Text sets action_ctx to point at one
// of these specs (owned by the settings screen that built the row) instead of
// pointing at a UIScreen* the way Submenu's `submenu` field does -- the specs
// bundle which shared field-editor instance + NavStack to push into, plus the
// per-field get/set callbacks and range/label data.
namespace FormField {

  struct ToggleFieldSpec {
    NavStack* nav;
    ToggleField* field;
    const char* title;
    ToggleGetFn get;
    ToggleSetFn set;
    void* ctx;
  };

  struct StepperFieldSpec {
    NavStack* nav;
    StepperField* field;
    const char* title;
    const char* unit;
    StepperGetFn get;
    StepperSetFn set;
    void* ctx;
    float min, max, step;
    int decimals;
  };

  struct EnumFieldSpec {
    NavStack* nav;
    EnumField* field;
    const char* title;
    const char* const* labels;
    int count;
    EnumGetFn get;
    EnumSetFn set;
    void* ctx;
  };

  struct TextFieldSpec {
    NavStack* nav;
    TextField* field;
    const char* title;
    TextGetFn get;
    TextSetFn set;
    void* ctx;
    int max_len;
  };

  // MenuActionFn-shaped (void (*)(void* ctx)) -- ctx must point at the
  // matching *FieldSpec above. Defined once in FormField.cpp, called from
  // MenuScreen::activate() for every Toggle/Stepper/Enum/Text row on every
  // settings screen -- no per-field or per-screen opener boilerplate needed.
  void openToggleField(void* ctx);
  void openStepperField(void* ctx);
  void openEnumField(void* ctx);
  void openTextField(void* ctx);

}
