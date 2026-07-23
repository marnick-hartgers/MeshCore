# ui-forest — as-built architecture

Describes the code as it exists after Phase 8 (the last phase in `PLAN.md`'s roadmap), not the
aspirational design in `PLAN.md`. Where this file and `PLAN.md` disagree, this file wins for
"what's actually there today" — `PROGRESS.md` explains why, when the difference was a deliberate
call. Read this before touching any screen so new code plugs into the existing shape instead of
reinventing it.

## Entry point contract

`UITask` (`UITask.h`/`.cpp`) is the only class `examples/companion_radio/main.cpp` touches
directly:

```cpp
UITask ui_task(&board, &serial_interface);
...
ui_task.begin(disp, &sensors, the_mesh.getNodePrefs());
...
ui_task.loop();
```

Everything else `MyMesh` needs goes through the `AbstractUITask*` base pointer
(`msgRead`/`newMsg`/`notify`/`loop`/`hasConnection`/etc. — see `examples/companion_radio/
AbstractUITask.h`), which `main.cpp` passes to `MyMesh`'s constructor separately. This means
swapping `-I examples/companion_radio/ui-forest` for `ui-new`/`ui-tiny`/`ui-orig` in a
board's `platformio.ini` is the entire integration surface; nothing else in the repo needs
to know which UI variant is compiled in.

## Key vocabulary

Every class speaks the `KEY_*` codes from `src/helpers/ui/UIScreen.h` verbatim:
`KEY_LEFT/UP/DOWN/RIGHT/SELECT/ENTER/CANCEL/HOME/NEXT/PREV/CONTEXT_MENU`. No ui-forest file
invents a new event type. `UIScreen::render(DisplayDriver&)` returns "millis until next
render"; `handleInput(char c)` returns whether it consumed the key; `poll()` is called every
loop iteration regardless of input, for time-driven state (splash dismiss timer, confirm
countdown).

## Render loop (`UITask::loop()`)

```
c = InputRouter::poll(*this)                      -- one gesture -> one key code, or 0
                                                      (side effects like display-wake and
                                                      CLI-rescue may already have consumed it)
if c == KEY_HOME:  NavStack::popToRoot(); c = 0    -- Phase 2: intercepted here, once, before
                                                      dispatch -- see "KEY_HOME" below
if c != 0:  NavStack::current()->handleInput(c)  -- screen may push/pop/mutate itself
userLedHandler(); buzzer.loop()                   -- time-driven, every iteration
NavStack::current()->poll()                      -- time-driven screen state, every iteration
if display on:
  showing_splash = (NavStack::current() == splash)
  if !showing_splash: updateStatusBar()           -- rebuild scrolling text + battery/mute state
  if content_due || status_due || toast.isShowing():
    display->startFrame()                           -- every backend fully clears/fills here (Phase 4
                                                        finding -- see "Known gaps"), so everything
                                                        that should be visible must be redrawn below,
                                                        not just whatever triggered this pass
    NavStack::current()->render(display)          -- the active screen draws itself
    if !showing_splash: StatusBar::render(display) -- composited on top, every pass (not just
                                                        status_due ones -- Phase 4 fix, see below)
    ToastOverlay::composite(display)                -- composited on top of that
    display->endFrame()
  auto-off / KEEP_DISPLAY_ON_USB check
vibration.loop()
AUTO_SHUTDOWN_MILLIVOLTS low-battery check
```

`NavStack::current()` is re-fetched after `handleInput`/`poll` rather than cached once at
the top of `loop()`, because either call can change the stack (e.g. `Screen_Splash::
handleInput` calls `nav.reset(_home)`) and rendering must reflect the *new* top screen the
same frame.

`StatusBar` is suppressed while `Screen_Splash` is the current screen, identified by pointer
equality against `UITask`'s own `_splash` member -- splash draws its logo across the same top
rows the status bar strip occupies, so both would overlap otherwise. Every other screen
reserves `Layout::statusBarHeight(true)` at the top of its own layout and draws below it.

**Compositing and `status_due` (Phase 4 fix):** every `DisplayDriver` backend's `startFrame()`
does a full clear/fill of the screen buffer, not a partial update -- confirmed by reading all ten
backend `.cpp` files (`SH1106`/`SSD1306`/`LGFX`/`GxEPD`/`E290`/`E213`/`NV3001B`/`ST7735`/`ST7789`/
`ST7789LCD`), not assumed. Because of that, `_status_bar.render()` must run on *every* pass through
the compositing block, not only the passes `status_due` itself triggered -- otherwise a
content-only refresh (the current screen's own ~1000ms timer) or a toast-only one wipes the status
bar's pixels via `startFrame()` and never redraws them, producing a real blank-status-bar frame.
This was a real device-reported bug ("status bar sometimes flashes") on `WioTrackerL1`; fixed by
gating the status bar draw on `!showing_splash` alone. `status_due` (from `StatusBar::needsRedraw()`)
still does its original job of deciding whether the block runs *at all* when nothing else would
have triggered it (e.g. mid-marquee-scroll with no content change) -- it just isn't the gate for
whether the bar draws once something else already decided to redraw.

### `KEY_HOME` (jump-to-root)

Phase 2 wires `KEY_HOME` (PLAN.md 3.1/item 16) as a single interception in `UITask::loop()`,
right after `InputRouter::poll()` and before dispatch to the current screen -- exactly the "handled
once in NavStack, not per individual screen" placement PLAN.md calls for. No screen's
`handleInput()` ever sees `KEY_HOME` itself.

It's emitted by `UITask::handleTripleClick()`, not by a new `InputRouter` gesture: PLAN.md 3.1's
suggested mechanism ("a long-press-anywhere ... maps to KEY_HOME") isn't available as-built,
because long-press is already `KEY_ENTER` on every board scheme (single-button's *only* activate
gesture) as of Phase 1 -- repurposing it would break activation entirely on single-button pilots.
Triple-click was the one gesture with room: `checkDisplayOn(c)` still runs for its display-wake
side effect, then `handleTripleClick` checks `_nav.depth()` --

- `depth() == 1` (at Home): unchanged from Phase 1, toggles the buzzer (nothing to jump home
  from at the root).
- `depth() > 1` (drilled into any submenu/list/detail screen): returns `KEY_HOME` instead of
  toggling the buzzer; `UITask::loop()`'s interception turns that into `_nav.popToRoot()`.

This one method is the single call site for triple-click on *every* board scheme (single-button,
analog, and joystick's `back_btn` all route through it -- see the `InputRouter` gesture table
below), so this covers all three pilot boards without any `InputRouter` changes. It also happens
to be the only escape mechanism single-button/analog/rotary boards have out of a list-type
screen (`Screen_Contacts`/`Screen_Channels`/`Screen_ContactDetail`): those boards have no raw
`KEY_CANCEL` source at all (only joystick's dedicated `back_btn` emits one), and Phase 1's
"leaf screens alias `KEY_PREV` to cancel" trick doesn't work for screens where `KEY_PREV` has to
keep meaning "move up/scroll" -- see PROGRESS.md's Phase 2 "Decisions" section for the full
reasoning.

## Class map

### `NavStack` (header-only)

Fixed array of 8 `UIScreen*`. `push()`/`pop()` move pointers only — screens are constructed
once in `UITask::begin()` (matches the project's "no allocation outside setup" rule) and
reused for the process lifetime. `pop()` refuses to empty the stack (always leaves depth
≥ 1). `reset(UIScreen* root)` replaces the entire stack with a single root — used once, by
`Screen_Splash`, to swap splash out for Home without leaving splash reachable via CANCEL.

### `InputRouter`

One `poll(UITask& task)` method, called once per `UITask::loop()` iteration, returning a
single `KEY_*` char (or 0). Internally branches on the same board `#ifdef`s `ui-new`'s
`UITask::loop()` used to branch on inline (`UI_HAS_JOYSTICK` / `PIN_USER_BTN` /
`UI_HAS_ROTARY_INPUT` / `PIN_USER_BTN_ANA`), but centralized so screens and `UITask` never
see a board `#ifdef`.

**Phase 1 addition: `poll()` takes a `UITask&` and calls back into it.** ui-new wraps raw
button events in helper calls right where they're detected --
`checkDisplayOn()`/`handleLongPress()`/`handleDoubleClick()`/`handleTripleClick()`
(`examples/companion_radio/ui-new/UITask.cpp:713-785`, methods at :862-894) -- because only
at that point does the code still know *which gesture* produced a given key. A joystick
click and a single-button long-press can both resolve to `KEY_ENTER`, but only the long-press
should trigger boot-time CLI rescue, and only a click when the display was off should be
silently swallowed. Once collapsed to a bare `char`, that distinction is gone. So `InputRouter`
calls these as public `UITask` methods (they still own the state: `_display`, `_auto_off`,
`ui_started_at`, `buzzer`) at exactly the call sites ui-new wraps:

- `task.checkDisplayOn(KEY_X)` -- wraps every click-type event across all board schemes
  (single-button click, joystick user/left/right/up/down clicks, analog click). If the
  display was off, turns it on and returns `0` (consumes the press); otherwise extends the
  auto-off deadline and returns the key unchanged.
- `task.handleLongPress(KEY_X)` -- wraps every long-press event. If within 8s of boot, calls
  `the_mesh.enterCLIRescue()` and returns `0`; otherwise returns the key unchanged.
- `task.handleDoubleClick(KEY_PREV)` -- single-button and analog-button only (joystick has no
  double-click mapping). Just re-applies the display-wake side effect (matches ui-new, including
  its quirk of discarding `checkDisplayOn`'s return value -- ported as-is).
  `task.handleTripleClick(KEY_SELECT)` -- single-button, analog-button, **and** joystick's
  `back_btn` (all three route through this one method). Phase 1: always toggled the buzzer and
  always consumed the event (`c = 0`), never reaching `NavStack::current()->handleInput()` on
  any board -- ui-new's real behavior, not the more hedged "screen-dependent SELECT" wording in
  PLAN.md §3.1 (see PROGRESS.md's Phase 1 "Decisions" section). Phase 2 makes this
  context-sensitive (mute at Home, `KEY_HOME` below it) -- see the "`KEY_HOME`" section above.

A `#if defined(HAS_TORCH)` branch (back-button double-click toggles a physical torch, single
click maps to `KEY_CANCEL`) is also ported, compiled-but-unverified since no Phase 1 pilot
defines `HAS_TORCH`.

Gesture vocabulary as implemented:

| Board scheme | Physical input | Key emitted |
|---|---|---|
| Single button (`PIN_USER_BTN`) | click | `KEY_NEXT` |
| | double-click | `KEY_PREV` |
| | triple-click | `KEY_SELECT` |
| | long-press | `KEY_ENTER` |
| Analog button (`PIN_USER_BTN_ANA`) | same 4 gestures | same 4 keys (throttled to a 10ms poll interval, only checked if no other input already produced a key this frame) |
| Joystick (`UI_HAS_JOYSTICK`) | `user_btn` click | `KEY_ENTER` |
| | `joystick_left` click | `KEY_LEFT` |
| | `joystick_right` click | `KEY_RIGHT` |
| | `joystick_up` click *(only if board defines `JOYSTICK_UP`/`JOYSTICK_DOWN`)* | `KEY_UP` |
| | `joystick_down` click *(ditto)* | `KEY_DOWN` |
| | `back_btn` click | `KEY_CANCEL` |
| Rotary (`UI_HAS_ROTARY_INPUT`) | encoder turn | `KEY_NEXT` / `KEY_PREV` (only consulted if the button path above didn't already produce a key this frame) |

Display-power-state gating and CLI-rescue interception on long-press are wired as of Phase 1
(see the "Phase 1 addition" note above) -- both needed mesh/board state Phase 0 deliberately
had zero dependency on.

**Single-button/rotary "back" gap:** none of `NEXT`/`PREV`/`SELECT`/`ENTER` include a
cancel/back gesture (ui-new never needed one -- no screen stack, just a flat page ring).
Resolved in Phase 1 not in `InputRouter` but at the screen level: every Phase 1 leaf content
screen treats `KEY_PREV` as equivalent to `KEY_CANCEL` (both pop the nav stack), since a leaf
screen has no internal list for PREV to mean "move up" anyway. `MenuScreen` is unaffected --
`KEY_PREV` still moves its selection cursor there. See PROGRESS.md for the full rationale.

**Adding a new joystick board with real up/down pins:** just declare `JOYSTICK_UP`/
`JOYSTICK_DOWN` pin macros in the board's `variant.h`, wire `joystick_up`/`joystick_down`
`MomentaryButton`s in its `target.cpp` (see `variants/wio-tracker-l1/target.cpp` for the
pattern), and declare them `extern` in `target.h`. `InputRouter` picks them up automatically
via the `#if defined(JOYSTICK_UP) && defined(JOYSTICK_DOWN)` guard — no ui-forest changes
needed.

**Phase 6 additions: touch (`sensecap_indicator-espnow`) and keyboard+trackball
(`lilygo_tdeck`).** Both follow the same "extra input source, only consulted if nothing higher-
priority already produced a key this poll" precedence the rotary/analog/torch blocks already
established — see PROGRESS.md's Phase 6 section for the confidence levels behind each (touch is
verified-by-reading-code; the T-Deck pin/protocol assumptions are not verified against any
schematic in this repo):

- `#if defined(HAS_TOUCH)` — polls `task.getTouch(&x, &y)` (a thin passthrough to the
  `DisplayDriver*` `UITask` already holds, via the new `DisplayDriver::getTouch()` virtual —
  default `false`, overridden in `LGFXDisplay`) every poll() regardless of whether a button
  already won this frame, since a tap's touch-down and touch-up can both land between two
  polls otherwise. Tracks touch-down/last-point/touch-up as `InputRouter` instance state
  (`_touch_active`/`_touch_start_*`/`_touch_last_*`); on touch-up, classifies the total
  start-to-end delta as a tap (`KEY_ENTER`, both axes under `TOUCH_SWIPE_THRESHOLD`=12px) or a
  swipe on whichever axis moved more (right/up = `KEY_NEXT`, left/down = `KEY_PREV`). Purely
  additive on top of this board's existing `PIN_USER_BTN=38` fallback button.
- `#if defined(LILYGO_TDECK)` — polls four `MomentaryButton`s (`trackball_up/down/left/right`,
  declared in `target.h`/`.cpp`, multiclick disabled so each trackball pulse becomes one key
  immediately) for `KEY_UP/DOWN/LEFT/RIGHT`, then polls `tdeck_keyboard.poll()` (throttled to
  20ms, same shape as `PIN_USER_BTN_ANA`'s ADC throttling) and returns whatever raw ASCII byte
  it reports, unchanged, as the "key" code. See "`FormField` (Toggle/Stepper/Enum/Text
  editors)" below for how `TextField` is the only screen that interprets that raw byte.

**Dynamic control hints (item 32).** `InputRouter` also exposes three static, stateless
helpers with no `poll()` dependency at all — `activateHint()` (label for the gesture that
performs `KEY_ENTER`/`KEY_SELECT` on this board: "long press"/"tap"/"click"/"press Enter"/
"press"), `moveHint()` (label for `KEY_NEXT`/`KEY_PREV` list movement: "click/dbl-click"/
"swipe"/"trackball"/"stick"/"turn"), and `textEntryHint(buf, size)` (the two-clause line
`FormField`'s `TextField` shows, since keyboard boards need a real second sentence, not just a
relabeled gesture). These replace ui-new's `PRESS_LABEL` macro (a single compile-time `#if
UI_HAS_JOYSTICK`/`#else`, ported verbatim into `Screen_Bluetooth`/`Advert`/`Shutdown` in Phase
1) and the literal button-press wording `FormField`'s three interactive editors used to draw.
Every board macro `InputRouter::poll()` already checks is also what these three functions
switch on, so adding a new control scheme's gesture-to-key mapping and its hint label happen in
the same file, in one place.

### `Layout` (header-only)

`DisplayDriver` doesn't expose a font-height accessor, so `Layout::rowHeight(textSize)`
hardcodes `textSize * 8 + 3`, matching the row spacing `ui-new` already uses by convention
for text size 1 (`y += 11` in its RECENT page). `headerHeight()` = `rowHeight(1)`.
`statusBarHeight(bool showing)` = `rowHeight(1)` or 0. `visibleRows(display, statusBarShowing,
textSize)` = `(display.height() - headerHeight() - statusBarHeight(showing)) / rowHeight(textSize)`,
floored to 1. Every list-style screen should compute its visible row count and header
position through these, never a literal pixel constant — this is what lets the same
`MenuScreen` code show ~4 rows on a 64px OLED and ~10+ on a 240px TFT.

### `MenuScreen` + `MenuItem` / `MenuItemKind`

The one generic list-menu widget everything menu-shaped will be built from (Home, Settings,
Diagnostics, Contacts, Channels, every settings sub-menu). Constructor takes a `NavStack&`
(to push/pop), a `ToastOverlay&` (only used by `Action` items that want to show one),
a title, a `MenuItem*` array + count, and a `status_bar_shown` bool.

```cpp
struct MenuItem {
  const char* label;
  const uint8_t* icon;      // optional 16x16 XBM, NULL if none
  MenuItemKind kind;
  MenuActionFn action;      // used when kind == Action
  void* action_ctx;
  UIScreen* submenu;        // used when kind == Submenu
};
```

`MenuItemKind` has 7 values today, 6 of them fully wired as of Phase 3: `Action` invokes the
callback; `Submenu` pushes `item.submenu`; `Toggle`/`Stepper`/`Enum`/`Text` push a shared
`FormField` editor instance by calling `FormField::openToggleField()` / `openStepperField()` /
`openEnumField()` / `openTextField()` with `item.action_ctx` — for these four kinds,
`action_ctx` points at a `FormField::*FieldSpec` (built by whichever settings screen owns the
row), not at a `UIScreen*` the way `Submenu`'s `submenu` field does. `MenuScreen.cpp` never
needs to know about any specific settings field as a result — see `FormField`'s section below.
`Info` is still a placeholder for Phase 4 stat rows.

`handleInput()` treats `KEY_NEXT`/`KEY_DOWN`/`KEY_RIGHT` as "move selection down" and
`KEY_PREV`/`KEY_UP`/`KEY_LEFT` as "move up" — this is deliberate, not an oversight: joystick
boards only have a horizontal axis (or, per the `wio-tracker-l1` fix, both axes), and
single-button/rotary boards only ever emit NEXT/PREV, so folding all four directional keys
into the same two logical actions lets one `handleInput` implementation serve every input
scheme without per-scheme branching. `KEY_ENTER`/`KEY_SELECT` activate the current item;
`KEY_CANCEL` pops the nav stack.

`render()` draws a one-row title/header (offset by `Layout::statusBarHeight()` if
`status_bar_shown`), a separator line, then up to `Layout::visibleRows()` item rows with
scroll-into-view (adjusts `_scroll_offset` so the selected index is always drawn). No
allocation — the `MenuItem*` array is expected to be a `static const`-ish table built once
(see `UITask::begin()`'s placeholder array for the pattern), not rebuilt per frame.

**Instantiating a submenu:** build a `static` (or `UITask`-member) `MenuItem[]` table, set
`kind = MenuItemKind::Submenu` and `submenu = &someOtherMenuScreenInstance` on the item that
should drill in, construct that target `MenuScreen` once in `UITask::begin()` same as any
other screen, and `handleInput`'s `activate()` path will `nav.push()` it for free.

**Instantiating a settings field (Phase 3):** build a `FormField::StepperFieldSpec` (or
`Toggle`/`Enum`/`Text`FieldSpec) as a member of the owning settings screen, filling `nav`/`field`
to point at `UITask`'s shared editor instance and `NavStack`, `get`/`set` to a pair of `static`
member functions on that settings screen (signature `XxxGetFn`/`XxxSetFn` from `FormField.h`,
`ctx` almost always `this`), and the rest to the field's title/range/labels. Set the `MenuItem`'s
`kind` to match (`Stepper`, say) and `action_ctx = &thatSpec` — leave `action`/`submenu` NULL,
they're unused for these four kinds. See any `Screen_SettingsXxx.cpp` constructor for the pattern
repeated across every field; every one of them looks the same.

### `FormField` (Toggle/Stepper/Enum/Text editors — Phase 3)

Four small `UIScreen` subclasses (`ToggleField`, `StepperField`, `EnumField`, `TextField`) plus a
`namespace FormField` of spec structs and opener functions, all in `FormField.h`/`.cpp`. Each
class is a **single shared instance** — one of each, owned by `UITask` (`_toggle_field`,
`_stepper_field`, `_enum_field`, `_text_field`), reused across every settings screen exactly like
`ConfirmScreen` already is. `begin()` reconfigures the instance (title, get/set callbacks,
range/labels) immediately before `nav.push()`; nothing about these classes is per-field state
that would need one instance per field.

Commit is staged, not applied per keypress: `Stepper`/`Enum`/`Text` only mutate a local working
value (`_pending`/cursor state) while `KEY_NEXT`/`KEY_PREV`/etc. are pressed, and call the `set()`
callback exactly once, on `KEY_ENTER`/`KEY_SELECT`. `KEY_CANCEL` pops without ever calling `set()`.
This matters specifically for radio params — without staging, scrolling through frequency values
would call `radio_driver.setParams()` (and `the_mesh.savePrefs()`, a flash write) on every single
adjustment. `Toggle` is the one exception: it commits on every `KEY_ENTER` press, per
phase-3-settings.md's literal wording and because a two-state field has no "scrolling through many
values" cost to stage against.

`TextField`'s gesture mapping deliberately isn't the obvious NEXT=increment/PREV=decrement/
SELECT=move-cursor scheme — `KEY_SELECT` never actually reaches any screen on the current pilot
boards (see `InputRouter`'s gesture table below: single-button/analog/joystick's triple-click is
always intercepted by `UITask::handleTripleClick()` for mute/`KEY_HOME` first), and single-button
hardware only ever delivers `KEY_NEXT`/`KEY_PREV`/`KEY_ENTER` to a screen at all. So `KEY_PREV` is
repurposed here to mean "advance the cursor" instead of "decrement" — see `FormField.h`'s
`TextField` class comment for the full gesture table and reasoning; this is the same
"a screen can give `KEY_PREV` its own meaning" precedent `MenuScreen` (move-up) and the Phase 1
leaf screens (cancel/back) already established, extended to a third meaning.

`MenuScreen::activate()` is the only caller of the four `FormField::openXxxField()` functions —
see that class's section above for the `MenuItem`/spec wiring.

**Phase 6 addition: `TextField` gained a second, additive input path.** `handleInput()`
now also accepts a raw byte in `[32,127)` (typed at the cursor, cursor advances — capped once
the buffer is full, see PROGRESS.md's Phase 6 "Real bug found and fixed") and `8`/`127`
(backspace/delete). This is exactly the byte `InputRouter`'s `LILYGO_TDECK` block feeds through
unchanged from `lilygo_tdeck`'s keyboard co-processor — no other screen interprets a raw
printable byte, so this is a no-op everywhere else by construction. The increment-picker
gestures above are untouched and still reachable via the same board's trackball-click button;
this is a second path, not a replacement, per phase-6.md's own framing.

### `ConfirmScreen`

See `PROGRESS.md`'s "Deviations" section for why this doesn't poll raw button state like
`ui-new`'s shutdown flow does. Usage shape:

```cpp
confirmScreen.begin("Erase all data?", "This cannot be undone", myCallback, myCtx, 5000);
nav.push(&confirmScreen);
```

`render()` shows title/message/countdown; `poll()` fires `on_confirm(ctx)` and pops once the
deadline passes; `KEY_CANCEL` pops without firing. First real caller as of Phase 1:
`Screen_Shutdown` arms it with a 3-second countdown and a callback that calls
`UITask::shutdown()`. Still the single shared instance described in PLAN.md 3.2 — Phase 3's
`Screen_SettingsDanger` (3 actions) and every settings screen's Restore Defaults action are its
second/third+ real callers, all reusing the same `UITask::_confirm` object rather than
constructing their own, confirming the single-shared-instance design holds up with multiple
callers.

### `ToastOverlay` (header-only)

`show(text, duration_millis)` arms a `millis()` expiry; `composite(display)` draws a
bordered box centered vertically (`display.height()/3`) over whatever the current screen
just rendered, if still within the expiry window. Called unconditionally at the end of
`UITask::loop()`'s render block — it's a no-op draw when nothing is showing.

### `StatusBar`

Ported from `ui-tiny/ScrollingStatusBar.h`'s change-detection + marquee-scroll-if-too-wide
logic, generalized to take an arbitrary pre-built string via `setText(display, text)` instead
of building the string itself from watched fields. `UITask::updateStatusBar()` calls
`setText()` every `loop()` iteration (while the display is on and splash isn't showing) with a
freshly-`snprintf`'d string in `ui-tiny`'s format (`name | BUZ:x | GPS:x | BLE:x`) -- the
change-detection inside `setText()` (`strcmp` against the last string) means calling it every
frame is cheap and correct, it just early-returns if nothing changed.

**Phase 1 addition: `setBattery(milliVolts, muted)` + a drawn battery gauge.** Ported from
`ui-new`'s `HomeScreen::renderBatteryIndicator()`
(`examples/companion_radio/ui-new/UITask.cpp:112-150) -- a rect-based percentage gauge (using
`BATT_MIN_MILLIVOLTS`/`BATT_MAX_MILLIVOLTS`) plus an 8x8 muted-icon overlay when the buzzer is
quiet (`#ifdef PIN_BUZZER` gated, exactly like ui-new, so boards without a buzzer never show a
permanently-"muted" icon even though `UITask::isBuzzerQuiet()` reports `true` when there's no
buzzer at all). Drawn top-right, after the scrolling text, every `render()` call, so it's
never painted over. Moving this into `StatusBar` (rather than duplicating it per-screen the
way `ui-new` did on its `HomeScreen`) means every Phase 1 screen gets it for free.
`needsRedraw()` now also returns `true` whenever battery/mute state changed, independent of
the text-scroll timer.

### `Screen_Splash`

Takes `NavStack&` + a `UIScreen* home` pointer at construction. Shows the meshcore logo +
version + build date for `BOOT_SCREEN_MILLIS` (3000ms default), then `poll()` calls
`nav.reset(home)`. `handleInput()` (any key) does the same reset immediately, so any input
dismisses it early. `Screen_MsgPreview` (below) is the only other Phase 1 caller of
`nav.reset()` -- everything else uses `push()`/`pop()`.

### Phase 1 leaf content screens (`Screen_Status`/`Recents`/`RadioInfo`/`Bluetooth`/`Advert`/
`Gps`/`Sensors`/`Shutdown`)

Eight screens, each a straight port of one `HomePage` case from
`examples/companion_radio/ui-new/UITask.cpp`'s `HomeScreen`, now split into its own file and
reachable as a `MenuItemKind::Submenu` entry off Home instead of a swipe-page (PLAN.md item
14's intentional restructuring). Common shape:

- Constructor takes `NavStack&` (to pop back to Home) plus whatever else it needs: a
  `UITask*` for screens that call back into task state (`Screen_Status`, `Bluetooth`,
  `Advert`, `Gps`, `Sensors`, `Shutdown`), `NodePrefs*` for `Screen_RadioInfo`, nothing extra
  for `Screen_Recents`. Screens needing `UITask*` forward-declare `class UITask;` in their own
  header and only `#include "UITask.h"` in the `.cpp`, to avoid a circular include (`UITask.h`
  includes every `Screen_*.h` to declare its own member pointers).
- `render()` reserves `Layout::statusBarHeight(true)` at the top (the persistent `StatusBar`
  is drawn separately by `UITask::loop()`, on top of whatever the screen draws) and otherwise
  keeps ui-new's original pixel layout below that -- these are fixed-content screens, not
  adaptive lists, so `Layout` isn't used for anything beyond that one top offset (contrast
  with `MenuScreen`, which does use `Layout::visibleRows()`/`rowHeight()` throughout).
  Full layout-driven polish is Phase 5's job (PLAN.md item 15/31), not this phase's.
- `handleInput()` treats both `KEY_CANCEL` and `KEY_PREV` as "pop back to Home" -- see the
  "single-button/rotary back gap" note under `InputRouter` above for why `KEY_PREV` is
  included. `KEY_ENTER`/`KEY_SELECT` trigger whatever the screen's primary action is
  (`Bluetooth` toggles serial, `Advert` sends an advert + toasts the result, `Gps` toggles
  GPS, `Sensors` also toggles GPS -- ui-new's real, not-a-typo behavior, see PROGRESS.md --
  `Shutdown` arms `ConfirmScreen`). `Screen_Recents`/`Screen_RadioInfo` have no primary action,
  only the back gesture.
- `Screen_Gps`/`Screen_Sensors` are `#if ENV_INCLUDE_GPS == 1`/`#if UI_SENSORS_PAGE == 1`
  gated in `UITask.h`/`.cpp` (construction and Home-menu wiring), but their own `.cpp` files
  have no such guard and compile unconditionally on every board -- see PROGRESS.md's
  "Decisions" section on why (PLAN.md's flat-file-layout choice in §4).

### `Screen_Contacts` / `Screen_ContactDetail` / `Screen_Channels` (Phase 2)

The first real data-driven list screens, and the first screens (besides `MenuScreen` itself)
to use `Layout::visibleRows()` for scroll-into-view rather than a fixed pixel layout. All three
read through `the_mesh` on demand by index every `render()` call -- never caching more than one
`ContactInfo`/`ChannelDetails` at a time (PLAN.md 3.6).

- **`Screen_Contacts`** -- `NavStack&` + a `Screen_ContactDetail*` at construction (constructed
  first in `UITask::begin()` so its pointer exists in time). `render()` walks
  `the_mesh.getNumContacts()`/`getContactByIdx()` for only the rows in the current
  `Layout::visibleRows()` window, with the same scroll-into-view math `MenuScreen` uses
  (`_selected`/`_scroll_offset`, clamped to `[0, count)` every render in case the contact table
  shrank underneath a stale selection). `KEY_NEXT`/`KEY_DOWN`/`KEY_RIGHT` and
  `KEY_PREV`/`KEY_UP`/`KEY_LEFT` move the cursor (mod `count`, guarded against `count == 0`).
  `KEY_ENTER`/`KEY_SELECT` calls `_detail->show(selected_idx)` then `nav.push(_detail)`.
  `KEY_CANCEL` pops back to Home. No `KEY_PREV`-as-cancel aliasing here (unlike the Phase 1 leaf
  screens) -- `KEY_PREV` has a real job in a list screen (move the cursor up), so only
  `KEY_CANCEL` pops, matching `MenuScreen`'s own convention.
- **`Screen_ContactDetail`** -- holds only a `NavStack&` and an `int _idx` set via `show(idx)`
  right before `nav.push()`; re-reads `the_mesh.getContactByIdx(_idx, contact)` fresh on every
  `render()`, so it can never go stale relative to the underlying contact table across renders.
  Renders the contact name as a `MenuScreen`-style header (ellipsized, separator line below),
  then a `Layout::visibleRows()`-scrolled list of five read-only fields (Type, Path, Seen, GPS,
  ID) below it -- see PROGRESS.md's Phase 2 "Decisions" for why this needed to be scrollable
  rather than a fixed `y += 11` layout like `Screen_RadioInfo` (six rows of content don't fit a
  64px OLED's ~4 visible rows at once). `Type` comes from a plain `switch` over `ADV_TYPE_*`
  (small enough enum that no helper class is needed, per the phase-2.md open question). `ID` is
  a 6-byte hex prefix of `contact.id.pub_key` (`mesh::Utils::toHex(..., 6)`), matching the
  `pubkey_prefix` convention used elsewhere in the codebase, not the full 32-byte key.
  `KEY_NEXT`/`KEY_PREV` scroll the field list; `KEY_CANCEL` **and** `KEY_ENTER`/`KEY_SELECT` both
  pop back to `Screen_Contacts` (there's no primary action on a read-only screen for ENTER to do,
  so it doubles as an obvious "go back" for single-button testers).
- **`Screen_Channels`** -- `NavStack&` only, no detail screen (PLAN.md scopes channel
  editing/detail as a stretch goal, out of Phase 2). `the_mesh.getChannel(idx, ChannelDetails&)`
  has no "count populated slots" API of its own, so this screen keeps its own small static
  helpers (`countPopulatedChannels()`, `findNthPopulatedChannel()`) that linear-scan
  `0..MAX_GROUP_CHANNELS-1`, skipping `name[0] == 0` slots, to translate between "position in the
  visible, non-empty-only list" (what `_selected`/`_scroll_offset` track, same shape as
  `Screen_Contacts`) and "raw channel slot index" (what `getChannel()` takes). Cheap given every
  pilot env sets `MAX_GROUP_CHANNELS=40`; each lookup only ever holds one `ChannelDetails` on the
  stack. Same input vocabulary as `Screen_Contacts` minus the ENTER/push-detail behavior.

### `Screen_Settings` / `Screen_SettingsRadio` / `Screen_SettingsAdvert` / `Screen_SettingsNetwork` / `Screen_SettingsDevice` / `Screen_SettingsDanger` (Phase 3)

Six thin `MenuScreen` subclasses, not six bespoke `UIScreen` implementations — every settings
screen IS exactly generic list-of-labeled-rows behavior, so each one's `render()`/`handleInput()`
is inherited unchanged from `MenuScreen`; the subclass only exists to own a `MenuItem _rows[N]`
array, the `FormField::*FieldSpec` structs those rows point at, and the `static` get/set/action
callback functions the specs reference. This is the same relationship `Screen_Splash` has to
`UIScreen` directly, just one level deeper in the hierarchy, and it's what makes `MenuScreen`
live up to ARCHITECTURE.md's own description of it as "the single generic widget that implements
Home, Settings, Diagnostics, Contacts list, Channels list, and every settings sub-menu."

- **`Screen_Settings`** — the root menu, `nav`/`toast` only (no `NodePrefs*`, no callbacks of its
  own); constructor takes the five sub-screens as `UIScreen*` and builds 5 `Submenu` rows.
- **`Screen_SettingsRadio`** — Frequency/Bandwidth/Spreading Factor/Coding Rate/TX Power +
  Restore Defaults. Every field's setter writes `_node_prefs` **and** calls
  `radio_driver.setParams()` (freq/bw/sf/cr — all four together, since that's how the real
  `CMD_SET_RADIO_PARAMS` wire command applies them) or `radio_driver.setTxPower()` (TX power,
  a separate wire command in the real protocol, called separately here too). Bandwidth is an
  `Enum` over the 10 standard LoRa steps (7.8–500 kHz) rather than a free-form `Stepper` — the
  radio hardware only supports discrete values even though the wire protocol's own validation
  doesn't enforce that. Frequency's setter also calls a private `checkRepeatStillValid()` helper
  that silently turns "Repeat" (`client_repeat`) back off if the new frequency can't support it
  (`the_mesh.isValidClientRepeatFreq()`), since Repeat and Frequency are edited on separate screens
  and can't be validated together the way one atomic wire command would.
- **`Screen_SettingsAdvert`** — Name (`Text`) + Share Location (`Toggle`) + Restore Defaults.
  Name's setter mirrors `CMD_SET_ADVERT_NAME`'s handler exactly (length-truncate to 31 chars, no
  character allowlist). This is the one and only place `node_name` is edited — see its header
  comment for why `Screen_SettingsDevice` doesn't duplicate a second "name" field.
- **`Screen_SettingsNetwork`** — Repeat, RX Boost, Telemetry ×3 (`Enum` Deny/Allow-flagged/Allow-
  all), Auto-add Contacts, Auto-add Max Hops, Duty Cycle Factor, RX Delay Factor + Restore
  Defaults. RX Boost's setter calls `radio_driver.setRxBoostedGainMode()` — a live-apply pairing
  not called out by name in PLAN.md but found by the same reasoning PLAN.md applies to radio
  params (`MyMesh::begin()` calls the same method right after loading prefs). Repeat's setter
  calls `the_mesh.isValidClientRepeatFreq()` before allowing it on. Per-contact-type auto-add
  allow bits are deliberately not exposed (see PROGRESS.md).
- **`Screen_SettingsDevice`** — Buzzer (`Toggle`, wired directly to the existing
  `UITask::toggleBuzzer()` rather than a fresh get/set pair — `set()` ignores its `value` argument
  and just calls `toggleBuzzer()`, since `ToggleField` only ever calls `set()` with the opposite of
  what `get()` returned, so "flip" and "set-to-value" are equivalent here) + Notifications (stub,
  toasts a placeholder string — Phase 7 builds the real screen) + Restore Defaults. Takes
  `UITask*`, not `NodePrefs*` — it never touches `_node_prefs` directly.
- **`Screen_SettingsDanger`** — Erase All Data / New Identity / Reboot. Every row is
  `MenuItemKind::Action` (never `Toggle`/`Stepper`/etc.) whose callback arms `UITask`'s shared
  `ConfirmScreen` before doing anything irreversible — there is no code path from this screen that
  reaches `MyMesh::factoryReset()`/`selfRekey()` or `UITask::shutdown(true)` without going through
  `ConfirmScreen` first. Takes `UITask*` for the Reboot action; Erase/Rekey call `the_mesh`
  directly (no `UITask*` needed for those two).

**Two new `MyMesh` methods** (`examples/companion_radio/MyMesh.h`/`.cpp`, not `ui-forest`) back
Danger Zone: `factoryReset()` (on-device equivalent of `CMD_FACTORY_RESET`'s handler — disable
serial, format filesystem, reboot) and `selfRekey()` (on-device equivalent of
`CMD_IMPORT_PRIVATE_KEY`'s identity-regen side effect, generating a fresh identity instead of
importing one, then `resetContacts()` + reload from disk to invalidate ECDH secrets computed
against the old identity). Both exist because `_store`/`self_id`/`resetContacts()` are private to
`MyMesh` and `UITask` isn't part of that class hierarchy — same reasoning PLAN.md already used to
justify `getQueueLen()` for Phase 4. `isValidClientRepeatFreq()` was also widened from `private`
to `public` (no implementation change) so the two settings screens above could call it.

### `Screen_Diagnostics` / `Screen_DiagRadio` / `Screen_DiagPackets` / `Screen_DiagCore` / `Screen_EventLog` (Phase 4)

`Screen_Diagnostics` is a thin `MenuScreen` subclass, same shape as `Screen_Settings` -- 4
`Submenu` rows (Radio/Packets/Core/Event Log), no callbacks of its own. The four leaf screens
underneath it are **not** `MenuScreen` subclasses, though -- they're read-only stat dumps, so
they instead reuse `Screen_ContactDetail` (Phase 2)'s Layout-scrolled label/value list shape:
a one-row header + separator, then `Layout::visibleRows()`-scrolled `label`/`value` pairs drawn
via `drawTextLeftAlign`/`drawTextRightAlign`, `KEY_NEXT`/`KEY_PREV` scroll, `KEY_CANCEL`/
`KEY_ENTER`/`KEY_SELECT` pop back. `MenuItemKind::Info` (declared since Phase 0) stays an unused
placeholder -- it only ever renders `item.label`, a static string, so showing a live per-frame
value through it would mean mutating a label buffer imperatively before every render, which is
more awkward than the label/value-columns shape `Screen_ContactDetail` already established for
exactly this problem.

- **`Screen_DiagRadio`** -- 3 rows: Noise Floor / RSSI / SNR, via `radio_driver.getNoiseFloor()`/
  `getLastRSSI()`/`getLastSNR()` (already used by `Screen_RadioInfo`, zero new API). Deliberately
  doesn't also show tx/rx air-time even though the companion app's `STATS_TYPE_RADIO` reply
  includes both -- phase-4-diagnostics.md step 3 names exactly these 3 fields.
- **`Screen_DiagPackets`** -- 7 rows: Received / Sent / Flood TX / Direct TX / Flood RX / Direct
  RX / RX Errors, via `radio_driver.getPacketsRecv()/getPacketsSent()/getPacketsRecvErrors()` +
  `the_mesh.getNumSentFlood()/getNumSentDirect()/getNumRecvFlood()/getNumRecvDirect()`. All seven
  are public, zero new API -- `getNumSentDirect()`/`getNumRecvDirect()` (`src/Dispatcher.h:187,189`)
  confirmed to exist right alongside the flood counters, resolving PLAN.md §8's open question.
  This exact 7-field set matches `MyMesh::handleCmdFrame`'s `STATS_TYPE_PACKETS` reply
  (`MyMesh.cpp`) field-for-field, since that's what the companion app's stats view parses.
- **`Screen_DiagCore`** -- 3 rows: Queue (new `MyMesh::getQueueLen()` passthrough), Uptime
  (`millis()/1000` -- deliberately not `rtc_clock`, matching `STATS_TYPE_CORE`'s own
  `_ms->getMillis()/1000`, so this reads the same as the companion app even before the wall clock
  is set), Transport (`UI_FOREST_TRANSPORT_NAME`, see `Transport.h` below).
- **`Screen_EventLog`** -- reads the shared `EventLog` ring buffer by reference (owned by
  `UITask`, fed via `UITask::logEvent()`), renders newest-entry-first with an "Ns/Nm/Nh/Nd ago"
  prefix computed from `millis() - entry->timestamp`, ellipsized via `drawTextEllipsized()` per
  row so long entries never overflow narrow displays.

**`EventLog` (`EventLog.h`)** -- a small standalone header-only class, not a `ui-forest` screen:
a fixed 20-entry ring buffer (`push(text)`, `count()`, `getEntry(idxFromNewest)`), no allocation,
same discipline as `NavStack`. Kept as its own file (rather than nested inside
`Screen_EventLog.h`) specifically so Phase 7's `Screen_RecentEvents` can construct its own
instance of the same buffer class (PLAN.md §7's note on the two screens sharing the buffer class
while staying separate screens). `UITask` owns the single instance (`_event_log`) and exposes
`void logEvent(const char* text)` as the one place any screen appends to it -- fed from four
places that already observe something worth logging (not from new `Mesh`/`MyMesh` callback
plumbing, per phase-4-diagnostics.md step 7's explicit constraint):
`UITask::notify()` (message/channel/room/new-contact events -- `ack` deliberately excluded, see
below), `Screen_Advert`'s send action, `Screen_SettingsRadio`'s `applyRadioParams()`/`setTxPower()`,
and `UITask::loop()`'s low-battery branch. `ack` isn't logged because `notify(UIEventType::ack)`
also fires as a generic confirmation tone for GPS/buzzer toggles (`toggleGPS()`/`toggleBuzzer()`),
which would flood the 20-entry buffer with toggle noise. "Contact discovered" and "advert
received" (two of phase-4-diagnostics.md's four example events) have no existing hook to observe
from -- `MyMesh::onDiscoveredContact` only ever writes to `_serial`, never touches `_ui` -- so
they're not logged; adding one would be exactly the new-callback-plumbing step 7 says not to add.

**`Transport.h`** -- a small standalone header resolving PLAN.md §8's transport-type open
question: `#if defined(WIFI_SSID)` / `#elif defined(BLE_PIN_CODE)` / `#elif defined(ETHERNET_ENABLED)`
/ `#else` (USB), mirroring the exact precedence `main.cpp` uses to pick a `BaseSerialInterface`
subclass at compile time -- confirmed by reading `main.cpp` directly, not guessed. Defines
`UI_FOREST_TRANSPORT_NAME` as a string literal, set once, not queried per frame. Used by both
`Screen_DiagCore` (a row) and `UITask::updateStatusBar()` (replacing what used to be a hardcoded
`"BLE:%s"` label in the scrolling status text with the real compiled-in transport name).

**One new `MyMesh` method** (`examples/companion_radio/MyMesh.h`, not `ui-forest`) backs
`Screen_DiagCore`'s Queue row: `uint32_t getQueueLen() const { return _mgr->getOutboundTotal(); }`.
`_mgr` is `protected` (not private) on `Dispatcher`, reachable from `MyMesh`'s own member functions
via the public inheritance chain `MyMesh -> BaseChatMesh -> mesh::Mesh -> Dispatcher` -- confirmed
by grep, not assumed. Same passthrough shape as Phase 3's `factoryReset()`/`selfRekey()`.

**Home's Phase 4 addition:** a "Diagnostics" `Submenu` entry, inserted right before "Settings"
(after the optional GPS/Sensors slots) -- `UI_FOREST_HOME_ITEM_COUNT` bumped 11 -> 12.
phase-4-diagnostics.md's own stated prerequisite (that Phase 3 already added this slot as a
placeholder) didn't hold — see "Known gaps" below — so this phase adds the Home entry and its
real content in one step instead of the originally-planned two.

### `Screen_MsgPreview`

Straight port of `MsgPreviewScreen`. `UITask::newMsg()` calls `addPreview()` then
`nav.reset(_msg_preview)` -- a full stack replacement, not a `push()`, matching ui-new's own
`setCurrScreen(msg_preview)` interrupt behavior (new messages always yank the display away
from whatever was showing, discarding any submenu context). Dismissing (`KEY_ENTER`/
`KEY_SELECT`/`KEY_CANCEL`/`KEY_PREV`, or paging through all unread via `KEY_NEXT`/`KEY_RIGHT`
until none remain) calls `nav.reset(_home)`, so it always lands back at Home root, never at
whatever screen was showing before the interrupt -- again matching ui-new exactly.

### `Screen_NotificationSettings` / `Screen_NotificationEventConfig` / `Screen_RecentEvents` (Phase 7)

`AbstractUITask.h`'s `UIEventType` gained a sixth value, `advertSent` -- a user-initiated advert
send, kept distinct from the generic `ack` confirmation tone (which is also fired for GPS/buzzer
toggle confirmations, see `toggleGPS()`/`toggleBuzzer()` below) so it can be independently
configured/logged. Every `notify()` implementation in `ui-new`/`ui-tiny`/`ui-orig` already ends its
switch with a `default:` case (confirmed by grep before adding this), so the new value is a silent
no-op there.

**`NotificationPrefs.h`** -- a new standalone header: `NotificationTypeConfig{bool buzzer; bool
vibration;}`, one per configurable event type (`contactMessage`/`channelMessage`/`ack`/
`advertSent` -- phase-7-notifications.md's "at minimum" list), bundled into a `NotificationPrefs`
struct `UITask` owns as `_notify_prefs`. Deliberately UI-local/non-persisted, not new `NodePrefs`
fields -- same reasoning as Phase 3's vibration-persistence gap (see PROGRESS.md): `NodePrefs` is a
hand-maintained, unversioned, fixed-byte-offset binary format with no length guard, and extending
it without a compiler on PATH to verify the change is real structural surgery on existing users'
saved prefs files. Resets to all-on every reboot as a result.

**`Screen_NotificationEventConfig`** -- one reusable `MenuScreen` subclass, instantiated four times
(as plain member objects of `Screen_NotificationSettings`, not `UITask`-owned pointers -- see below)
-- a "Buzzer" `Toggle` row plus, only `#ifdef PIN_VIBRATION`, a "Vibration" `Toggle` row, both
operating directly on the `NotificationTypeConfig&` reference this instance was built with. No
`the_mesh.savePrefs()` call anywhere in this class (unlike every other `Toggle` row in this
codebase) since there's no persisted field behind it. LED isn't a third row here -- `userLedHandler()`
(Phase 1) is a generic `_msgcount`-driven heartbeat never routed through `notify()` or any per-event
trigger point, so there's nothing per-event to gate.

**`Screen_NotificationSettings`** -- root menu (`Screen_SettingsDevice`'s "Notifications" row, a
Phase-3 toast stub, now pushes this instead), 4 `Submenu` rows (Message/Channel Message/Ack/Advert)
into the four `Screen_NotificationEventConfig` instances it holds as direct members. This is a
deliberate departure from the "every screen is a `UITask`-owned pointer `new`'d in `begin()`"
pattern every other screen in this codebase follows: since `Screen_NotificationSettings` itself is
`new`'d exactly once in `UITask::begin()` and never moved afterward, its member sub-objects get
stable addresses for the process lifetime, which is all `NavStack::push()` needs -- and nothing
outside this one menu ever needs to reference any of the four individually, so there was no reason
to also give `UITask` four more pointer members for them.

**`Screen_RecentEvents`** -- the user-facing notification history (PLAN.md item 35), rendering
against a *second*, separately-fed `EventLog` instance (`UITask::_recent_events`), not a filtered
view over Phase 4's diagnostic `_event_log`. Render/scroll logic is a near-duplicate of
`Screen_EventLog` (same newest-first, age-prefixed, `Layout`-scrolled shape) -- kept as its own
class/file (matching PLAN.md §4's explicit two-file listing) rather than parameterizing
`Screen_EventLog` with a title, since the two remain conceptually separate screens for separate
audiences even though the buffer class is shared, per PLAN.md §7's own note.

**`UITask::notify()` restructured**: a new per-event-type gating switch (`buzzer_on`/`vibration_on`,
read from `_notify_prefs`) runs before the existing tune-selection switch (which gained an
`advertSent` case) and the existing `PIN_VIBRATION` trigger; only `contactMessage`/`channelMessage`/
`ack`/`advertSent` are independently gated -- `roomMessage`/`newContactMessage`/`none` fall through
every switch's `default:` case and keep their exact pre-Phase-7 behavior (unconditional, no
distinguishable tune). A third switch feeds `Screen_RecentEvents` (via the new
`logRecentEvent()` passthrough, mirroring `logEvent()`'s shape): `contactMessage`/`channelMessage`/
`newContactMessage` (the last as a proxy for "last contact seen", same reasoning Phase 4 already
used for its own diagnostic-log entry of the same name -- there's no real contact-discovery hook
into `_ui`). `ack` is deliberately excluded from this feed despite phase-7-notifications.md's own
checklist naming it, because every real call site of `notify(UIEventType::ack)` in this codebase is
a generic UI-action confirmation tone (GPS/buzzer toggles, and pre-Phase-7 `Screen_Advert`), not a
real mesh delivery ack -- logging it would fill Recent Events with toggle noise, the same reasoning
Phase 4 used to exclude `ack` from the diagnostic event log. `advertSent` isn't logged from inside
`notify()` either -- `Screen_Advert` calls `logRecentEvent()` itself, since it (not `notify()`)
knows the `the_mesh.advert()` result.

**`Screen_Advert`** changed its confirmation-tone call from unconditional `notify(UIEventType::ack)`
(fired before knowing the send result) to `notify(UIEventType::advertSent)` fired only on success --
see PROGRESS.md's Phase 7 "Decisions" for the reasoning and the resulting small behavior change
(no tone on a failed send, though the failure toast is unchanged).

**Home's Phase 7 addition:** a "Recent Events" `Submenu` entry, inserted right before "Diagnostics"
-- `UI_FOREST_HOME_ITEM_COUNT` bumped 12 -> 13. Deliberately a distinct entry from the existing
Phase 1 "Recent" (`Screen_Recents`, recently-heard adverts/nodes) rather than reusing that name or
screen -- the two show unrelated content.

### `UITask`

Owns one instance of every framework class above — including the four shared `FormField` editor
instances (`_toggle_field`/`_stepper_field`/`_enum_field`/`_text_field`, Phase 3), the single shared
`EventLog` instance (`_event_log`, Phase 4), and, as of Phase 7, a second `EventLog`
(`_recent_events`) plus `NotificationPrefs _notify_prefs` — plus every screen and the real Home
`MenuItem[]` table (sized `UI_FOREST_HOME_ITEM_COUNT` = 13 as of Phase 7, the worst case with
GPS+Sensors both present plus Contacts/Channels/Recent Events/Settings/Diagnostics;
`_home_item_count` tracks how many slots are actually used on this board). Home's item order is
Status, Recent, Radio, Bluetooth, Advert, Contacts, Channels, [GPS], [Sensors], Recent Events,
Diagnostics, Settings, Shutdown.
`begin()` constructs `_contact_detail` before `_contacts` (the latter's constructor takes a
`Screen_ContactDetail*`), every other leaf screen (including, as of Phase 7, `_recent_events_screen`
alongside the other top-level ones), then the five `Screen_SettingsXxx` sub-screens before the root
`Screen_Settings` (Phase 7: `_notification_settings` constructed before `_settings_device`, which
needs a pointer to it, same "leaf before container" ordering), then the four
`Screen_DiagXxx`/`Screen_EventLog` leaf screens before the root `Screen_Diagnostics` (same ordering
again), fills the item table with `MenuItemKind::Submenu` entries pointing at all of them, then
constructs `_home` (a `MenuScreen`), `_msg_preview`, and `_splash` in that order (each later one
needs a pointer to something built earlier), and calls
`nav.reset(_splash)`. `loop()` is the render loop described above, now also driving
`InputRouter`'s callback methods (`checkDisplayOn`/`handleLongPress`/`handleDoubleClick`/
`handleTripleClick`), `userLedHandler()`, buzzer/vibration `loop()`, auto-off, and the
`AUTO_SHUTDOWN_MILLIVOLTS` low-battery shutdown check -- all ported from `ui-new`'s `loop()`
tail. `msgRead`/`newMsg`/`notify` (the `AbstractUITask` virtuals `MyMesh` calls) now have real
behavior: `notify()` gates buzzer tones/vibration per event type against `_notify_prefs` (Phase 7),
plays the tune/triggers vibration, feeds the Phase 4 diagnostic event log for message/channel/room/
new-contact events, feeds the Phase 7 Recent Events buffer for the subset of those that also
buzz/vibrate, `newMsg()` pushes `Screen_MsgPreview`, `msgRead(0)` returns to Home.

## Board configuration surface

Every per-board knob ui-forest reacts to is a compile-time macro set in that board's
`platformio.ini` env, exactly like `ui-new`:

| Macro | Meaning | Consumed by |
|---|---|---|
| `PIN_USER_BTN` | single momentary button pin | `InputRouter` |
| `PIN_USER_BTN_ANA` | analog resistor-ladder button pin | `InputRouter` |
| `UI_HAS_JOYSTICK` | 3+-button joystick board | `InputRouter` |
| `JOYSTICK_UP` / `JOYSTICK_DOWN` | optional real up/down pins on a joystick board | `InputRouter` (guards the extra polling block) |
| `UI_HAS_ROTARY_INPUT` | rotary encoder present | `InputRouter` |
| `HAS_TORCH` | second physical button doubles as a torch toggle | `InputRouter` (compiled, not hardware-verified -- no Phase 1 pilot has it) |
| `DISPLAY_CLASS` | concrete `DisplayDriver` subclass for this board | `main.cpp` (constructs `disp`, passed into `UITask::begin()`) |
| `PIN_BUZZER` | buzzer present | `UITask` (tones via `notify()`, mute toggle, `StatusBar`'s muted icon) |
| `PIN_VIBRATION` | vibration motor present | `UITask` (`notify()` triggers it) |
| `PIN_STATUS_LED` | status LED pin | `UITask::userLedHandler()` (compiled -- `ThinkNode_M1`, Phase 8, is the first board defining this macro; still not hardware-verified, nobody has flashed that board) |
| `ENV_INCLUDE_GPS` | board has a GPS module | `UITask` (gates constructing/wiring `Screen_Gps`; `heltec_rc32`'s forest env sets this) |
| `UI_SENSORS_PAGE` | board wants the CayenneLPP sensor-scroll screen | `UITask` (gates constructing/wiring `Screen_Sensors`; `WioTrackerL1`'s forest env sets this) |
| `AUTO_OFF_MILLIS` | display auto-off delay (default 15000, `0` disables) | `UITask::loop()` |
| `AUTO_SHUTDOWN_MILLIVOLTS` | battery voltage that triggers auto-shutdown | `UITask::loop()` (`heltec_rc32`'s forest env sets this to 3400) |
| `KEEP_DISPLAY_ON_USB` | opt-in: don't auto-off while externally powered | `UITask::loop()` |
| `WIFI_SSID` / `BLE_PIN_CODE` / `ETHERNET_ENABLED` | which transport `main.cpp` compiled in (checked in that precedence order, else USB) | `Transport.h` (Phase 4) -- resolves to `UI_FOREST_TRANSPORT_NAME`, consumed by `Screen_DiagCore` and `UITask::updateStatusBar()`. Reports "USB" (wrong) on `sensecap_indicator-espnow`, whose ESPNOW transport isn't in this precedence chain at all -- known, deliberately unfixed this phase, see PROGRESS.md's Phase 6 section |
| `HAS_TOUCH` | capacitive touchscreen present (`sensecap_indicator-espnow`) | `InputRouter` (Phase 6 -- tap/swipe polling block); also the gate `InputRouter::activateHint()`/`moveHint()` check first |
| `LILYGO_TDECK` | keyboard co-processor + trackball present (`lilygo_tdeck`) | `InputRouter` (Phase 6 -- trackball/keyboard polling block, and the hint functions' second-highest precedence check) |
| `TDECK_TRACKBALL_UP` / `_DOWN` / `_LEFT` / `_RIGHT` | trackball direction GPIO pins, `#ifndef`-defaulted in `variants/lilygo_tdeck/target.h` (Phase 6) | `variants/lilygo_tdeck/target.cpp` (the `trackball_*` `MomentaryButton` globals `InputRouter` polls) |
| `TDECK_KEYBOARD_I2C_ADDR` | keyboard co-processor's I2C address, `#ifndef`-defaulted in `TDeckKeyboard.h` (Phase 6) | `TDeckKeyboard::poll()` |

## Rollout mechanism

Each pilot board gets one new `_companion_radio_forest_ble`/`_forest_usb` env in its own
`variants/<board>/platformio.ini`, `extends`-ing the board's base section (not the existing
`_ble`/`_usb` env) and restating `build_flags`/`build_src_filter`/`lib_deps` from scratch with
two lines changed: the `-I` path and the `ui-*/*.cpp` glob. Existing `_ble`/`_usb`/`_wifi` envs
for every board are untouched. See any of the eight `variants/*/platformio.ini` files' forest env
for the exact template (`sensecap_indicator-espnow`'s is named `_comp_radio_forest_usb`, not
`_companion_radio_forest_usb` — matches that board's existing, already-shortened `_comp_radio_usb`
env, see PROGRESS.md's Phase 6 section); `PROGRESS.md` lists which eight boards have one today
(Phase 8 added the eighth, `ThinkNode_M1`).

## Phase 5 additions (visual polish)

- **The status-bar/menu overlap bug (Phase 2-4) is fixed. Root cause: Adafruit_GFX's default
  text-wrap, not a `Layout` offset bug.** Every screen's `Layout::statusBarHeight()`/`headerHeight()`
  math was already correct (re-verified from scratch this phase, per the user's instruction not to
  reuse the ruled-out u8g2-baseline theory) -- the actual cause lives one layer lower. `StatusBar`'s
  marquee scroll (`StatusBar.cpp::render()`) deliberately prints its full (screen-width-exceeding)
  string starting at a negative x so it can slide across the strip. `Adafruit_GFX::write()` wraps to
  the next text row (`cursor_y += textsize*8`) once `cursor_x` would run past the right edge --
  which, starting from a very negative x, happens *partway through* the string, not at its end.
  With wrap left on (the library default; nothing in this codebase had ever turned it off), that
  wrap dumped the tail of the status-bar text onto row 8+, painting over whatever the current
  screen had just drawn there -- exactly the reported symptom. Confirmed by reading all ten
  `DisplayDriver` backends again (same discipline as the Phase 4 status-bar-flash fix): `SH1106Display`,
  `SSD1306Display`, `ST7789LCDDisplay`, and `GxEPDDisplay` all wrap `Adafruit_GFX`/`GxEPD2` (which
  extends it) and default to wrap-on; `NV3001BDisplay`'s hand-rolled `print()` never auto-wraps
  (only on explicit `\n`/`\r`), so it was never affected regardless of board. **Fixed** by adding
  `display.setTextWrap(false)` to those four backends' `startFrame()` (`src/helpers/ui/*.cpp`, not
  `ui-forest` itself -- this is a framework-level bug, not specific to this UI variant). Off-edge
  characters now simply clip instead of wrapping, which is exactly what a horizontally-scrolling
  marquee needs. `LGFXDisplay`/`ST7735Display`/`E213Display` weren't touched -- they wrap
  LovyanGFX/TFT_eSPI/`heltec-eink-modules` respectively, none of which are vendored in-repo to
  verify `setTextWrap`'s exact signature against, and (`ST7735`/`LGFX` are Phase 6-deferred boards,
  `E213` was the e-ink pilot *not* chosen this phase, see below) none of the currently-built forest
  envs exercise them. This was a real device-reported bug (WioTrackerL1/`SH1106Display`) fixed by
  reading library source, not yet re-confirmed fixed on hardware.
- **`StatusBar`'s battery gauge now clears its own bounding box before drawing** (real bug, found by
  the user reviewing the diff): `renderBattery()` is deliberately called *after* the scrolling text
  so the gauge always wins, but the gauge itself is only an outline + partial fill + optional muted
  glyph, not a solid opaque block -- so any marquee-text pixels landing inside its bounding box but
  outside those lit segments showed straight through, mixing scrolled characters into the battery
  icon. Fixed by `fillRect()`-ing the full gauge-plus-muted-icon box in `DARK` first.
- **Node names containing emoji/non-ASCII now render cleanly in the status bar.** `UITask::
  updateStatusBar()` previously passed `_node_prefs->node_name` straight into the scrolling-text
  `snprintf()`; other screens run user-supplied names through `DisplayDriver::
  translateUTF8ToBlocks()` (substitutes one block glyph per multi-byte sequence), but doing that in
  the compact single-line status strip would read as clutter for a multi-codepoint emoji. Added a
  small `stripNonAscii()` in `UITask.cpp` that drops every non-ASCII byte instead of substituting
  anything, applied to the name before it's formatted in.
- **`StatusBar` now takes an `is_eink` flag** (`StatusBar::begin(int display_width, bool is_eink)`,
  set from `_display->isEink()` in `UITask::begin()`) and skips the marquee entirely when true --
  `render()` draws the truncated/ellipsized form instead (`DisplayDriver::drawTextEllipsized()`,
  already used elsewhere for long names), and `needsRedraw()` no longer fires purely off the
  scroll-cadence timer, so an e-ink build doesn't churn extra redraws for a scroll effect it never
  shows. Resolves PLAN.md 3.3/3.2's explicit "no marquee-scrolling `StatusBar` on e-ink" callout.
- **Expanded icon set (`icons.h`)** -- confirmed (repo-wide grep, not assumed) that no XBM
  conversion script/convention exists anywhere in this repo, resolving PLAN.md 8's open question.
  New 16x16 icons: `settings_icon` (equalizer-style sliders, not a literal gear -- a clean circular
  gear rim doesn't hand-author reliably at 16px without a rendering pass to check it against),
  `warning_icon` (solid triangle with the "!" cut into it in the background color), `event_log_icon`
  (bullet list), `contact_icon` (person silhouette), `channel_icon` ("#" glyph), `gps_fix_icon` /
  `gps_nofix_icon` (filled/outline diamond), `signal_bars_0`..`signal_bars_4` (5-frame signal-strength
  bars, plus a `signal_bars[5]` lookup array). One new 32x32: `torch_icon` (flame + handle, same
  full-screen-glyph size as `bluetooth_on`/`advert_icon`/`power_icon`), wired nowhere yet -- same
  compiled-but-hardware-unverified status as the rest of the `HAS_TORCH` path. All were generated
  from small ASCII-art/formula descriptions via a one-off Node script (not part of the build, not
  checked in) rather than hand-typed byte-by-byte, specifically to avoid transcription errors in
  bitmap data nobody can visually or compile-check in this dev environment; the generated bytes
  were then cross-checked row-by-row against the script's intent before being pasted in.
  `signal_bars_*` are **not wired into any live reading this phase** -- RSSI's "no packet received
  yet" sentinel value differs across the six radiolib wrapper backends
  (`src/helpers/radiolib/Custom*Wrapper.h`) and wasn't verified for all of them here; wiring a
  threshold mapping in blind risked showing "full signal" before any packet had ever been heard.
- **Icons are wired into `Screen_Contacts`/`Screen_Channels` list rows and two of
  `Screen_SettingsDanger`'s three rows, deliberately *not* into Home's or `Screen_Diagnostics`'s
  `MenuItem` tables.** Discovered while wiring the first icon in: `MenuScreen`'s existing 16x16 icon
  slot never had a real caller before this phase, and nobody had noticed that `Layout::rowHeight()`
  (11px at text size 1) is shorter than a 16x16 icon -- drawing one would have bled 5px into the
  next row. Fixed generically with two new `Layout` helpers, `iconRowHeight()`/`visibleIconRows()`
  (return `max(rowHeight(), 18)` and its matching visible-row count), and a `MenuScreen`-constructor-time
  scan (`_has_icons`, cached once since `MenuItem` tables are static/built-once) that switches a
  *whole menu* over to the taller row height only if at least one of its rows actually has an icon --
  menus with none (the majority) are completely unaffected. Applying that everywhere still has a
  real cost, though: Home (up to 12 items) and `Screen_Diagnostics` (4 items) would each lose about
  a third of their visible rows on a 64px OLED for the sake of icons on a handful of entries, which
  felt like a worse trade than it was worth on the app's most-used menus. So Home/Diagnostics keep
  every `MenuItem::icon` as `NULL` (unchanged), while `Screen_Contacts`/`Screen_Channels` (every row
  has one, by design -- no mixed-density cost) and `Screen_SettingsDanger` (only 3 rows total, minor
  cost) use them. Also fixed while wiring `SettingsDanger`: when a menu mixes icon and non-icon rows
  (Erase/New Identity get `warning_icon`, Reboot doesn't -- disruptive but not data-destructive, so
  the icon stays a meaningful signal rather than decorating every row), `MenuScreen::render()` now
  indents *every* row's text to the icon column once `_has_icons` is true, even icon-less ones --
  otherwise the text columns would ragged-left instead of aligning.
- **`MenuItem` gained an optional `tint`/`has_tint` pair** (defaults `LIGHT`/`false` via the struct's
  own default member initializers, so none of the ~15 existing `MenuItem`-building call sites needed
  touching) -- overrides a row's *unselected* color, used by `Screen_SettingsDanger`'s two
  data-destructive rows (`DisplayDriver::RED`) so the warning reads even before the row is
  highlighted. `MenuScreen::render()`'s selected-row color is unaffected (still always
  GREEN-highlight/DARK-text) -- tint only applies to the unselected state.
- **Card-style grouping (`Layout::drawCard()`)** -- left/right/bottom edges around a screen's
  scrollable content block, added to the five screens sharing the label/value-list or
  ellipsized-line-list shape (`Screen_ContactDetail`, `Screen_DiagRadio`, `Screen_DiagPackets`,
  `Screen_DiagCore`, `Screen_EventLog`). Deliberately left/right/bottom only, not a full rectangle --
  the header separator every one of these screens already draws doubles as the card's top edge, so a
  second line directly under it would sit exactly where the first row's text is drawn. Also
  deliberately one border per screen, not a divider between every row: at `row_h=11` on a 64px OLED,
  rows already sit close together, and a line every ~11px reads as clutter, not polish, at that size.
  Every row's text also moved in by 2px from each edge (`x=0`/`width()-1` -> `x=2`/`width()-3`) so it
  doesn't sit directly on the new left/right border lines.
- **Color convention (green=good, yellow=info, red=warning) swept across the screens that had a
  real green/red status to show** (`ConfirmScreen`/`EnumField`/`StepperField` already followed it
  since Phase 3): `Screen_Bluetooth` (green=serial enabled, red=disabled -- previously always green
  regardless of state), `Screen_Gps` (green=on, red=off for the top line; green=fix/yellow=searching,
  not red, for the fix line -- no fix indoors or just after power-on is normal, not an error),
  `Screen_DiagPackets` (RX Errors row only: red once any have been seen, still yellow at zero --
  every other counter stays yellow=info), `FormField`'s `ToggleField` (green=on/red=off).
- **`DisplayDriver::supportsColor()` (new virtual, default `true`) + `Layout::accentColor()`**,
  added after the user pointed out that picking a semantic color on a display that can't actually
  render it isn't just "invisible", it's an unreviewed accident waiting to happen -- reusing a
  hue-based distinction on a 1-bit buffer relies entirely on that buffer's `setColor()` collapsing
  every non-`DARK` value to the same "on" pixel, which is true for every backend audited (confirmed
  by re-reading all ten again) but shouldn't be something calling code has to trust implicitly.
  `supportsColor()` is overridden `false` in every backend confirmed to be a strict 1-bit/2-color
  buffer regardless of the physical panel's own capability: `SH1106Display`, `SSD1306Display`,
  `U8g2Display` (monochrome OLEDs); `GxEPDDisplay`/`E213Display`/`E290Display` (`GxEPD2_BW`/
  `heltec-eink-modules` -- black/white e-ink); and, notably, `ST7789Display` -- despite driving a
  real color TFT panel, its `ST7789Spi` wrapper is built on ThingPulse's `OLEDDisplay`, a strict
  1-bit framebuffer (confirmed by its own `setColor()`, whose non-`DARK` cases are all `#if 0`'d out
  already). `ST7735Display`/`ST7789LCDDisplay`/`NV3001BDisplay`/`LGFXDisplay` keep the default `true`.
  `Layout::accentColor(display, wanted)` returns `wanted` unchanged when `supportsColor()`, else
  plain `LIGHT` -- every conditional color pick added or touched this phase now goes through it
  (`Screen_Bluetooth`, `Screen_Gps`, `Screen_DiagPackets`, `MenuItem` tint in `MenuScreen::render()`,
  `FormField`'s `ToggleField`) instead of calling `display.setColor()` with a raw `RED`/`GREEN`/
  `YELLOW` and relying on the driver to downgrade it silently.
- **Screen-transition animation (PLAN.md item 29): attempted, then reverted after user feedback.**
  First cut was a 4-step solid-color bar wipe in `NavStack::push()`/`pop()`/`popToRoot()` using only
  `fillRect()`/`startFrame()`/`endFrame()`/`setColor()`. The user reviewed it and reported it "looks
  more like a weird flash / bug" rather than a transition. On reflection this is a fundamental
  limitation, not a tuning problem: every `DisplayDriver` backend fully clears its buffer on
  `startFrame()` (confirmed across all ten in the Phase 4 fix), so *any* animation built from real
  display updates is necessarily a sequence of full-screen redraws, not a compositing effect -- and
  most pilot boards are monochrome, where a "wipe" is just a hard-edged block of on/off pixels, not
  a gradient. With no way to see the actual result in this dev environment, guessing at different
  step counts/timings risked the same outcome again. Removed entirely (`NavStack` is back to the
  pre-Phase-5 shape, no `DisplayDriver*`/`setDisplay()`/`playTransition()`) -- no transition is a
  safer default than a wrong-looking one PLAN.md item 29 was always the most speculative,
  hardware-dependent item in this phase's list.
- **`lilygo_techo` chosen as the e-ink pilot board** (over `heltec_e213`) specifically because its
  `GxEPDDisplay` wraps `GxEPD2_BW`, which is vendored in-repo (`src/helpers/ui/GxEPDDisplay.h`
  includes `<GxEPD2_BW.h>` etc.) and confirmed to extend `Adafruit_GFX` -- so the text-wrap fix
  above could be verified against real, in-tree headers the same way every other backend was.
  `heltec_e213`'s `E213Display` wraps the external `heltec-eink-modules` library's `BaseDisplay`,
  not vendored anywhere in this repo, so its text-wrap behavior (and thus whether it needed the same
  fix) couldn't be verified the same way in this no-compiler dev environment -- left untouched
  rather than guessed at. New env: `LilyGo_T-Echo_companion_radio_forest_ble`
  (`variants/lilygo_techo/platformio.ini`), built as the same two-line diff (`-I` path,
  `ui-*/*.cpp` glob) from the existing `LilyGo_T-Echo_companion_radio_ble` env as every other forest
  env, per PLAN.md §5's rollout convention.
- **Adaptive layout review pass (item 31): no structural violations found, but several bare pixel
  literals were replaced with `Layout::rowHeight()`-based expressions for consistency** --
  `Screen_RadioInfo`/`Screen_Recents` (`y += 11` -> `y += Layout::rowHeight()`, same value, just
  named), `Screen_Gps`/`Screen_Sensors` (`y += 12` -> `y += Layout::rowHeight() + 1`, preserving the
  existing slightly-looser spacing), and the `display.height() - 11` bottom-hint-row pattern repeated
  across `Screen_Bluetooth`/`Screen_Advert`/`Screen_Shutdown`/all four `FormField` editors (->
  `display.height() - Layout::rowHeight()`; `ConfirmScreen` already used this form). `Screen_Splash`'s
  fixed logo/version-text y-positions and the low-battery shutdown message in `UITask::loop()` were
  deliberately left as literals -- both are one-off centered full-screen content, not list rows, so
  `Layout`'s row-based helpers don't conceptually apply to them.

## Known gaps carried forward

- No contact/channel editing (add/delete/rename) -- Phase 2 is read-only browsing only, matching
  PLAN.md 1's non-goals; editing is a stretch goal, not scheduled.
- No persisted vibration-enable setting -- see PROGRESS.md's Phase 3 "Decisions" for why (adding
  one means hand-editing `DataStore.cpp`'s unversioned binary prefs format, out of scope for a
  UI-only phase done with no compiler available to verify the change).
- Per-contact-type auto-add allow bits and "overwrite oldest when full" are not exposed in
  Settings -- deliberate scope cut, see PROGRESS.md's Phase 3 "Decisions".
- `StatusBar` always renders its scrolling text as text; no per-board icon-vs-text switch at
  `display.height() > ~80px` (PLAN.md §3.2's suggestion) -- superseded in practice by Phase 5's
  narrower e-ink text-vs-marquee switch (`is_eink`), but the broader "use icons instead of text on
  tall/wide displays generally" idea was never built. `signal_bars_0..4` exist as icon assets
  (Phase 5) but aren't wired into `StatusBar` -- see "Phase 5 additions" above for why. `FormField`'s
  four editor screens still have no icons of their own either.
- `HAS_TORCH` and `PIN_STATUS_LED` code paths are compiled but not hardware-verified (no pilot
  board defines either) -- `torch_icon` (Phase 5) exists as an asset but isn't wired into any
  `HAS_TORCH`-gated screen, same unverified status.
- `ConfirmScreen`'s auto-fire-after-N-seconds design (vs. ui-new's press-and-hold) hasn't been
  validated on real hardware yet, despite now having 5 real callers (`Screen_Shutdown` plus every
  Danger Zone action and every settings screen's Restore Defaults) -- still worth a hold-it-in-
  your-hand judgment call once any of them can actually be tried.
- The Phase 2 `KEY_HOME`-via-triple-click mechanism (see "`KEY_HOME`" above) hasn't been
  button-mashed on real hardware -- confirm mute is still reachable at Home and that jumping
  home from deep in Contacts/Channels/ContactDetail/any `FormField` editor actually feels right,
  on all three pilots.
- **Phase 3 has been flashed and informally tested on WioTrackerL1** (radio param live-apply and
  reboot-survival confirmed -- see PROGRESS.md's Phase 3 "Real-device findings"), but not on
  `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32`, and Danger Zone's three actions were not
  triggered on any board. One real bug (a one-byte buffer overflow in `TextField::begin()`) was
  found and fixed by hand-tracing before that flash, which is a concrete reason to still be a
  little more careful than usual with this phase's code -- see PROGRESS.md's Phase 3 "Real bug
  found during tracing". An independent second-pass review (via a subagent) was attempted but
  didn't complete before the org's monthly spend limit was hit -- only one pass of scrutiny plus
  one informal device test has actually happened on this phase's code, not a full checklist run.
- `TextField`'s `KEY_PREV`-advances-cursor gesture mapping (see `FormField.h`) is logically
  reasoned through but has never been tried by a human finger -- WioTrackerL1's joystick input
  can't exercise the single-button-specific concern even if the Advert Name field was opened
  during testing, since it has `KEY_CANCEL` from a dedicated back button. Flag for
  reconsideration on `RAK_4631` once it can actually be typed on there.
- **Phase 4 (Diagnostics) has been flashed and informally tested on `WioTrackerL1`** -- see
  PROGRESS.md's Phase 4 "Real-device findings" for the full account. Diagnostics content itself
  wasn't flagged as wrong; a real status-bar-flash bug was found and fixed (see the "Compositing
  and `status_due`" note above); the pre-existing status-bar/menu overlap (Phase 2) was reconfirmed
  present; and a message-preview auto-dismiss-to-Home behavior was confirmed (user had the phone
  app connected via BLE during the test) to trace to the companion app syncing the message almost
  immediately via `CMD_SYNC_NEXT_MESSAGE` -> `UITask::msgRead(0)` -> `gotoHomeScreen()` --
  pre-existing `ui-new` interrupt behavior (ported in Phase 1), not new to this phase. Left
  unfixed by explicit user request ("note it so it can be fixed later") -- see PROGRESS.md's Phase
  4 finding 3 for the backlog note and possible directions. Not a formal checklist pass, and
  `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` have had none at all.
- The Diagnostics screens' single most important claim -- that the values shown match the
  companion app's stats view -- has only been checked by reading the same wire-protocol code the
  phone app parses (`STATS_TYPE_CORE`/`_RADIO`/`_PACKETS` in `MyMesh.cpp`), not by an actual
  side-by-side device comparison, even after the `WioTrackerL1` pass above.
- Event log entries (`Screen_EventLog`) haven't been specifically confirmed on a real screen (not
  flagged as wrong, but not specifically called out either) -- worth deliberately triggering
  several event types in sequence (advert, radio param change, message) and confirming newest-first
  ordering, the 20-entry drop-oldest behavior, and the "Ns ago" age counting correctly across
  renders.
- The status bar's transport label change (`"BLE:%s"` -> `UI_FOREST_TRANSPORT_NAME ":%s"`) is a
  no-op in practice today -- every existing forest env is BLE -- so it hasn't actually been
  exercised against a non-BLE build.
- `phases/completed/phase-4-diagnostics.md`'s stated prerequisite ("Phase 3 added a Diagnostics `Submenu`
  placeholder to Home") did **not** hold -- `phase-3-settings.md`'s own "What to build" list never
  asked for one. Resolved by building the Home entry and its real content in this phase in one
  step, rather than the originally-planned placeholder-then-content across two phases -- see
  PROGRESS.md's Phase 4 section.
- **Phase 5 is entirely build-unverified**, same limitation as every prior phase (no `pio`/`g++` on
  PATH in this dev environment) -- traced by hand against the real `Adafruit_GFX`/`GxEPD2` headers
  and all ten `DisplayDriver` backends instead. Specifically not yet confirmed on real hardware:
  the text-wrap fix actually eliminates the status-bar/menu overlap on `WioTrackerL1` (the one board
  it was device-reported on); the fixed `StatusBar` battery-gauge clear actually stops the
  text/icon mixing on a real marquee scroll; the new `Layout::iconRowHeight()`/`visibleIconRows()`
  math actually renders `Screen_Contacts`/`Screen_Channels` icons without clipping/misalignment; the
  `MenuItem` tint change doesn't visually clash with the selected-row highlight on
  `Screen_SettingsDanger`; the new `supportsColor()` overrides actually match each board's real
  capability (`heltec_rc32` should still show red/green/yellow, monochrome pilots should now show
  every accent color as plain white/on); and `lilygo_techo` (the new e-ink pilot,
  `LilyGo_T-Echo_companion_radio_forest_ble`) has never been flashed with `ui-forest` at all -- this
  would be its first hardware pass for everything built since Phase 0, not just this phase's
  additions. Per the user's own correction to
  this phase's spec, `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` are in the same position: despite
  phase-5-visual-polish.md's "Prerequisites" section calling their Phase 5 pass "a regression sweep",
  none of the three has been physically tested on *any* phase 0-4 either -- Phase 5 is their first
  hardware pass for everything, not a regression check.
- **Phase 6's shared-file changes regression-cleared on `WioTrackerL1`, but its two actual pilot
  boards (`lilygo_tdeck`/`sensecap_indicator-espnow`) have never run any phase of `ui-forest` at
  all, and the user has no hardware for either.** See PROGRESS.md's Phase 6 section for the full
  account, including per-item confidence levels -- the `WioTrackerL1` pass only confirms this
  phase didn't break already-working boards (neither `HAS_TOUCH` nor `LILYGO_TDECK` compiles in
  on it), it says nothing about whether touch/keyboard/trackball actually work. In short: dynamic
  hints (item 32) are a low-risk refactor of already-working code; touch (item 33) is reasoned
  through by reading `LGFXDisplay.cpp`/`SCIndicatorDisplay.h` but not finger-tested; keyboard+
  trackball (item 34) rests on published-but-unverified-in-this-repo pin assignments and I2C
  protocol for `lilygo_tdeck` specifically, and is the most likely part of this phase to need a
  real-hardware correction whenever that board becomes available to test on.
  `sensecap_indicator-espnow`'s forest env also reports the wrong transport name ("USB" instead
  of "ESPNOW") via a pre-existing `Transport.h` gap this phase surfaced but deliberately didn't
  fix (out of scope, cosmetic-only).
- **Phase 7 is entirely build-unverified and unflashed** (same no-`pio`/no-`g++` limitation as every
  prior phase, but unlike most of them, no informal `WioTrackerL1` pass has happened yet either) --
  see PROGRESS.md's Phase 7 section for the full account. Per-event-type notification config
  (`NotificationPrefs`) is UI-local/non-persisted and resets to all-on every reboot, same shape as
  the still-open Phase 3 vibration-persistence gap. `ack` events are deliberately not fed into
  `Screen_RecentEvents` despite phase-7-notifications.md's own checklist naming them, since every
  real `notify(UIEventType::ack)` call site in this codebase is a generic UI-action confirmation
  tone, not a real mesh delivery ack (see PROGRESS.md's Phase 7 "Decisions"). A pre-existing (not
  Phase-7-introduced) `MenuScreen`/`_has_icons` construction-order concern was also noticed while
  tracing this phase's code -- flagged in PROGRESS.md but not fixed, since it spans every
  `MenuScreen` subclass back through Phase 5, not just this phase's additions.
- **Phase 8 widened board coverage by one (`ThinkNode_M1`, a `GxEPDDisplay` e-ink board), entirely
  build-unverified** (no hardware for this board, no `pio`/`g++` in this dev environment) -- see
  PROGRESS.md's Phase 8 section for the full account, including a hand-traced regression pass of
  Phase 7's diff against `WioTrackerL1`'s specific macro set (found nothing that looks like a
  regression, but is a paper trace, not a reflash). **This repo has exactly one LGFX/LovyanGFX
  RGB-panel board (`sensecap_indicator-espnow`, Phase 6's pilot) and no second one to widen coverage
  to** -- confirmed by grep (`helpers/ui/LGFXDisplay.cpp` usage, `public LGFXDisplay`/`LGFX_Device`/
  `lgfx::` references, and a full listing of `src/helpers/ui/`), not assumed; this is a real gap in
  available board variants, not a skipped task. `ThinkNode_M1` is the first `ui-forest` board across
  all 8 phases to define `PIN_STATUS_LED` -- `UITask::userLedHandler()`'s active-low polarity handling
  was traced against it and found correct, but still nobody has seen the LED actually blink. The
  migration/deprecation decision for `ui-forest` (which boards, if any, get it as their default,
  and on what timeline) is deliberately NOT decided here -- see `MIGRATION-DECISION.md` (same
  directory), which lays out the tradeoffs and asks the user rather than recommending one unilaterally.
