# ui-forest — as-built architecture

Describes the code as it exists after Phase 1, not the aspirational design in `PLAN.md`.
Where this file and `PLAN.md` disagree, this file wins for "what's actually there today" —
`PROGRESS.md` explains why, when the difference was a deliberate call. Read this before
starting Phase 2 so new screens plug into the existing shape instead of reinventing it.

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
if c != 0:  NavStack::current()->handleInput(c)  -- screen may push/pop/mutate itself
userLedHandler(); buzzer.loop()                   -- time-driven, every iteration
NavStack::current()->poll()                      -- time-driven screen state, every iteration
if display on:
  showing_splash = (NavStack::current() == splash)
  if !showing_splash: updateStatusBar()           -- rebuild scrolling text + battery/mute state
  if content_due || status_due || toast.isShowing():
    display->startFrame()
    NavStack::current()->render(display)          -- the active screen draws itself
    if status_due: StatusBar::render(display)      -- composited on top, suppressed during splash
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
- `task.handleDoubleClick(KEY_PREV)` / `task.handleTripleClick(KEY_SELECT)` -- single-button
  and analog-button only. Double-click just re-applies the display-wake side effect (matches
  ui-new, including its quirk of discarding `checkDisplayOn`'s return value -- ported as-is).
  Triple-click **always** toggles the buzzer and **always** consumes the event (`c = 0`) --
  it never reaches `NavStack::current()->handleInput()`, on any board. This is ui-new's real
  behavior, not the more hedged "screen-dependent SELECT" wording in PLAN.md §3.1 -- see
  PROGRESS.md's Phase 1 "Decisions" section for why the literal port won this one.

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

`MenuItemKind` has 7 values today: `Action` and `Submenu` are fully wired (`activate()`
invokes the callback or pushes the submenu screen); `Toggle`/`Stepper`/`Enum`/`Text` exist as
enum values only, with a documented no-op in `activate()` — Phase 3 will give them real
behavior by pushing a `FormField`-based editor screen instead of extending this switch
statement's shape. `Info` is a placeholder for Phase 4 stat rows.

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
`UITask::shutdown()`. Still the single shared instance described in PLAN.md 3.2 (Phase 3's
danger-zone settings will reuse the same `UITask::_confirm` object, not construct their own).

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

### `Screen_MsgPreview`

Straight port of `MsgPreviewScreen`. `UITask::newMsg()` calls `addPreview()` then
`nav.reset(_msg_preview)` -- a full stack replacement, not a `push()`, matching ui-new's own
`setCurrScreen(msg_preview)` interrupt behavior (new messages always yank the display away
from whatever was showing, discarding any submenu context). Dismissing (`KEY_ENTER`/
`KEY_SELECT`/`KEY_CANCEL`/`KEY_PREV`, or paging through all unread via `KEY_NEXT`/`KEY_RIGHT`
until none remain) calls `nav.reset(_home)`, so it always lands back at Home root, never at
whatever screen was showing before the interrupt -- again matching ui-new exactly.

### `UITask`

Owns one instance of every framework class above, plus every Phase 1 screen and the real Home
`MenuItem[]` table (sized `UI_FOREST_HOME_ITEM_COUNT` = 8, the worst case with GPS+Sensors
both present; `_home_item_count` tracks how many slots are actually used on this board).
`begin()` constructs every leaf screen first, fills the item table with `MenuItemKind::Submenu`
entries pointing at them, then constructs `_home` (a `MenuScreen`), `_msg_preview`, and
`_splash` in that order (each later one needs a pointer to something built earlier), and calls
`nav.reset(_splash)`. `loop()` is the render loop described above, now also driving
`InputRouter`'s callback methods (`checkDisplayOn`/`handleLongPress`/`handleDoubleClick`/
`handleTripleClick`), `userLedHandler()`, buzzer/vibration `loop()`, auto-off, and the
`AUTO_SHUTDOWN_MILLIVOLTS` low-battery shutdown check -- all ported from `ui-new`'s `loop()`
tail. `msgRead`/`newMsg`/`notify` (the `AbstractUITask` virtuals `MyMesh` calls) now have real
behavior: `notify()` plays buzzer tones/triggers vibration per `UIEventType`, `newMsg()` pushes
`Screen_MsgPreview`, `msgRead(0)` returns to Home.

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
| `PIN_STATUS_LED` | status LED pin | `UITask::userLedHandler()` (compiled, not hardware-verified -- no Phase 1 pilot has it) |
| `ENV_INCLUDE_GPS` | board has a GPS module | `UITask` (gates constructing/wiring `Screen_Gps`; `heltec_rc32`'s forest env sets this) |
| `UI_SENSORS_PAGE` | board wants the CayenneLPP sensor-scroll screen | `UITask` (gates constructing/wiring `Screen_Sensors`; `WioTrackerL1`'s forest env sets this) |
| `AUTO_OFF_MILLIS` | display auto-off delay (default 15000, `0` disables) | `UITask::loop()` |
| `AUTO_SHUTDOWN_MILLIVOLTS` | battery voltage that triggers auto-shutdown | `UITask::loop()` (`heltec_rc32`'s forest env sets this to 3400) |
| `KEEP_DISPLAY_ON_USB` | opt-in: don't auto-off while externally powered | `UITask::loop()` |

## Rollout mechanism

Each pilot board gets one new `_companion_radio_forest_ble` env in its own
`variants/<board>/platformio.ini`, `extends`-ing the board's base section (not the existing
`_ble` env) and restating `build_flags`/`build_src_filter`/`lib_deps` from scratch with two
lines changed: the `-I` path and the `ui-*/*.cpp` glob. Existing `_ble`/`_usb`/`_wifi` envs
for every board are untouched. See any of the four `variants/*/platformio.ini` files' forest
env for the exact template; `PROGRESS.md` lists which four boards have one today.

## Known gaps carried into Phase 2

- Home is a flat 8-entry `MenuScreen` with no further nesting -- Contacts/Channels browsing
  and the `KEY_HOME` jump-to-root gesture (PLAN.md items 16-18) aren't built yet.
- No Settings or Diagnostics menus (Phase 3/4).
- `StatusBar` always renders its scrolling text as text; no per-board icon-vs-text switch at
  `display.height() > ~80px` yet (mentioned in PLAN.md §3.2, not built -- Phase 5 polish).
- No screen-transition animation, expanded icon set, or consistent color-convention sweep
  (Phase 5).
- `HAS_TORCH` and `PIN_STATUS_LED` code paths are compiled but not hardware-verified (no
  Phase 1 pilot board defines either).
- `ConfirmScreen`'s auto-fire-after-N-seconds design (vs. ui-new's press-and-hold) hasn't been
  validated on real hardware yet -- `Screen_Shutdown` is its first caller.
- Nothing in this codebase has been build-verified in this dev environment (no `pio` on
  PATH) -- see PROGRESS.md.
