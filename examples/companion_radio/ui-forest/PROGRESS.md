# ui-forest — progress log

Tracks what's actually been built and verified, phase by phase. See `PLAN.md` for the
full spec and `ARCHITECTURE.md` for how the as-built code fits together. Update this file
at the end of each phase (or when a phase's plan changes) before starting the next one.

## Phase 0 — Framework skeleton: DONE (build-verified on 4 boards, device-verified on 1)

Spec: `phases/phase-0-framework-skeleton.md`.

### Built

All new, flat in `examples/companion_radio/ui-forest/` per PLAN.md §4:

- `NavStack` (header-only) — fixed-depth (8) `UIScreen*` stack: `push`/`pop`/`popToRoot`/`reset`/`current`
- `Layout` (header-only) — `rowHeight()`/`headerHeight()`/`statusBarHeight()`/`visibleRows()`
- `InputRouter` — single-button / joystick / rotary gesture-to-key mapping
- `MenuScreen` + `MenuItem`/`MenuItemKind` — generic list menu (only `Action` is live)
- `ConfirmScreen` — timed arm-then-confirm gate (see "Deviations from PLAN.md" below)
- `ToastOverlay` (header-only) — alert box over the current screen
- `StatusBar` — top strip, static placeholder text only
- `Screen_Splash` — logo/version, dismisses into the placeholder Home menu
- `icons.h` — meshcore logo only (copied from `ui-new/icons.h`)
- `UITask` — wires all of the above; placeholder 3-item Home menu whose items toast their own label

### Envs added (existing `_ble`/`_usb` envs untouched)

| Env | Board file | Input scheme | Build |
|---|---|---|---|
| `RAK_4631_companion_radio_forest_ble` | `variants/rak4631/platformio.ini` | single button | ✅ builds |
| `GAT562_30S_Mesh_Kit_companion_radio_forest_ble` | `variants/gat562_30s_mesh_kit/platformio.ini` | joystick (left/right + back) | ✅ builds |
| `heltec_rc32_companion_radio_forest_ble` | `variants/heltec_rc32/platformio.ini` | rotary + button | ✅ builds |
| `WioTrackerL1_companion_radio_forest_ble` | `variants/wio-tracker-l1/platformio.ini` | joystick (4-way + press + back) | ✅ builds, ✅ **flashed and confirmed working on real hardware** |

`WioTrackerL1` wasn't one of PLAN.md's three pilot boards — it was added because it's the
device the user actually has on their desk for testing. Its joystick input scheme
(dedicated Up/Down/Left/Right pins + press + separate back button) is a superset of the
GAT562 pilot's (left/right + back only), so it's a reasonable stand-in for joystick
coverage and has the advantage of actually being tested on hardware.

### Real-device findings (from flashing WioTrackerL1)

1. **`AbstractUITask.h` pulls in `buzzer.h` whenever `PIN_BUZZER` is defined, regardless of
   UI variant.** GAT562/heltec_rc32/WioTrackerL1 all define `PIN_BUZZER`, so their forest
   envs need `+<helpers/ui/buzzer.cpp>` and the `NonBlockingRTTTL` lib dep even though
   `ui-forest`'s own code never touches the buzzer yet (that's Phase 1). Missing this
   causes a `NonBlockingRtttl.h: No such file` build error. Fixed in all three envs.
2. **WioTrackerL1's physical joystick has real Up/Down pins that were never wired up.**
   `variants/wio-tracker-l1/variant.h` defines `JOYSTICK_UP`/`JOYSTICK_DOWN` (pins 25/26,
   labeled "Joystick Up"/"Joystick Down" in the pin comments), but `target.cpp` only ever
   instantiated `joystick_left`/`joystick_right` (pins 27/28) — Up/Down were declared as
   macros but never turned into `MomentaryButton`s. This is a pre-existing gap in the
   board's shared `target.h`/`target.cpp` (used by every UI variant, not just ui-forest),
   not a `ui-forest` mapping bug. Symptom reported by the user: pressing the stick felt
   "twisted" — physical Up/Down did nothing, only Left/Right worked, so vertical menu
   navigation ended up mapped onto the horizontal axis.
   **Fix:** added `joystick_up`/`joystick_down` `MomentaryButton` globals to
   `variants/wio-tracker-l1/target.h`/`.cpp`, and taught `InputRouter::poll()` to emit
   `KEY_UP`/`KEY_DOWN` from them, gated on `#if defined(JOYSTICK_UP) && defined(JOYSTICK_DOWN)`
   so boards without real up/down pins (GAT562) are unaffected. `MenuScreen` already treated
   `KEY_UP`/`KEY_DOWN` identically to `KEY_LEFT`/`KEY_RIGHT`, so no menu-side change was
   needed. User confirmed the fix on hardware.

### Deviations from PLAN.md / phase-0.md (intentional, worth knowing about)

- **`ConfirmScreen` is a timed arm-then-auto-confirm window, not raw `isButtonPressed()`
  polling.** PLAN.md's reference model (`ui-new`'s shutdown flow) polls a single button's
  raw pressed/released state directly on `UITask`, which only works for single-button
  boards. Since `ConfirmScreen` needs to work identically across single-button/joystick/
  rotary boards without new per-board hardware hooks, it instead: `begin()` arms a
  `millis()` deadline, `poll()` fires the action automatically when the deadline passes,
  and `KEY_CANCEL` aborts. Not yet exercised by any real screen — first caller is Phase 1's
  shutdown screen or Phase 3's danger-zone settings, at which point re-validate this design
  actually feels right on hardware (a hold gesture might read better than an auto-fire
  countdown; easy to swap since there's still no caller).
- **`StatusBar` is built and seeded with placeholder text in `UITask::begin()`, but
  `MenuScreen` needs a `status_bar_shown` constructor flag to reserve space for it** (see
  `ARCHITECTURE.md` for how the offset math works). This wasn't explicitly speced in
  phase-0.md but was necessary to avoid the status bar and menu header drawing on top of
  each other at y=0.
- **Single-button long-press maps straight to `KEY_ENTER`, no CLI-rescue carve-out yet.**
  PLAN.md §3.1 mentions "CLI rescue in first 8s" as part of the overall vocabulary, but
  that's explicitly Phase 1 (item 9) and touches `MyMesh`/`the_mesh`, which Phase 0 has zero
  dependency on. `InputRouter` in Phase 0 has no mesh dependency at all.
- **No auto-off, no buzzer/vibration/LED, no boot-time CLI rescue.** All explicitly deferred
  to Phase 1 per phase-0.md; `UITask`'s `msgRead`/`newMsg`/`notify` overrides are no-ops for
  now (just cache `_msgcount`).

### Not yet done before Phase 0 can be called fully signed off

- `RAK_4631` and `gat562_30s_mesh_kit` (2 of PLAN.md's 3 named pilots) are **build-verified
  only** — nobody has flashed and manually run the checklist in phase-0.md §"Done-when" on
  actual hardware for those two yet. Only `WioTrackerL1` (single-button... no, joystick)
  has been physically tested.
- `heltec_rc32` (rotary pilot) is build-verified only, not device-tested.
- The manual test checklist template (PLAN.md §9 / phase-0.md's filled-in copy) hasn't been
  formally run/recorded for any board yet, even WioTrackerL1 — the WioTrackerL1 test so far
  was informal ("works great" + the joystick bug report), not a checklist pass.
- Native unit tests (`pio test -e native`) could not be run in this dev environment (no
  `g++` on PATH here) — unrelated to ui-forest, since Phase 0 doesn't touch
  `src/Utils.cpp`/`Packet.cpp`, but worth running in CI/another machine before merging.

## Phase 1 — Parity with ui-new (+ ui-tiny's best bits): BUILD NOT YET VERIFIED (no `pio` in this dev environment)

Spec: `phases/phase-1-parity-with-ui-new.md`. Code for every item in that spec has been
written; nobody has run `pio run` or flashed a board for Phase 1 yet (this dev environment
has no `pio` on PATH, same limitation noted at the end of Phase 0). **The user will build
and flash `WioTrackerL1_companion_radio_forest_ble` next** -- treat Phase 1 as code-complete
but hardware-unverified until that happens.

### Built

All new files, flat in `examples/companion_radio/ui-forest/` per PLAN.md §4:

- `Screen_Status` -- today's `HomePage::FIRST` (msg count, connection/BLE pin). Battery icon
  + mute overlay moved out of this screen and into `StatusBar` (see below).
- `Screen_Recents` -- recently-heard list via `the_mesh.getRecentlyHeard()`, read on demand
  into a stack-local array each render (never a member, so the header doesn't need
  `MyMesh.h`'s `AdvertPath` definition).
- `Screen_RadioInfo` -- freq/sf/bw/cr/tx power from `NodePrefs` + noise floor from
  `radio_driver`.
- `Screen_Bluetooth` -- BLE on/off icon, `KEY_ENTER`/`KEY_SELECT` toggles serial enable.
- `Screen_Advert` -- advert icon, `KEY_ENTER`/`KEY_SELECT` sends an advert and toasts the
  result.
- `Screen_Gps` (`#if ENV_INCLUDE_GPS == 1`) -- fix/sat/pos/alt, toggles GPS on ENTER.
- `Screen_Sensors` (`#if UI_SENSORS_PAGE == 1`) -- scrolling CayenneLPP dump. Its `KEY_ENTER`
  handler calls `toggleGPS()`, not a GPS-page mixup -- that really is what ui-new's
  `HomePage::SENSORS` handler does
  (`examples/companion_radio/ui-new/UITask.cpp:451-457`), ported as-is.
- `Screen_Shutdown` -- power icon, `KEY_ENTER`/`KEY_SELECT` arms `ConfirmScreen` (first real
  caller -- see "Decisions / deviations" below).
- `Screen_MsgPreview` -- unread-message viewer, straight port of `MsgPreviewScreen`. `UITask`
  pushes it via `nav.reset()` (not `push()`) whenever a new message arrives, matching
  ui-new's flat "replace whatever's showing" interrupt behavior; dismissing goes back to
  Home root the same way.
- `StatusBar` gained `setBattery(milliVolts, muted)` and now draws a battery-percentage
  gauge + muted overlay (ported from `ui-new`'s `HomeScreen::renderBatteryIndicator()`) on
  every screen, not just Home. `UITask::updateStatusBar()` rebuilds the scrolling text every
  loop() with node name/buzzer/GPS/BLE state, mirroring `ui-tiny`'s `ScrollingStatusBar::
  update()` format. Suppressed while `Screen_Splash` is showing (the splash logo occupies
  the same top rows the strip does -- see "Decisions" below).
- `InputRouter` gained the display-wake-on-press / boot-CLI-rescue / triple-click-mute side
  effects ui-new applies inline in `UITask::loop()`
  (`examples/companion_radio/ui-new/UITask.cpp:862-894`, methods `checkDisplayOn`/
  `handleLongPress`/`handleDoubleClick`/`handleTripleClick`) -- these now live as public
  `UITask` methods that `InputRouter::poll(UITask&)` calls back into, since only
  `InputRouter` still has the raw click/double/triple/long-press event type at the point
  those side effects need to apply (see ARCHITECTURE.md). Also added a `#if defined(HAS_TORCH)`
  branch (item 17) -- compiles, not hardware-tested, no Phase 1 pilot has `HAS_TORCH`.
- `UITask` gained: buzzer/vibration instances + `notify()` playing real tones
  (item 6), `userLedHandler()` heartbeat (item 7, `#ifdef PIN_STATUS_LED` --
  no Phase 1 pilot defines this pin either, compiles-but-unverified same as torch),
  auto-off + `AUTO_SHUTDOWN_MILLIVOLTS` low-battery shutdown (item 8, ported from
  `ui-new`'s `loop()` tail), `shutdown()`/`toggleBuzzer()`/`getGPSState()`/`toggleGPS()`
  (straight ports), and the Home menu is now built from real `MenuItem`s (`kind =
  MenuItemKind::Submenu`) pointing at each screen above, instead of Phase 0's 3-item
  toast-only placeholder.

### Decisions / deviations worth knowing about

- **Single-button "back" gap, resolved by widening `KEY_PREV`'s meaning.** ui-new's
  single-button vocabulary (click=NEXT, double=PREV, triple=SELECT, long=ENTER) has no
  "cancel/back" gesture at all -- it never needed one, since ui-new has no screen stack, just
  a flat page ring. `ui-forest`'s menu hierarchy does need one, and `RAK_4631` (a Phase 1
  pilot) has no dedicated back button. Fix: every Phase 1 leaf content screen treats
  `KEY_PREV` identically to `KEY_CANCEL` (both pop the nav stack) -- safe because a leaf
  screen has no internal list for PREV to mean "move up" in the first place. `MenuScreen`
  itself is unaffected (`KEY_PREV` still moves the selection cursor there, where it does mean
  something). Joystick/rotary boards are unaffected too (their dedicated `back_btn` already
  produces `KEY_CANCEL`, and rotary/joystick never emit bare `KEY_PREV`).
- **Triple-click stays a global, unconditional mute toggle, not a screen-dispatched
  `KEY_SELECT`.** PLAN.md §3.1 hedges triple-click as "SELECT (context menu / mute,
  screen-dependent)", but ui-new's actual `handleTripleClick` always toggles the buzzer and
  always consumes the event (`c = 0`), regardless of which page is showing
  (`examples/companion_radio/ui-new/UITask.cpp:888-894`). Ported literally: `InputRouter`
  calls `task.handleTripleClick()` for every triple-click path (single-button, analog,
  joystick back-button), which never forwards to the current screen. `MenuScreen` still
  accepts `KEY_SELECT` as an activate-item synonym for `KEY_ENTER`, but nothing currently
  produces a forwarded `KEY_SELECT` on the boards in the Phase 1 pilot set (joystick's
  back-button triple-click is the only source, and it's consumed for mute before reaching
  the nav stack) -- revisit if a future board's input mapping changes this.
- **`ConfirmScreen` now has its first real caller (`Screen_Shutdown`).** Still the Phase 0
  timed arm-then-auto-fire design (not press-and-hold) -- validate on hardware whether a
  3-second unattended countdown feels right for shutdown, or whether it should require an
  explicit re-confirm keypress instead. Easy to change since there's exactly one caller.
- **`StatusBar` is suppressed during `Screen_Splash`.** All other screens reserve
  `Layout::statusBarHeight(true)` at the top and draw below it; splash draws its logo across
  those same rows, so showing both at once would overlap. `UITask::loop()` special-cases this
  with an identity check against the splash screen pointer.
- **`Screen_Gps.cpp`/`Screen_Sensors.cpp` always compile, even on boards without
  `ENV_INCLUDE_GPS`/`UI_SENSORS_PAGE`.** PLAN.md §4 chose a flat file layout specifically to
  avoid relying on `build_src_filter` directory-glob recursion (see PLAN.md §8), which means
  every `.cpp` in this folder is compiled for every forest env regardless of board flags --
  only their *instantiation* in `UITask::begin()` is `#if`-gated. This was already implicitly
  true in Phase 0; Phase 1 just makes it matter (adds two real files that are dead weight on
  boards without those flags). Worth checking flash headroom on `RAK_4631` (the tightest pilot)
  once a build is available -- PLAN.md §3.6 says trim there first if it doesn't fit.

### Not yet done / needs hardware verification

- No `pio run` in this environment -- **zero Phase 1 code has been build-verified**, let alone
  flashed. This is the first thing to do before trusting anything else in this section.
- Manual test checklist (`phases/phase-1-parity-with-ui-new.md`'s "Done-when" block) not run
  on any board yet.
- `HAS_TORCH` and `PIN_STATUS_LED` branches are compile-shaped ports with no hardware to
  verify them against (no Phase 1 pilot has either pin defined).
- GPS/Sensors screens: `heltec_rc32`'s forest env defines `ENV_INCLUDE_GPS=1`, and
  `WioTrackerL1`'s defines `UI_SENSORS_PAGE=1`, so both are technically reachable on already-
  wired pilot boards -- neither has been exercised on real hardware yet under `ui-forest`.
- `ConfirmScreen`'s auto-fire timing (see Decisions above) needs a real hold-it-in-your-hand
  judgment call once shutdown can actually be tried on a board.
