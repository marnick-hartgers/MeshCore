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

## Phase 2 — Navigation & data browsing: DONE (device-verified on WioTrackerL1)

Spec: `phases/phase-2-navigation-and-data-browsing.md`. Code was written and manually traced
against the actual headers (`BaseChatMesh.h`, `ContactInfo.h`, `ChannelDetails.h`,
`AdvertDataHelpers.h`, `DisplayDriver.h`, `Utils.h`) instead of a real compile in this dev
environment (no `pio`/`g++` on PATH here, same limitation as Phases 0/1). The user then built
and flashed `WioTrackerL1_companion_radio_forest_ble` and confirmed it works: Home menu
restructuring, Contacts/Channels browsing, and the KEY_HOME jump-to-root gesture all behave as
intended on real hardware. One minor visual bug was found (see "Real-device findings" below);
the user judged it not important to fix right now, so it's carried forward rather than blocking
Phase 3.

### Built

All new, flat in `examples/companion_radio/ui-forest/` per PLAN.md §4:

- `Screen_Contacts` -- list screen driven by `the_mesh.getNumContacts()` /
  `the_mesh.getContactByIdx(idx, ContactInfo&)`. Reads one contact at a time, only for the rows
  currently on screen (via `Layout::visibleRows`) -- never copies the whole contact table into a
  buffer (PLAN.md 3.6's specific correctness concern for this screen; spot-checked by re-reading
  the render loop before calling this done). `KEY_ENTER`/`KEY_SELECT` pushes
  `Screen_ContactDetail` with the selected index; `KEY_CANCEL` pops back to Home.
- `Screen_ContactDetail` -- pushed via `show(idx)` + `nav.push()`. Re-reads the contact by index
  from `the_mesh` on every `render()` (no cached copy, same on-demand discipline as the list
  screen). Shows name as the header row, then a `Layout`-driven scrollable field list (Type,
  Path, Seen, GPS, ID) -- see "Decisions" below for why this is a scrollable list rather than a
  fixed pixel layout like `Screen_RadioInfo`. `ADV_TYPE_*` maps to a plain string switch (Chat/
  Repeater/Room/Sensor/Unknown) -- confirms the Section-8/phase-2.md open question that the enum
  is small enough for a switch, no helper needed. Pub key shown as a 6-byte hex prefix
  (`mesh::Utils::toHex(..., contact.id.pub_key, 6)`), matching the `pubkey_prefix` convention
  used elsewhere in the codebase (e.g. `simple_repeater/MyMesh.cpp`), not the full 32-byte key.
- `Screen_Channels` -- iterates `the_mesh.getChannel(idx, ChannelDetails&)` over
  `0..MAX_GROUP_CHANNELS-1`, skipping slots where `name[0]==0`. No per-channel detail/edit
  screen this phase (matches PLAN.md's non-goals -- editing is a stretch goal). Selection cursor
  is kept in "filtered list" index space (0-based, non-empty slots only) and mapped back to the
  underlying raw slot via a small linear scan each lookup -- cheap given `MAX_GROUP_CHANNELS=40`
  on every pilot env, and still never materializes more than one `ChannelDetails` on the stack
  at a time.
- `Screen_Home` restructuring -- Home's `MenuItem` table gained two entries ("Contacts",
  "Channels") between "Advert" and the optional GPS/Sensors entries.
  `UI_FOREST_HOME_ITEM_COUNT` bumped from 8 to 10 to fit the new worst-case (all optional
  screens + both new entries) item count.
- `KEY_HOME` (jump-to-root) wiring -- see "Decisions" below for the mechanism; net effect is
  every pilot board's input scheme now has some way to jump back to Home from arbitrarily deep
  in the nav stack, not just one level via `KEY_CANCEL`.

### Decisions / deviations worth knowing about

- **KEY_HOME is emitted by repurposing the existing triple-click gesture below Home, not by a
  new raw gesture in `InputRouter`.** PLAN.md 3.1 suggests "a long-press-anywhere ... maps to
  KEY_HOME", but long-press is already claimed for `KEY_ENTER` on every board scheme as of
  Phase 1 (single-button's only activate gesture, joystick/rotary's post-boot-CLI-rescue
  fallback) -- stealing it for KEY_HOME would remove the only way single-button boards activate
  anything. Single-button/analog/rotary boards also have no spare gesture at all: all four
  `MomentaryButton` event types (click/double/triple/long) are already claimed
  (NEXT/PREV/mute/ENTER). PLAN.md's own parenthetical -- "(or, on joystick boards, the
  back-button triple-click that today only exists for mute)" -- describes a gesture that, in the
  as-built Phase 1 code, isn't actually joystick-specific: single-button, analog, *and* joystick
  boards all route triple-click through the same `UITask::handleTripleClick()` method. So Phase 2
  changes that one method instead of touching `InputRouter`'s key tables: triple-click now checks
  `_nav.depth()` -- at Home (`depth() == 1`) it keeps ui-new's original always-mute behavior
  (nothing to jump home from); below Home it returns `KEY_HOME` instead, which `UITask::loop()`
  intercepts before dispatch and turns into `_nav.popToRoot()`. This covers every pilot board's
  input scheme with zero `InputRouter` changes, and resolves what would otherwise be a real
  hardware gap: single-button/analog/rotary boards have no raw `KEY_CANCEL` source at all (only
  joystick's dedicated `back_btn` emits one), so without this, a single-button user drilling into
  Contacts/Channels (both real list screens now, unlike Phase 1's leaf screens which sidestepped
  the issue by aliasing `KEY_PREV` to cancel) would have had no way back out short of a power
  cycle. Mute becomes reachable only from Home on those boards as a result -- an acceptable
  trade given the alternative was being stuck.
- **`Screen_ContactDetail` is a `Layout`-driven scrollable field list, not a fixed pixel layout
  like `Screen_RadioInfo`.** The straightforward "one field per row, `y += 11`" approach (as used
  by `Screen_RadioInfo`/`Screen_Gps`) doesn't fit: Name+Type+Path+Seen+GPS+ID is 6 rows, and a
  64px OLED with the status bar reserved only fits ~4-5 rows total before clipping off the
  bottom edge -- confirmed by running `Layout`'s own math (`(64-11-11)/11 = 3` visible rows).
  Resolved by treating Name as the screen's header (drawn once, ellipsized, with the same
  title-row/separator convention `MenuScreen` uses) and the remaining 5 fields as a
  `Layout::visibleRows()`-scrolled list below it, with `KEY_NEXT`/`KEY_PREV` scrolling instead of
  moving a selection cursor (there's nothing to select -- every row is read-only). This is the
  first screen in ui-forest to actually need `Layout`'s adaptive-row-count math for a *non*-menu
  screen, which is a good sign the abstraction generalizes the way PLAN.md 3.3 intended.
- **`Screen_Contacts`/`Screen_Channels` list rows are unstyled plain rows (no icon), unlike
  `MenuScreen`'s optional 16x16 icon slot.** No contact-type icon exists yet (PLAN.md 3.5 lists
  "contact"/"channel" icons as Phase 5 work) -- deferred rather than hand-authoring placeholder
  icons a phase early.
- **`Screen_ContactDetail`'s `KEY_ENTER`/`KEY_SELECT` also pop back to `Screen_Contacts`,** in
  addition to `KEY_CANCEL`. There's no primary action on a read-only detail screen for ENTER to
  do, so mapping it to "go back" (rather than leaving it unhandled) gives single-button testers
  an obvious, low-risk way out of the detail view without waiting to learn the triple-click/
  KEY_HOME escape hatch.

### Real-device findings (from flashing WioTrackerL1)

1. **`StatusBar` overlaps some screen content instead of every screen reserving clean space for
   it.** User-reported: the status bar strip visually falls over some UI elements rather than
   sitting cleanly above them. Every screen's `render()` computes its top offset via
   `Layout::statusBarHeight(true)`, so the *height* being reserved is consistent -- the overlap is
   more likely a specific screen (or `MenuScreen`'s selection-row `fillRect`/a leaf screen's
   content) drawing at or above that offset rather than strictly below it, or a screen not
   accounting for the header row's own height on top of the status bar's. Not diagnosed further
   yet -- user judged it not important to chase right now. **Carried forward, not blocking
   Phase 3**; worth a closer look whenever a screen's layout is next touched, or as part of
   Phase 5's visual-polish pass (consistent header/status-bar handling is explicitly in scope
   there per PLAN.md item 15).

### Not yet done / needs hardware verification

- `RAK_4631` and `gat562_30s_mesh_kit` (2 of PLAN.md's 3 named pilots) are still **build-verified
  only** for Phase 2 -- only `WioTrackerL1` has been physically tested, same gap as Phase 0/1.
- `heltec_rc32` (rotary pilot) is still build-verified only, not device-tested.
- Manual test checklist (`phases/phase-2-navigation-and-data-browsing.md`'s "Done-when" block)
  hasn't been formally run/recorded on any board -- the WioTrackerL1 pass so far was informal
  ("works great" + the status bar report), not a checklist pass.
- Real contact/channel data was exercised informally on WioTrackerL1 but scroll-into-view once a
  contact/channel list exceeds the visible-row count, and whether 350 contacts / 40 channels
  (the pilot envs' `MAX_CONTACTS`/`MAX_GROUP_CHANNELS`) causes any noticeable per-frame lag from
  the on-demand `getContactByIdx`/`getChannel` reads, haven't been specifically confirmed.
- The `_nav.depth() > 1` triple-click-becomes-KEY_HOME behavior was exercised informally on
  WioTrackerL1 (works) but hasn't been button-mashed on the other two pilot boards yet, and
  nobody's specifically double-checked that mute is still reachable in practice / that
  muscle-memory triple-clicks while drilled into Contacts don't surprise anyone.

## Phase 3 — Settings: DONE (device-verified on WioTrackerL1)

Spec: `phases/phase-3-settings.md`. Code was hand-traced against the real headers
(`NodePrefs.h`, `MyMesh.h`/`.cpp`, `RadioLibWrappers.h`, `DisplayDriver.h`, `UIScreen.h`) instead
of a real compile -- same limitation as every prior phase. One real bug was found and fixed
during that trace (see "Real bug found during tracing" below); an independent second-pass review
via a subagent was attempted but the org's monthly spend limit was hit partway through, so that
pass never completed -- only the author's own hand-trace and the user's subsequent device test
have actually checked this code.

### Real-device findings (from flashing WioTrackerL1)

User flashed `WioTrackerL1_companion_radio_forest_ble` and confirmed: radio param live-apply
(changed freq/bw/sf/cr/TX power and confirmed the new values took effect immediately, not just
after reboot -- the single highest-risk item this phase, per phase-3-settings.md's own framing),
settings survive a reboot, and general menu navigation ("some of the older features" -- an
informal regression pass, not a full checklist run) all work with no new issues. The known
status-bar-overlap bug carried forward from Phase 2 is present and unchanged, nothing new. This
was **not** a formal checklist pass (`phases/phase-3-settings.md`'s "Done-when" block hasn't been
explicitly run/recorded) and did **not** specifically exercise:
- Danger Zone (`Screen_SettingsDanger`'s Erase/New Identity/Reboot) -- none of the three were
  triggered.
- `TextField` (the Advert "Name" editor) -- not confirmed either way; WioTrackerL1's joystick
  input scheme means it wouldn't exercise the single-button-specific `KEY_PREV`-advances-cursor
  concern (see ARCHITECTURE.md's `FormField` section) even if it was tried, since joystick boards
  have `KEY_CANCEL` from a dedicated back button that single-button boards lack.
- `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` (2 of PLAN.md's 3 named pilots plus the rotary
  pilot) -- still build-verified only, same gap carried since Phase 0.

Treat radio-param live-apply and reboot-survival as confirmed; treat everything above as still
open.

### Built

All new, flat in `examples/companion_radio/ui-forest/` per PLAN.md §4:

- **`FormField.h/.cpp`** -- four reusable field-editor `UIScreen` subclasses (`ToggleField`,
  `StepperField`, `EnumField`, `TextField`) plus a `namespace FormField` of spec structs
  (`ToggleFieldSpec`/`StepperFieldSpec`/`EnumFieldSpec`/`TextFieldSpec`) and opener functions
  (`openToggleField`/`openStepperField`/`openEnumField`/`openTextField`). See "Decisions" below
  for why specs+openers exist instead of each settings screen writing its own per-field glue.
  Each editor is a **single shared instance** (one of each, owned by `UITask`, matching
  `ConfirmScreen`'s already-established "shared, `begin()` reconfigures it" pattern) -- not one
  instance per settings field. Commit is staged (Stepper/Enum/Text only write+savePrefs on
  `KEY_ENTER`, discard on `KEY_CANCEL`) to avoid a flash write (and, for radio params, a live
  re-tune) on every single adjustment keypress; Toggle commits immediately per phase-3-settings.md's
  literal wording ("ENTER flips it immediately").
- **`Screen_Settings.h/.cpp`** -- root menu, 5 `Submenu` rows (Radio/Advert/Network/Device/Danger
  Zone). Purely declarative; no callbacks of its own.
- **`Screen_SettingsRadio.h/.cpp`** -- Frequency/Bandwidth/Spreading Factor/Coding Rate/TX Power +
  Restore Defaults. **Every field's setter calls `the_mesh.savePrefs()` AND
  `radio_driver.setParams()`/`setTxPower()`** -- the exact live-apply pairing PLAN.md flags as the
  easiest bug to introduce this phase. Bandwidth is an `Enum` over the 10 standard LoRa BW steps
  (7.8-500 kHz), not a free-form `Stepper` -- the wire protocol's own validation
  (`CMD_SET_RADIO_PARAMS`) only range-checks it, but the radio hardware only supports discrete
  values, so a stepper could produce a value the chip silently can't honor.
- **`Screen_SettingsAdvert.h/.cpp`** -- Name (`Text`) + Share Location (`Toggle`) + Restore
  Defaults. Name validation mirrors `CMD_SET_ADVERT_NAME`'s handler exactly: length-truncation to
  31 chars only, no character allowlist (confirmed by reading the full handler body, not guessed).
- **`Screen_SettingsNetwork.h/.cpp`** -- Repeat, RX Boost, Telemetry (Base/Location/Environment,
  each a 3-way `Enum`), Auto-add Contacts, Auto-add Max Hops, Duty Cycle Factor, RX Delay Factor +
  Restore Defaults. RX Boost's setter also calls `radio_driver.setRxBoostedGainMode()` -- a
  **second** write-prefs-but-forgot-to-live-apply trap beyond the one PLAN.md explicitly flagged,
  found by noticing `MyMesh::begin()` calls it right after loading prefs and asking why a settings
  screen wouldn't need to do the same. Repeat's setter re-checks
  `the_mesh.isValidClientRepeatFreq()` (now public, see below) before allowing it on; Radio's
  Frequency setter also re-checks this and auto-disables Repeat if the new frequency can't
  support it (see "Decisions" below).
- **`Screen_SettingsDevice.h/.cpp`** -- Buzzer (`Toggle`, wired straight to the existing
  `UITask::toggleBuzzer()`) + Notifications (stub, toasts "Phase 7" -- phase-3-settings.md
  explicitly says leave the slot, don't build the content) + Restore Defaults. "Vibration" is
  **not implemented** -- see "Decisions" below, this is a real scope gap, not an oversight.
- **`Screen_SettingsDanger.h/.cpp`** -- Erase All Data / New Identity / Reboot, every one
  `ConfirmScreen`-gated (reuses `UITask`'s single shared `_confirm` instance, per
  ARCHITECTURE.md's own prediction that this would be Phase 3's second caller). Erase and Rekey
  call the two new `MyMesh` methods below; Reboot calls the existing (previously dead-code)
  `UITask::shutdown(true)` restart path.
- **`MenuScreen.cpp`** -- `activate()`'s `Toggle`/`Stepper`/`Enum`/`Text` cases, previously
  documented no-ops, now call the matching `FormField::openXxxField()`.
- **`MyMesh.h`/`.cpp`** (not `ui-forest`, but required by it) -- two new public methods:
  - `bool factoryReset()` -- on-device equivalent of `CMD_FACTORY_RESET`'s handler body (disable
    serial, format filesystem, reboot).
  - `bool selfRekey()` -- on-device equivalent of `CMD_IMPORT_PRIVATE_KEY`'s identity-regen side
    effect, generating a fresh identity (same reserved-hash retry loop as `begin()`) instead of
    importing a supplied one, then `resetContacts()` + `_store->loadContacts(this)` to invalidate
    ECDH secrets computed against the old identity.
  - `isValidClientRepeatFreq()` moved from `private` to `public` (no implementation change) so
    `Screen_SettingsNetwork`/`Screen_SettingsRadio` can call it for the repeat-frequency gate.

  All three mirror the exact shape PLAN.md already pre-approved for `getQueueLen()` in Phase 4:
  `_store`/`self_id`/`resetContacts()` are private to `MyMesh`, so `UITask` can't reach them
  without a passthrough -- this isn't new API surface in spirit, just the same pattern applied
  to two more cases discovered while implementing the Danger Zone screen.
- **`UITask.h`/`.cpp`** -- 4 new members (the shared `ToggleField`/`StepperField`/`EnumField`/
  `TextField` instances) + 6 new screen pointers, all constructed in `begin()` following the
  existing "leaf screens before anything that references them" ordering. Home gained a
  "Settings" entry (between the optional GPS/Sensors slots and Shutdown);
  `UI_FOREST_HOME_ITEM_COUNT` bumped 10 -> 11.

### Real bug found during tracing

`TextField::begin()` originally called `get(ctx, _buf, sizeof(_buf))` -- passing the FULL buffer
size (32) as `max_len` to the `TextGetFn` callback. `Screen_SettingsAdvert::getName()` (the one
implementation of that callback this phase) does `dest[max_len] = 0;`, which with `max_len=32`
writes to `dest[32]` -- one byte past the end of a 32-byte array, corrupting whatever member
follows `_buf` in memory (`_len`, an `int`, was declared immediately after it). Every time the
Advert screen's Name field was opened, this would have silently clobbered part of `TextField`'s
own length tracking. Fixed by passing `_max_len` (always `<= 31`) instead of `sizeof(_buf)`. Caught
by hand-tracing the buffer-index math end to end, not by a compiler/sanitizer -- exactly the kind
of bug this dev environment's lack of a compiler makes easy to miss, so worth calling out as a
concrete example of why "trace carefully" matters here, not just a formality.

### Decisions / deviations worth knowing about

- **`MenuItem`'s `Toggle`/`Stepper`/`Enum`/`Text` kinds resolve through a spec-struct + shared
  opener function, not by each settings screen writing its own per-field "push the editor"
  glue.** ARCHITECTURE.md's Phase 2 text predicted almost exactly this shape ("Phase 3 will give
  them real behavior by pushing a FormField-based editor screen instead of extending this switch
  statement's shape"). Concretely: a `MenuItem` of kind `Toggle` sets `action_ctx` to point at a
  `FormField::ToggleFieldSpec` (built by the owning settings screen, bundling which shared
  `ToggleField` instance + `NavStack` to push into, plus the field's own get/set callbacks) instead
  of a `UIScreen*` the way `Submenu` does; `MenuScreen::activate()` calls
  `FormField::openToggleField(item.action_ctx)`, which is the ONE place that knows how to
  `begin()` + `push()` a `ToggleField`. This means adding a new Toggle-kind row anywhere in the
  codebase never touches `MenuScreen.cpp` again -- confirmed useful immediately, since Radio/
  Advert/Network/Device between them add 6 `Toggle` rows, 6 `Enum` rows, and 8 `Stepper` rows
  through this one shared mechanism with zero duplicated "open the editor" boilerplate.
- **Get/set callbacks are plain C function pointers (`void*` ctx), not `std::function` or
  capturing lambdas.** Matches the rest of this codebase's callback idiom exactly (`MenuActionFn`,
  `ConfirmActionFn`, `Screen_Shutdown.cpp`'s `shutdownAction(void* ctx)`). Every settings screen
  passes `this` as `ctx` and casts back inside a `static` member function -- same trick already
  used for every `Action`-kind row since Phase 0.
- **`advert_interval`/`flood_advert_interval` do not exist for `companion_radio` at all.** PLAN.md
  §8 and phase-3-settings.md both flag "does changing these reschedule automatically, or does
  companion_radio need an explicit call" as an open question to resolve. The actual answer:
  the question's premise doesn't apply -- these are fields on `CommonCLI`'s own `NodePrefs`-shaped
  struct (`src/helpers/CommonCLI.h`), used only by `simple_repeater`/`simple_room_server`/
  `simple_sensor`. `companion_radio`'s own `NodePrefs.h` (a completely separate struct) has no such
  fields, and `companion_radio` doesn't use `CommonCLI` at all (confirmed by grep, not assumed).
  `Screen_SettingsAdvert` therefore has nothing to build for this -- resolved by discovering the
  premise was false, not by picking an answer.
- **Advert name and Device settings both wanted "name" per phase-3-settings.md's wording, but
  `NodePrefs` has exactly one name field (`node_name`).** Implemented once, under
  `Screen_SettingsAdvert` (matches `CMD_SET_ADVERT_NAME`'s own framing of it as the advertised
  name); `Screen_SettingsDevice` does not duplicate a second editor for the same field. See
  `Screen_SettingsAdvert.h`'s header comment.
- **"Vibration" (phase-3-settings.md step 6) is not implemented.** There is no persisted
  vibration-enable field in `NodePrefs` -- `UITask::notify()` triggers the vibration motor
  unconditionally whenever `PIN_VIBRATION` is defined, with no toggle at all today. Adding one
  means appending a field to `NodePrefs` *and* to `DataStore.cpp`'s `loadPrefsInt()`/`savePrefs()`,
  which is a hand-maintained, unversioned, fixed-byte-offset binary format with no length guard
  (confirmed by reading it) -- real structural surgery on existing users' saved prefs files, not a
  UI-only change, and not something to attempt unverified with no compiler on PATH. A toggle that
  can't persist would also fail the phase's own "survives a reboot" checklist item, so building a
  fake one would have been worse than flagging the gap. Left for a future phase alongside an
  actual prefs-format version bump if the maintainers want it.
- **Per-contact-type auto-add allow bits (`AUTO_ADD_CHAT`/`REPEATER`/`ROOM_SERVER`/`SENSOR`) and
  "overwrite oldest when full" are not exposed in `Screen_SettingsNetwork`.** Those bit constants
  are `#define`d locally inside `MyMesh.cpp` (not a header), so surfacing them would mean either
  duplicating private implementation constants in `ui-forest` or adding new `MyMesh` API that
  neither PLAN.md's data table nor phase-3-settings.md's field list calls for. "Auto-add Contacts"
  (on/off) + "Auto-add Max Hops" cover the primary policy `isAutoAddEnabled()`/`getAutoAddMaxHops()`
  already gate on; the finer-grained per-type bits are a deliberate scope cut, not a miss.
- **Radio-frequency changes auto-disable "Repeat" if the new frequency can't support it, instead
  of blocking the frequency change.** `isValidClientRepeatFreq()` only permits a handful of exact
  frequencies (region-specific repeater channels). The real `CMD_SET_RADIO_PARAMS` handler
  validates `repeat` and `freq` together as one atomic wire command and rejects the whole thing if
  inconsistent; here they're edited on two separate screens at two separate times, so there's no
  single atomic moment to validate against. Chose "changing frequency silently turns Repeat back
  off + toasts why" over "block the frequency edit" because blocking a user's primary reason for
  visiting the Frequency field (changing the frequency) to protect a secondary, already-disabled-
  by-default toggle seemed like the wrong thing to surprise someone with.
- **Restore-defaults writes known default constants directly, rather than calling into a
  `MyMesh` "reset to defaults" method.** No such method exists on `MyMesh` today (only
  `begin()`'s constructor path sets defaults, for first-boot use). Radio/Network's defaults reuse
  the exact macros (`LORA_FREQ`, `SX126X_RX_BOOSTED_GAIN`, etc.) `MyMesh`'s own constructor uses,
  so they can't drift out of sync silently. Advert's restore-defaults deliberately does **not**
  reset `node_name` -- there's no reachable "default name" without new `MyMesh` API (the boot
  default is derived from `self_id.pub_key`, private to `MyMesh`), and resetting a user's chosen
  name as a side effect of "restore defaults" would be surprising regardless of reachability. The
  on-screen confirm message says so explicitly ("Name is not reset") rather than leaving it
  silent.
- **`ConfirmScreen`'s single shared instance now has its second and third real callers**
  (`Screen_SettingsDanger`'s 3 actions, plus every settings screen's Restore Defaults action --
  all funnel through the same `UITask::_confirm` `Screen_Shutdown` already established in Phase
  1). Confirms the "single shared instance" design holds up under multiple simultaneous callers,
  same as `ConfirmScreen.h`'s own comment predicted it would need to.
- **`UITask::shutdown(true)` (the restart path) gets its first real caller** via
  `Screen_SettingsDanger`'s Reboot action -- previously dead code (`Screen_Shutdown` only ever
  calls the default `shutdown()` / power-off path).

### Not yet done / needs hardware verification

- **Radio param live-apply and reboot-survival are now confirmed** on WioTrackerL1 (see
  "Real-device findings" above) -- the two highest-priority items phase-3-settings.md called out
  are done. Not yet confirmed via a *second* radio/companion app independently observing the new
  params (the user's test confirmed the device applied the change immediately, not specifically
  that a peer saw it) -- low-priority follow-up, not a blocker.
- The repeat-auto-disable-on-frequency-change behavior (see "Decisions" above) has never been
  exercised -- worth deliberately setting Repeat on at a valid frequency, then changing frequency
  away from it, to confirm the toast fires and Repeat actually reads back off.
- `TextField`'s gesture mapping (`KEY_NEXT`=increment char, `KEY_PREV`=advance cursor -- see
  `FormField.h`'s class comment for why PREV doesn't decrement) has never been tried by a human
  finger, and WioTrackerL1's joystick input scheme can't exercise the single-button-specific
  concern even once it is tried (its dedicated back button gives it `KEY_CANCEL`, which
  single-button boards lack). Still needs a real test on `RAK_4631`, where it's the only editing
  path available at all.
- `Screen_SettingsDanger`'s Erase/Rekey/Reboot have never been triggered on real hardware. Rekey
  in particular is worth a deliberate test with a real second node already paired as a contact,
  to confirm the "contacts survive, re-sync automatically" claim (`MyMesh::selfRekey()`'s
  `resetContacts()` + `_store->loadContacts(this)` round-trip) actually holds and the other node
  can still talk to this one afterward (it can't, until this node re-adverts its new identity --
  worth confirming that's an acceptable/expected UX, not a surprise).
- Every "Restore Defaults" action, per section, still needs its own reboot-survival check --
  only ordinary field edits were confirmed to survive a reboot, not the restore-defaults path
  specifically.
- `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` (2 of PLAN.md's 3 named pilots plus the rotary
  pilot) are still build-verified only, same gap carried since Phase 0 -- only WioTrackerL1 has
  been physically tested for Phase 3, same pattern as every prior phase.
- Flash/RAM headroom on `RAK_4631` (tightest pilot board) has not been checked -- this phase adds
  6 new screens + 4 shared editor instances + 2 new `MyMesh` methods on top of Phase 0-2's
  already-unverified footprint. PLAN.md §3.6 says trim here first if it doesn't fit. WioTrackerL1
  booting and running fine is a data point but not proof for `RAK_4631` specifically.
- The independent second-pass code review (via a subagent) that was meant to catch anything the
  author's own trace missed did not complete -- the org's monthly spend limit was hit mid-review.
  Only the author's hand-trace plus this informal device pass have checked this code; no formal
  checklist run (`phases/phase-3-settings.md`'s "Done-when" block) yet.

## Phase 4 — Diagnostics ("nerd stats"): DEVICE-TESTED ON WioTrackerL1, ONE BUG FOUND AND FIXED

Spec: `phases/phase-4-diagnostics.md`. Code was hand-traced against the real headers
(`Dispatcher.h`, `RadioLibWrappers.h`, `MyMesh.h`/`.cpp`, `main.cpp`, `StatsFormatHelper.h`,
`UIScreen.h`, `DisplayDriver.h`) instead of a real compile -- same limitation as every prior
phase (no `pio`/`g++` on PATH in this dev environment). The user then flashed
`WioTrackerL1_companion_radio_forest_ble` and reported three things (see "Real-device findings"
below): the pre-existing status-bar/menu overlap bug is still present (unchanged, deferred to
Phase 5 as before), a real status-bar-flash bug was found and fixed by hand-tracing every
`DisplayDriver` backend's `startFrame()`, and a message-preview auto-dismiss behavior was observed
that most likely traces to existing (pre-Phase-4) companion-app-connected behavior, pending user
confirmation. Diagnostics content itself wasn't flagged as wrong ("everything else looked good").

### Open questions from PLAN.md §8 / phase-4-diagnostics.md -- resolved by reading the real headers

- **Direct-path packet counters: they exist.** `Dispatcher::getNumSentDirect()`/
  `getNumRecvDirect()` (`src/Dispatcher.h:187,189`) sit right next to the already-confirmed
  `getNumSentFlood()`/`getNumRecvFlood()`, public, inherited by `MyMesh` unchanged. `Screen_DiagPackets`
  shows all four (plus `radio_driver`'s recv/sent/recv_errors) -- 7 fields total, no flood-only
  scope cut needed.
- **Transport-type story: compile-time constant, confirmed by reading `main.cpp`.**
  `examples/companion_radio/main.cpp:37-89` picks exactly one `BaseSerialInterface` subclass per
  board via `#if defined(WIFI_SSID)` / `#elif defined(BLE_PIN_CODE)` / `#elif defined(SERIAL_RX)` /
  `#else` (ESP32), and an analogous chain for RP2040/NRF52/STM32 (NRF52 additionally has
  `ETHERNET_ENABLED`). There is no runtime signal for "which transport is this" --
  `isSerialEnabled()`/`hasConnection()` report whether the *chosen* transport is enabled/connected,
  not which one it is. Resolved with a new `Transport.h` (see "Built" below): the same macro
  precedence, once, as a compile-time string constant (`UI_FOREST_TRANSPORT_NAME`) -- not queried
  per frame.
- **A second discovery made while tracing, not one of the two named open questions:** the
  exact field sets to show per diagnostics screen were cross-checked against
  `MyMesh::handleCmdFrame`'s `CMD_GET_STATS`/`RESP_CODE_STATS` handler (`MyMesh.cpp`, `STATS_TYPE_CORE`/
  `_RADIO`/`_PACKETS`) and `src/helpers/StatsFormatHelper.h`, since that's the exact binary protocol
  the companion app's stats view parses -- the same "second source of truth" the phase's own
  Done-when checklist requires matching. `Screen_DiagPackets`'s 7 fields are an exact match to
  `STATS_TYPE_PACKETS`'s 7-field reply. `Screen_DiagRadio` intentionally stays at the 3 fields
  phase-4-diagnostics.md step 3 names (noise floor/RSSI/SNR) rather than also adding
  `STATS_TYPE_RADIO`'s tx/rx air-time fields, to avoid scope creep beyond what was asked -- the
  companion app's radio stats view will show a superset, not a mismatch. `Screen_DiagCore`'s uptime
  uses `millis()/1000` specifically because that's what `STATS_TYPE_CORE`'s reply uses
  (`_ms->getMillis() / 1000`, not `rtc_clock`) -- confirms PLAN.md's "or simply millis()/1000"
  option is the one that actually matches the phone app, not a guess.

### Prerequisite gap (see ARCHITECTURE.md) -- resolved by building it in this phase, not before it

phase-4-diagnostics.md's own "Prerequisites" section claimed Phase 3 already added a Diagnostics
`Submenu` placeholder to Home's menu. It didn't (confirmed: `phase-3-settings.md`'s "What to build"
list never asked for one, and `PROGRESS.md`'s Phase 3 section only lists a "Settings" entry).
Rather than treating this as a blocker, this phase adds the Home entry and its real content in one
step -- `UI_FOREST_HOME_ITEM_COUNT` bumped 11 -> 12, "Diagnostics" inserted right before "Settings"
in Home's item order (after the optional GPS/Sensors slots).

### Built

All new, flat in `examples/companion_radio/ui-forest/` per PLAN.md §4:

- **`Screen_Diagnostics.h/.cpp`** -- root menu, thin `MenuScreen` subclass, 4 `Submenu` rows
  (Radio/Packets/Core/Event Log). Same shape as `Screen_Settings` -- no callbacks of its own.
- **`Screen_DiagRadio.h/.cpp`** -- Noise Floor / RSSI / SNR via `radio_driver.getNoiseFloor()`/
  `getLastRSSI()`/`getLastSNR()`, zero new API (already used by `ui-new` and `Screen_RadioInfo` today).
- **`Screen_DiagPackets.h/.cpp`** -- Received / Sent / Flood TX / Direct TX / Flood RX / Direct RX /
  RX Errors -- 7 fields, `radio_driver.getPacketsRecv()/getPacketsSent()/getPacketsRecvErrors()` +
  `the_mesh.getNumSentFlood()/getNumSentDirect()/getNumRecvFlood()/getNumRecvDirect()`, all
  already-public, zero new API.
- **`Screen_DiagCore.h/.cpp`** -- Queue (via the new `MyMesh::getQueueLen()`), Uptime
  (`millis()/1000`, formatted `Xd HH:MM:SS`/`H:MM:SS`/`M:SS`), Transport (`UI_FOREST_TRANSPORT_NAME`
  from the new `Transport.h`).
- **`Screen_EventLog.h/.cpp`** -- ring-buffer viewer, newest-entry-first, Layout-scrolled. Reads
  the shared `EventLog` instance by reference; doesn't own it.
- **`EventLog.h`** -- new standalone header (not called for by PLAN.md §4's file list verbatim, but
  explicitly sanctioned by its own "add files as needed" caveat): a small header-only, fixed-array
  (20-entry) ring buffer class, kept separate from `Screen_EventLog` specifically so Phase 7's
  `Screen_RecentEvents` can reuse the same buffer class for its own instance (PLAN.md §7's note on
  the two screens). `UITask` owns the one shared instance (`_event_log`) and exposes
  `logEvent(const char* text)` as the single place any screen appends to it.
- **`Transport.h`** -- new standalone header: the compile-time transport-name macro
  (`UI_FOREST_TRANSPORT_NAME`), resolving the transport-type open question above. Kept as its own
  tiny header (rather than folded into `UITask.h`) so both `UITask.cpp` (status bar text) and
  `Screen_DiagCore.cpp` can include just this, without one pulling in the other's larger header.
- **`MyMesh.h`** (not `ui-forest`, but required by it) -- one new public passthrough:
  `uint32_t getQueueLen() const { return _mgr->getOutboundTotal(); }`. `_mgr` is `protected` on
  `Dispatcher` (not private) -- confirmed accessible from `MyMesh`'s own member functions via the
  public inheritance chain `MyMesh -> BaseChatMesh -> mesh::Mesh -> Dispatcher` (all `public`,
  confirmed by grep, not assumed). Same passthrough shape as Phase 3's `factoryReset()`/`selfRekey()`.
- **`StatusBar` / `UITask::updateStatusBar()`** -- the status bar's transport field used to be a
  hardcoded `"BLE:%s"` label (harmless so far since every existing forest env is BLE, but wrong for
  any future WiFi/USB/Ethernet forest env). Replaced with `UI_FOREST_TRANSPORT_NAME ":%s"` so the
  label always matches what this build was actually compiled for (PLAN.md item 25's "fold into
  StatusBar" half, alongside the `Screen_DiagCore` row).
- **Event log feed points** -- per phase-4-diagnostics.md step 7's explicit constraint ("hook into
  existing code paths ... don't add new callback plumbing on Mesh/MyMesh"), `logEvent()` is called
  from four places that already observe something happening, not from any new `MyMesh` callback:
  - `UITask::notify()` -- already the one call site `MyMesh` routes `contactMessage`/
    `channelMessage`/`roomMessage`/`newContactMessage` through (`MyMesh.cpp`); each now also logs a
    short line ("Message received", "Channel message received", etc). `ack` deliberately does
    **not** log -- see "Decisions" below.
  - `Screen_Advert`'s `KEY_ENTER` handler -- "Advert sent"/"Advert failed", right alongside the
    existing toast.
  - `Screen_SettingsRadio::applyRadioParams()` (the shared freq/bw/sf/cr helper) and `setTxPower()`
    -- "Radio params changed" each time either commits.
  - `UITask::loop()`'s `AUTO_SHUTDOWN_MILLIVOLTS` low-battery branch -- "Low battery: shutting
    down", right before the existing shutdown sequence.

### Decisions / deviations worth knowing about

- **Diagnostics screens use `Screen_ContactDetail`'s Layout-scrolled label/value list shape, not
  `MenuScreen`'s `Info` row kind.** `MenuItemKind::Info` (added in Phase 0, still unused before this
  phase) only ever renders `item.label` -- a static `const char*` -- so showing a live, per-frame
  changing value through it would mean mutating a label buffer imperatively before every render,
  which is more awkward than the label/value-columns pattern `Screen_ContactDetail` already
  established in Phase 2 for exactly this "more read-only fields than fit on screen" problem. All
  four new Diagnostics screens (`Radio`/`Packets`/`Core`/`EventLog`) use that shape instead. `Info`
  remains an unused placeholder in `MenuScreen` after this phase too -- not a regression, just never
  turned out to be the right tool for a live-value row.
- **`ack` events are not logged to the event log**, even though `UITask::notify()` is the feed
  point for the others. `notify(UIEventType::ack)` fires for actual message acks *and* as a generic
  "confirmation tone" for GPS/buzzer toggles (see `toggleGPS()`/`toggleBuzzer()` in `UITask.cpp`,
  both already call `notify(UIEventType::ack)` before Phase 4 existed) -- logging it indiscriminately
  would fill the 20-entry ring buffer with toggle-confirmation noise, crowding out actually
  mesh-diagnostic events. Left out rather than threading a new event type through just for this.
- **"Contact discovered" and "advert received" (two of phase-4-diagnostics.md step 7's four example
  events) are not logged -- there is no existing hook to observe them from.** Checked
  `MyMesh::onDiscoveredContact` (`MyMesh.cpp`) directly: it only ever writes to `_serial` (the
  companion-app push-frame path), never calls into `_ui`/`AbstractUITask` at all. Adding one would
  be exactly the "new callback plumbing on Mesh/MyMesh" step 7 explicitly says not to add for this
  phase. `newContactMessage` (logged as "New contact message") is the closest already-wired proxy --
  it fires when a message arrives from a not-yet-known contact (`MyMesh.cpp:364`), which is related
  but not the same event as an advert/discovery. Advert *sent* and low battery, the other two named
  examples, are both logged (see "Built" above).
- **`Screen_DiagRadio` stays at 3 fields (noise floor/RSSI/SNR) rather than also showing tx/rx
  air-time**, even though `STATS_TYPE_RADIO`'s companion-app reply includes both. phase-4-diagnostics.md
  step 3 names exactly noise floor/RSSI/SNR; adding fields beyond what was asked wasn't judged worth
  the scope creep for this phase. `Dispatcher::getTotalAirTime()`/`getReceiveAirTime()` are already
  public if a future phase wants them.
- **`EventLog` and `Transport.h` are new standalone headers not in PLAN.md §4's literal file list.**
  Both fall under that section's own "add files as needed ... this is the target shape, not a
  checklist" caveat -- `EventLog` because PLAN.md §7 explicitly asks for the ring-buffer class to be
  reusable by Phase 7, and a class two screens will eventually share doesn't belong inside either
  screen's own file; `Transport.h` because the transport-name macro is needed by both `UITask.cpp`
  and `Screen_DiagCore.cpp`, and neither is a natural home for logic the other one also depends on.

### Real-device findings (from flashing WioTrackerL1) + one bug found and fixed

User flashed `WioTrackerL1_companion_radio_forest_ble` and reported three things. All Diagnostics
content itself ("everything else looked good") was not called out as wrong, so treat the new
screens' values as informally confirmed pending the formal companion-app side-by-side comparison
that's still outstanding (see below).

1. **Status bar text still overlaps with menu content.** This is the same bug carried forward from
   Phase 2 (see ARCHITECTURE.md's "Known gaps"), not something Phase 4 introduced or changed --
   every new Diagnostics screen was written to match the existing `Layout::statusBarHeight()`
   offset convention exactly, same as every other screen. Still deferred to Phase 5 per the user's
   standing call on this one.
2. **A real bug, found and fixed: the status bar could flash/blank for one frame.** Root cause,
   confirmed by reading all ten `DisplayDriver` backend `.cpp` files (`SH1106Display.cpp`,
   `SSD1306Display.cpp`, `LGFXDisplay.cpp`, `GxEPDDisplay.cpp`, `E290Display.cpp`, `E213Display.cpp`,
   `NV3001BDisplay.cpp`, `ST7735Display.cpp`, `ST7789Display.cpp`, `ST7789LCDDisplay.cpp`): every
   single one's `startFrame()` does a full clear/fill of the screen buffer -- there is no partial
   redraw backend anywhere in this framework. `UITask::loop()` (`UITask.cpp`) didn't account for
   this: it only called `_status_bar.render()` when `status_due` was true, but entered the whole
   redraw block whenever `content_due || status_due || _toast.isShowing()`. Any time `content_due`
   (a screen's own ~1000ms refresh timer) or `_toast.isShowing()` fired *without* `status_due` also
   being true, `startFrame()` wiped the status bar's pixels and `_status_bar.render()` was never
   called to put them back -- a real blank-status-bar frame, exactly matching "sometimes flashes."
   Given the status bar text on a 128px-wide display is ~240px at text size 1 (computed from the
   actual `"%s | BUZ:%s | GPS:%s | BLE:%s - "` format with a typical node name, not guessed), it's
   almost always in marquee-scroll mode, so `status_due` fires on its own independent ~80ms cadence
   -- meaning this blank frame was probably happening roughly once per second, whenever a screen's
   own content-refresh timer happened to land between two scroll ticks. **Fixed** in `UITask.cpp`:
   the status bar is now redrawn on every pass through the compositing block (gated on
   `!showing_splash` only), not gated on `status_due` -- `status_due` still decides whether the
   block runs *at all*, it just no longer decides whether the status bar draws once something else
   already triggered a redraw.
3. **The message-preview screen flashes back to Home almost immediately after a message arrives --
   confirmed cause, not a bug, deliberately not fixed this phase (user's call: note it, fix later).**
   `Screen_MsgPreview`/`UITask::newMsg()` weren't touched by Phase 4 at all, so this isn't a
   regression from this phase's changes. Root cause, confirmed with the user (phone app was
   connected via BLE during the test): `UITask::msgRead()` (`if (msgcount == 0) gotoHomeScreen();`)
   is called from exactly one place, `CMD_SYNC_NEXT_MESSAGE`'s handler (`MyMesh.cpp:1397-1403`) --
   i.e. whenever the *companion app* actively pulls the new message over BLE. With the app
   connected, it synced the single new message within about a second of it arriving, calling
   `msgRead(0)` right after `newMsg()` had pushed `Screen_MsgPreview`, snapping back to Home almost
   immediately -- this is `ui-new`'s original, unchanged interrupt behavior (ported as-is in Phase
   1), not a bug in the message-preview screen itself, and not something Phase 4 introduced.
   **Backlog item for a future phase** (raised here, not fixed): on-device UX arguably shouldn't
   yank the display back to Home the instant a phone app happens to sync a message -- a
   phone-connected user gets essentially no chance to read the on-device preview at all. Possible
   directions for whichever phase picks this up: give the on-screen preview a minimum dwell time
   regardless of `msgRead(0)` arriving, or have `msgRead()` only pop back to Home if
   `Screen_MsgPreview` wasn't shown recently/deliberately dismissed by the user already. Not
   scoped or designed further here -- this note exists so the behavior isn't rediscovered as a
   mystery next time.

### Not yet done / needs hardware verification

- **`WioTrackerL1` has been flashed and informally tested** (see "Real-device findings" above) --
  the Diagnostics menu, its four sub-screens, and general navigation were reported as looking good;
  one real bug (status bar flash) was found and fixed as a result, and the status-bar/menu overlap
  bug (carried forward from Phase 2) was reconfirmed present. This was **not** a formal checklist
  pass (`phases/phase-4-diagnostics.md`'s "Done-when" block hasn't been explicitly run/recorded),
  and did **not** specifically confirm:
  - **Whether the Diagnostics values actually match the companion app's stats view side-by-side**,
    per the checklist's own explicit requirement -- this was traced against the same wire-protocol
    code the phone app's view is built from (`STATS_TYPE_CORE`/`_RADIO`/`_PACKETS`), but that's a
    paper argument, not a device comparison. `Screen_DiagCore`'s Queue field in particular is worth
    checking against actual mesh traffic (send a message while offline/queued and confirm the value
    visibly differs from 0).
  - Event log entries specifically -- whether they appear newest-first, drop the oldest past 20
    entries, and keep the "Ns ago" age counting up correctly across repeated renders. Not called
    out as wrong, but not specifically exercised either as far as reported.
  - The now-fixed status bar flash (finding 2 above) -- fixed by hand-trace against all ten
    `DisplayDriver` backends' `startFrame()` implementations, not yet re-confirmed fixed on hardware.
- The status bar's transport label change (`"BLE:%s"` -> `UI_FOREST_TRANSPORT_NAME ":%s"`) is a
  no-op in practice for every existing forest env (all four are `_ble`), so this hasn't actually
  changed anything visible yet -- only matters once/if a non-BLE forest env is added.
- `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` (2 of PLAN.md's 3 named pilots plus the rotary
  pilot) have no Phase 4 hardware pass at all yet, same gap carried since Phase 0.
- No independent second-pass code review (subagent or otherwise) has been attempted for this phase's
  code -- only the author's own hand-trace against the real headers, described above.

## Phase 5 — Visual polish: CODE COMPLETE, ENTIRELY BUILD-UNVERIFIED (no device pass yet)

Spec: `phases/phase-5-visual-polish.md`. Code was hand-traced against the real headers
(`Adafruit_GFX`/`Adafruit_SH110X`/`Adafruit_SSD1306`/`Adafruit_ST7789`/`GxEPD2_BW` for the
text-wrap fix, plus all ten `DisplayDriver` backend `.cpp` files for the same "confirm every
backend, don't assume" discipline the Phase 4 status-bar-flash fix used) -- same no-`pio`/no-`g++`
limitation as every prior phase. Nothing in this phase has been flashed yet; see ARCHITECTURE.md's
"Phase 5 additions"/"Known gaps" sections for the full technical account, this section is the
narrative/decisions log.

### Status heading into Phase 6

Phase 5 is code-complete but **not yet hardware-tested at all** -- unlike every prior phase,
there is no informal `WioTrackerL1` pass yet, let alone one for the other three pilots. Four
bugs were caught during the user's review of this phase's diff, before any flash, and fixed
same-session (battery gauge not clearing its own background, color choices not checking
`supportsColor()`, the screen-transition wipe reverted after reading as a flash/glitch, and
emoji in node names now stripped rather than substituted) -- see "Bugs found in user review"
below for the full account of each. Read that section before touching `StatusBar`/`Layout`/
color-related code again so none of the four get reintroduced.

Recommended hardware test order once a build is available: `WioTrackerL1` first, since it's
the only board with any track record on `ui-forest` (every prior phase's device pass ran on
it) -- confirms the baseline still holds before spending time on boards with zero history.
Then `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32`/`lilygo_techo`, in whatever order is
convenient -- none of the four has run any phase of `ui-forest` on real hardware yet, so each
is a first-time pass for everything built across Phases 0-5, not a regression check (see "Two
things resolved before starting" below for `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32`
specifically; `lilygo_techo` is new as of this phase and covered in "Not yet done" below).

### Two things resolved before starting, per the user's corrections to this phase's own spec

- **The Phase 4 status-bar-flash fix (compositing/`status_due`) was NOT re-verified on hardware
  this phase** -- the user flagged it as an easy regression check to do early on real hardware, but
  no device pass has happened yet at all (see "Not yet done" below). Still believed correct from the
  Phase 4 trace; just not re-confirmed.
- **`phases/phase-5-visual-polish.md`'s "Prerequisites" section undersells `RAK_4631`/
  `gat562_30s_mesh_kit`/`heltec_rc32` as needing only "a regression sweep".** Per the user: none of
  the three has ever been physically tested on any phase 0-4 -- only `WioTrackerL1` has. Phase 5's
  hardware pass on those three is their *first* test of everything built so far, not a regression
  check, budgeted accordingly (i.e. flagged prominently here rather than assumed low-risk).

### Real root cause found for the Phase 2 status-bar/menu overlap bug (fixed, not yet device-confirmed)

The u8g2-text-baseline theory floated through Phase 3 was already ruled out for `WioTrackerL1`
before this phase started (`SH1106Display` wraps `Adafruit_GFX`, whose `setCursor()` is top-left,
not baseline). Investigated fresh per the user's instruction not to reuse that theory. Actual cause:
`StatusBar`'s marquee scroll (`StatusBar.cpp`) prints its full (screen-width-exceeding) string
starting at a negative x so it can slide across the strip -- `Adafruit_GFX::write()`'s default
text-wrap (`wrap=true`, never disabled anywhere in this codebase) then wraps that print call onto
the next text row partway through the string once `cursor_x` would run past the right edge, painting
the tail of the status-bar text over whatever the current screen had drawn there. Confirmed by
reading `Adafruit_GFX`'s wrap logic directly and re-auditing all ten `DisplayDriver` backends (same
discipline as the Phase 4 fix): four wrap `Adafruit_GFX`/`GxEPD2` and default to wrap-on
(`SH1106Display`, `SSD1306Display`, `ST7789LCDDisplay`, `GxEPDDisplay`); `NV3001BDisplay`'s
hand-rolled `print()` never auto-wraps regardless, so `heltec_rc32` was never actually affected by
this bug despite also being a pilot. **Fixed** with one `display.setTextWrap(false)` call added to
each of those four backends' `startFrame()` (`src/helpers/ui/*.cpp` -- a framework-level fix, not
`ui-forest`-specific, since the bug lives in how every UI variant's status/marquee code interacts
with the shared `DisplayDriver` layer). `LGFXDisplay`/`ST7735Display`/`E213Display` intentionally
left untouched -- none is vendored in-repo in a form that let this be verified the same way
(LovyanGFX/TFT_eSPI/`heltec-eink-modules`, not `Adafruit_GFX`), and none of the currently-built
forest envs exercise them.

### Built

All in `examples/companion_radio/ui-forest/` unless noted, per PLAN.md §4:

- **Text-wrap fix** -- `src/helpers/ui/SH1106Display.cpp`/`SSD1306Display.cpp`/
  `ST7789LCDDisplay.cpp`/`GxEPDDisplay.cpp` (not `ui-forest`, see above).
- **`StatusBar` e-ink gating** -- `begin(int display_width, bool is_eink)` (was just `begin(int)`);
  skips the marquee and shows `drawTextEllipsized()`'s truncated form instead when `is_eink`;
  `needsRedraw()` no longer fires off the scroll-cadence timer in that case either. Wired from
  `UITask::begin()` via `_display->isEink()`.
- **`icons.h`** -- 9 new 16x16 icons (`settings_icon`, `warning_icon`, `event_log_icon`,
  `contact_icon`, `channel_icon`, `gps_fix_icon`, `gps_nofix_icon`, `signal_bars_0`..`_4` +
  `signal_bars[5]` lookup) and one new 32x32 (`torch_icon`), generated via a scratch Node script
  (not checked in) to avoid hand-transcription errors, then cross-checked row-by-row against the
  script's intent. Confirmed via repo-wide grep that no XBM conversion script/convention exists
  anywhere in this repo (PLAN.md 8's open question).
- **`Layout::iconRowHeight()`/`visibleIconRows()`** -- new helpers (`max(rowHeight(), 18)` and its
  matching visible-row count) so a 16x16 `MenuItem` icon doesn't bleed into the next row the way it
  would at the plain 11px `rowHeight()`. `MenuScreen` scans its own item table once at construction
  (`_has_icons`) and only switches to the taller row height (and aligns every row's text to the icon
  column) when at least one row actually has an icon.
- **`Layout::drawCard()`** -- left/right/bottom border around a screen's scrollable content block
  (the header separator already drawn by every such screen doubles as the top edge). Applied to
  `Screen_ContactDetail`/`Screen_DiagRadio`/`Screen_DiagPackets`/`Screen_DiagCore`/`Screen_EventLog`;
  every row in those five also moved its text in 2px from each edge so it doesn't sit on the new
  border lines.
- **Icons wired in**: `Screen_Contacts`/`Screen_Channels` (every row gets `contact_icon`/
  `channel_icon`); `Screen_SettingsDanger`'s Erase/New Identity rows get `warning_icon` (Reboot
  deliberately doesn't -- disruptive, not data-destructive). Deliberately **not** wired into Home's
  or `Screen_Diagnostics`'s `MenuItem` tables -- see "Decisions" below.
- **`MenuItem::tint`/`has_tint`** -- optional per-row unselected-color override, defaulting to
  `LIGHT`/`false` via the struct's own default member initializers (no existing call site needed
  touching). `Screen_SettingsDanger`'s two destructive rows set `tint = RED`.
- **`NavStack` screen-transition animation -- built, then reverted.** First cut was a 4-step
  solid-color wipe (`fillRect`+`startFrame`+`endFrame`) played from `push()`/`pop()`/`popToRoot()`,
  gated on `!display.isEink()`. Pulled after the user reviewed it and reported it read as a
  flash/glitch, not a transition -- see "Bugs found in user review" below. `NavStack` is back to
  its pre-Phase-5 shape.
- **Color convention sweep + `DisplayDriver::supportsColor()`** -- `Screen_Bluetooth`
  (green=enabled/red=disabled, was always green), `Screen_Gps` (green=on/red=off; green=fix/
  yellow=searching for the fix line specifically, not red -- no fix isn't an error),
  `Screen_DiagPackets` (RX Errors red only once nonzero, every other counter stays yellow=info),
  `FormField`'s `ToggleField` (green=on/red=off). All route through a new `Layout::accentColor()`
  helper, which downgrades to plain `LIGHT` on displays whose new `supportsColor()` override
  returns `false` -- see "Bugs found in user review" below for why this was added after the initial
  sweep, not as part of it originally.
- **Adaptive layout pass** -- no structural violations found (every screen already used
  `Layout::rowHeight()`/`visibleRows()`/`statusBarHeight()`/`headerHeight()` correctly); several
  bare pixel literals that happened to equal those helpers' output were replaced with the named
  calls for consistency (`Screen_RadioInfo`/`Screen_Recents`'s `y += 11`, `Screen_Gps`/
  `Screen_Sensors`'s `y += 12`, and the `display.height() - 11` bottom-hint-row pattern repeated
  across `Screen_Bluetooth`/`Screen_Advert`/`Screen_Shutdown`/all four `FormField` editors).
  `Screen_Splash` and the low-battery shutdown message were left as literals -- one-off centered
  full-screen content, not list rows.
- **New e-ink pilot env**: `LilyGo_T-Echo_companion_radio_forest_ble`
  (`variants/lilygo_techo/platformio.ini`), the same two-line-diff pattern (`-I` path, `ui-*/*.cpp`
  glob) as every other forest env, built from the existing `LilyGo_T-Echo_companion_radio_ble` env.

### Decisions / deviations worth knowing about

- **`heltec_e213` was considered and passed over for `lilygo_techo` as the e-ink pilot.** Both
  boards' envs exist and build `companion_radio` today (confirmed per the user), so either was a
  valid pick per PLAN.md. `lilygo_techo`'s `GxEPDDisplay` wraps `GxEPD2_BW`, vendored in-repo and
  confirmed to extend `Adafruit_GFX` -- letting the text-wrap fix (this phase's main finding) be
  verified against real, in-tree headers the same way every other backend was. `heltec_e213`'s
  `E213Display` wraps the external `heltec-eink-modules` library, not vendored anywhere in this
  repo, so the same verification wasn't possible without guessing at an unseen API. Picked the board
  where "trace carefully against the real headers" (the user's standing instruction for this
  no-compiler dev environment) could actually be followed all the way through.
- **Icons were deliberately NOT added to Home's or `Screen_Diagnostics`'s `MenuItem` tables**, even
  though both have entries (Contacts/Channels/Settings, Event Log) that logically map to a new icon.
  Discovered while wiring the very first icon in: `MenuScreen`'s 16x16 icon slot had never had a real
  caller before this phase, and nobody had noticed `Layout::rowHeight()` (11px) is shorter than a
  16x16 icon -- fixed generically via `iconRowHeight()`/`visibleIconRows()` (see "Built" above), but
  switching a *whole menu*'s row height taller still costs visible-row count for every row in that
  menu, not just the ones with icons. On a 64px OLED, Home (up to 12 items) would drop from ~3-4
  visible rows to ~2, and `Screen_Diagnostics` (4 items) similarly -- a real usability cost on the
  app's most-navigated screens, for icons on a minority of their rows. `Screen_Contacts`/
  `Screen_Channels` (every row has one, so the cost is uniform and intentional) and
  `Screen_SettingsDanger` (only 3 rows total, minor cost either way) don't have this problem, so
  those are where icons actually got wired in this phase. `event_log_icon`/`gps_fix_icon`/
  `gps_nofix_icon`/`settings_icon` all exist as assets regardless, available for whichever future
  phase wants to spend the density budget on them (or for a smarter per-row-height `MenuScreen` that
  doesn't force the whole list taller, which nobody has designed yet).
- **`signal_bars_0..4` exist but aren't wired into `StatusBar` (or anywhere) this phase.** PLAN.md
  3.5 names them as an icon to add, which is done; PLAN.md 3.2 additionally suggests using icons
  instead of text in the status bar on tall/wide displays, which was NOT attempted for signal
  strength specifically -- mapping RSSI to a 0-4 bar level needs a "no signal yet" baseline, and
  that sentinel value differs across the six radiolib wrapper backends
  (`src/helpers/radiolib/Custom*Wrapper.h`, `LR1110`/`LLCC68`/`SX1262`/`SX1276`/`SX1268`/`STM32WLx`)
  in ways not verified for all of them here. Wiring a threshold mapping in blind risked showing
  "full signal" on a device that had never received a packet -- a visible, embarrassing bug in the
  single most-visible strip in the whole UI. Left as an unwired asset rather than guessed at; a
  future phase with real hardware can check the boot-time RSSI reading before choosing thresholds.
- **Reboot doesn't get `warning_icon` in `Screen_SettingsDanger`, but Erase/New Identity do.** Same
  distinction the confirm-dialog messages already draw ("Cannot be undone" / "Contacts will re-sync"
  vs. no such warning for Reboot) -- reserving the icon for genuinely data-destructive actions keeps
  it a meaningful signal instead of decorating every row in the menu.

### Bugs found in user review, fixed same session

The user reviewed this phase's diff (still pre-flash) and reported four issues. All four are fixed;
none has been device-confirmed yet, same as the rest of Phase 5.

1. **`StatusBar`'s scrolling text visibly mixed into the battery icon.** Real bug, not a phantom --
   `renderBattery()` runs *after* the scrolling text specifically so the gauge always wins
   (`UITask.cpp`'s comment already said so), but the gauge is only an outline + partial fill +
   optional muted glyph, not a solid block, so marquee-text pixels landing inside its bounding box
   but outside those lit segments showed straight through underneath it. Fixed by clearing the full
   gauge-plus-muted-icon bounding box (`fillRect` in `DARK`) at the top of `renderBattery()`, before
   anything else is drawn there.
2. **Color choices could render as the same color as their background on displays that can't do
   color at all.** This phase's color-convention sweep picked `RED`/`GREEN`/`YELLOW` based on state
   (Bluetooth on/off, GPS fix, RX errors, danger-zone tint) and trusted each backend's `setColor()`
   to collapse them safely on monochrome hardware -- true for every backend audited, but an implicit
   assumption calling code had no way to check or rely on deliberately. Added
   `DisplayDriver::supportsColor()` (new virtual, defaults `true`) and a `Layout::accentColor()`
   helper that downgrades to plain `LIGHT` when it's `false`; overridden `false` in every backend
   confirmed to be a strict 1-bit/2-color buffer regardless of the physical panel
   (`SH1106Display`/`SSD1306Display`/`U8g2Display`/`GxEPDDisplay`/`E213Display`/`E290Display`, plus
   `ST7789Display` -- a real color TFT driven through ThingPulse's monochrome `OLEDDisplay` library,
   confirmed by its own `setColor()` already having every non-`DARK` case disabled). Every
   conditional color pick touched this phase now goes through `accentColor()` instead of calling
   `setColor()` with a raw hue directly.
3. **The screen-transition animation looked like a flash/bug, not a transition.** Reverted
   entirely -- see "Decisions" above and ARCHITECTURE.md for the full reasoning (every backend fully
   clears its buffer per redraw, so any animation built from real display updates is a sequence of
   hard-edged full-screen redraws, not a compositing effect; most pilots are monochrome besides,
   where a "wipe" has no gradient to soften it). `NavStack` is back to its pre-Phase-5 shape with no
   transition hook at all -- this wasn't judged worth a second blind attempt at different
   timings/step-counts with no way to see the result.
4. **Emoji in a user-chosen node name rendered as garbage in the status bar.**
   `UITask::updateStatusBar()` formatted `_node_prefs->node_name` straight into the scrolling text;
   every other screen that shows a user-supplied name (Contacts/Recents/message previews) already
   runs it through `DisplayDriver::translateUTF8ToBlocks()` first, which substitutes one block glyph
   per multi-byte UTF-8 sequence -- but the user specifically asked for emoji to be dropped rather
   than replaced with a block character (a run of blocks for a multi-codepoint emoji would still
   read as clutter in a compact single-line strip). Added a small `stripNonAscii()` in `UITask.cpp`
   that copies only bytes `< 0x80` and silently drops the rest, applied to the name before it's
   formatted into the status bar buffer.

### Not yet done / needs hardware verification

- **Nothing in Phase 5 has been flashed.** This is a bigger gap than usual: every prior phase had at
  least an informal `WioTrackerL1` pass before being called done. Highest-priority checks once a
  build is available:
  - Does `setTextWrap(false)` actually eliminate the status-bar/menu overlap on `WioTrackerL1`
    (`SH1106Display`) -- the one board it was originally reported on.
  - Does the Phase 4 status-bar-flash fix still hold (regression check the user specifically asked
    for early in this phase -- not done yet, no device pass has happened at all).
  - Do the new `Screen_Contacts`/`Screen_Channels` icons render without clipping/misalignment, and
    does `Screen_SettingsDanger`'s mixed icon/no-icon row alignment look right.
  - Does the fixed `StatusBar` battery-gauge clear actually stop the text/icon mixing on a real
    marquee scroll (fixed by hand-trace per "Bugs found in user review" above, not yet reflashed).
  - Does `Screen_SettingsDanger`'s red tint read clearly against the green selected-row highlight
    when a tinted row is the one currently selected (untested interaction between `tint` and the
    selected-row color, which always overrides to GREEN/DARK regardless of tint).
- `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` have never been physically tested on **any** phase
  0-5 -- Phase 5 is their first real hardware pass for everything built so far, not a regression
  sweep (see "Two things resolved before starting" above).
- `lilygo_techo` (`LilyGo_T-Echo_companion_radio_forest_ble`) has never been flashed with
  `ui-forest` at all -- first hardware pass for the whole UI on this board, plus the specific
  e-ink gating checks (no marquee, redraw cadence) phase-5-visual-polish.md calls for. (The
  "no transition animation on e-ink" check no longer applies -- there's no transition animation on
  any board now, see "Bugs found in user review" above.)
- `signal_bars_*`/`torch_icon`/`gps_fix_icon`/`gps_nofix_icon`/`settings_icon`/`event_log_icon` exist
  as assets but several are unwired anywhere yet (see "Decisions" above) -- worth a future phase
  actually rendering each one at least once (even in a throwaway test screen) to confirm they look
  like what they're supposed to before relying on them further.
- Per-screen redraw cadence (`render()`'s returned delay, ~200-1000ms across every screen) was not
  specifically tuned for e-ink -- phase-5-visual-polish.md's "redraw cadence respects
  `display.isEink()` wherever a cadence/animation decision exists" is addressed for the one cadence
  decision this phase kept (`StatusBar` marquee), not retrofitted across every screen's existing
  return-delay value. Worth revisiting if `lilygo_techo` testing shows redraws happening more often
  than an e-ink panel is comfortable with.
- The new `DisplayDriver::supportsColor()` overrides were traced against each backend's own
  `setColor()` implementation, not device-confirmed -- worth a real side-by-side look on
  `heltec_rc32` (`NV3001BDisplay`, should still show red/green/yellow) vs. any monochrome pilot
  (should now show every accent color as plain white/on) once hardware is available.

## Phase 6 — Input/control expansion: REGRESSION-CLEARED ON WioTrackerL1, NEW CAPABILITIES STILL ENTIRELY UNVERIFIED (no lilygo_tdeck/sensecap_indicator-espnow hardware available)

Spec: `phases/phase-6-input-control-expansion.md`. Same no-`pio`/no-`g++` limitation as every
prior phase, plus a bigger one specific to this phase: `lilygo_tdeck`'s keyboard co-processor
and trackball, and `sensecap_indicator-espnow`'s touch panel, are the first *new hardware
capabilities* (not just new screens/menus over what Phases 0-5 already exercised) this project
has wired up sight-unseen. Confidence is not uniform across this phase's three build items --
see "Confidence levels" below before trusting any one part of it equally.

### Confidence levels (read this before the device pass)

- **Dynamic on-screen control hints (item 32):** high confidence. Pure refactor of existing,
  already-working code paths (`InputRouter::activateHint()`/`moveHint()`/`textEntryHint()`
  replace literal strings and the ui-new-ported `PRESS_LABEL` macro) -- no new hardware
  interaction, just different text on screens that already render correctly today.
- **SenseCAP touch wiring (item 33):** medium-high confidence. `LGFXDisplay::getTouch()`
  already existed and, on reading it plus `SCIndicatorDisplay.h`'s `LGFX` config, turned out to
  already handle the two things phase-6.md flagged as unverified: it divides by `UI_ZOOM`
  before returning (so coordinates already come out in this board's logical `display.width()`/
  `height()` space, not raw panel pixels), and the touch controller (`Touch_FT5x06`) is
  pre-configured with `x_max=479`/`y_max=479` matching the panel's own 480x480 native
  resolution with rotation handled by LovyanGFX's own touch-to-panel-rotation mapping. This is
  "confirmed by reading the actual config in this repo," not a guess -- see "Resolved open
  questions" below. What's still unverified is only whether that LovyanGFX-internal rotation
  mapping actually lines up correctly in practice (no way to check without the physical panel).
- **T-Deck keyboard + trackball wiring (item 34): lower confidence, flagged prominently.**
  Nothing in this repo touched T-Deck's keyboard or trackball before this phase (confirmed by
  grep -- no TCA8418/keyboard/trackball reference anywhere in the codebase). The trackball pin
  assignments (`TDECK_TRACKBALL_UP/DOWN/LEFT/RIGHT` = GPIO 3/15/1/2 in `target.h`) and the
  keyboard's I2C protocol (single-byte ASCII read from address `0x55`, `TDeckKeyboard.h`) are
  both taken from the widely-published community T-Deck v1 pinout/protocol (the same one used
  by, among others, Meshtastic's T-Deck input driver and several open T-Deck example sketches)
  -- **not verified against a schematic or datasheet present in this repo**, since no T-Deck
  hardware or internet access to a primary source is available in this dev environment. This is
  meaningfully different from every prior "compiled but hardware-unverified" flag elsewhere in
  this project (e.g. `HAS_TORCH`/`PIN_STATUS_LED`), which exercised macros/hooks the codebase
  already defined -- here the pin numbers and I2C address themselves are new, external
  knowledge, not derived from anything already in this repo. If the keyboard/trackball don't
  respond on real hardware, this is the first thing to check, and the two files most likely to
  need a real-hardware correction are `variants/lilygo_tdeck/target.h` (pin macros) and
  `variants/lilygo_tdeck/TDeckKeyboard.h` (I2C address/protocol).

### Resolved open questions (from phase-6.md and PLAN.md §8)

- **`LGFXDisplay::getTouch()`'s coordinate/rotation semantics:** resolved by reading the code,
  not by device test. `LGFXDisplay.cpp`'s existing `getTouch()` implementation already divides
  the raw `lgfx::v1::touch_point_t` by `UI_ZOOM` before returning -- the exact same scaling
  `endFrame()` applies when compositing the sprite buffer onto the physical panel -- so a touch
  point and a screen coordinate computed by any `ui-forest` screen (which only ever deals in
  `display.width()`/`height()`, i.e. the post-`UI_ZOOM` logical size) are already in the same
  space with zero adjustment needed in `InputRouter`. Separately, `SCIndicatorDisplay.h`'s `LGFX`
  config gives its `Touch_FT5x06` instance `x_max=479`/`y_max=479`, matching the panel's native
  480x480 resolution one-for-one -- rotation itself (`panel.offset_rotation=1` vs.
  `touch.offset_rotation=0`) is handled by LovyanGFX internally mapping touch coordinates through
  whatever rotation `setRotation()` last applied to the panel, which is exactly the mechanism
  this two-line difference is designed to feed. Not re-verified by an actual finger on an actual
  panel -- flagged in "Not yet done" below.
- **`getTouch(int*, int*)` not being on the shared `DisplayDriver*` `InputRouter`/`UITask` hold:**
  resolved via the virtual-method route phase-6.md itself leaned towards -- `DisplayDriver::
  getTouch()` (new, default `return false`) added right alongside `isEink()`/`supportsColor()`,
  same "default no-op override in the one backend that has it" convention; `LGFXDisplay::
  getTouch()` marked `override`. `UITask` gains one new thin passthrough (`bool getTouch(int*,
  int*) const`), matching `isDisplayOn()`'s existing shape, so `InputRouter` never needs a cast
  or a new accessor.
- **Whether `HAS_TOUCH` needed a fresh gating convention:** yes, and it's now the same shape as
  every other board-capability macro `InputRouter` already checks (`#if defined(HAS_TOUCH)`),
  consulted only if no higher-priority input source already produced a key this frame, same
  precedence rule the rotary/analog/torch blocks already follow.
- **Whether `sensecap_indicator-espnow`'s ESPNOW transport needs special handling from Phase 4's
  transport-indicator work:** checked `Transport.h` -- its precedence chain is `WIFI_SSID` /
  `BLE_PIN_CODE` / `ETHERNET_ENABLED` / else-USB, and this board's new forest env defines none of
  those three, so it falls through to the `#else` branch and reports `"USB"`. That's wrong (this
  board's actual transport is ESPNOW, wired via `helpers/esp32/ESPNOWRadio.cpp` in its
  `platformio.ini`, not picked via any of `main.cpp`'s `BaseSerialInterface` `#if` chain at all --
  confirmed by re-reading `main.cpp`'s precedence list, none of which mentions ESPNOW) -- **but
  deliberately left unfixed this phase**, since fixing `Transport.h` correctly means also
  checking how `main.cpp` decides to skip the whole `BaseSerialInterface` selection for ESPNOW
  boards in the first place, which is outside this phase's stated scope (item 32-34 only) and
  risks a wider regression across every board for a cosmetic-only diagnostics-screen/status-bar
  label. Flagged here as a known-wrong value (`Screen_DiagCore`'s Transport row and the status
  bar's transport label will both show "USB" on this board) rather than silently left for
  someone to rediscover as a mystery.

### Built

All new/changed in `examples/companion_radio/ui-forest/` unless noted, per phase-6.md:

- **Dynamic control hints (item 32)** -- `InputRouter` gained three static, stateless helpers
  (`activateHint()`, `moveHint()`, `textEntryHint()`) that return a short, this-board's-real-
  gesture label ("long press"/"tap"/"click"/"press Enter"/"press"; "click/dbl-click"/"swipe"/
  "trackball"/"stick"/"turn") based purely on which board macros are compiled in -- no instance
  state needed, so every screen calls them directly. Replaced ui-new's `PRESS_LABEL` macro
  (ported verbatim into three ui-forest screens in Phase 1) in `Screen_Bluetooth.cpp`/
  `Screen_Advert.cpp`/`Screen_Shutdown.cpp`, and the literal "ENTER: toggle" / "< adjust
  ENTER: save" / "PREV: next char, ENTER: save" hint strings in `FormField.cpp`'s `ToggleField`/
  `StepperField`/`TextField`. `TextField` gets its own two-clause builder (`textEntryHint()`)
  since it needs to advertise both input paths on keyboard boards, not just relabel one gesture.
- **SenseCAP touch wiring (item 33)** -- `DisplayDriver::getTouch(int*, int*)` (new virtual,
  default `false`) in `src/helpers/ui/DisplayDriver.h`; `LGFXDisplay::getTouch()` marked
  `override` (`src/helpers/ui/LGFXDisplay.h`); `UITask::getTouch(int*, int*) const` passthrough
  (`UITask.h`). `InputRouter::poll()` gained a `#if defined(HAS_TOUCH)` block: tracks touch-down/
  touch-up across polls, and on touch-up classifies the total start-to-end delta as a tap
  (`KEY_ENTER`, if both axes moved less than `TOUCH_SWIPE_THRESHOLD`=12px) or a swipe on
  whichever axis moved more (`KEY_NEXT`/`KEY_PREV` -- right/up = next, left/down = prev,
  matching `MenuScreen`'s existing "any of NEXT/DOWN/RIGHT move the cursor down" equivalence, see
  ARCHITECTURE.md). Purely additive: this board's existing `PIN_USER_BTN=38` fallback button
  (single-button vocabulary, already wired since this board's `platformio.ini` already defined
  the pin) is untouched by this change.
- **T-Deck keyboard + trackball wiring (item 34)** -- new `variants/lilygo_tdeck/TDeckKeyboard.h`
  (header-only; see "Confidence levels" above for its I2C address/protocol provenance).
  `variants/lilygo_tdeck/target.h`/`.cpp` gained four `MomentaryButton` trackball-direction
  globals (`trackball_up/down/left/right`, pins via new `TDECK_TRACKBALL_*` macros,
  overridable via build_flags) and one `TDeckKeyboard tdeck_keyboard` global, following the
  exact "declare pins as `#ifndef`-guarded defaults in target.h, instantiate in target.cpp"
  pattern `TDeckBoard.h`'s own `PIN_VBAT_READ` already uses, and the exact `MomentaryButton`
  wiring style `wio-tracker-l1`'s Phase 0 `joystick_up`/`joystick_down` addition already
  established (see this file's Phase 0 section) -- multiclick explicitly disabled
  (`MomentaryButton`'s last ctor arg) so a rolling trackball's rapid pulses each become one key
  immediately instead of waiting out a double/triple-click window. `InputRouter::poll()` gained
  a `#if defined(LILYGO_TDECK)` block: trackball directions map to `KEY_UP/DOWN/LEFT/RIGHT`
  (additive to the existing `PIN_USER_BTN=0` click-only wiring, only consulted if that branch
  didn't already produce a key this frame); the keyboard's raw ASCII byte is fed straight
  through as the returned key code, throttled to a 20ms poll interval (mirrors
  `PIN_USER_BTN_ANA`'s own ADC-read throttling). `FormField.cpp`'s `TextField::handleInput()`
  gained the actual typing behavior: any byte in `[32,127)` is written at the cursor and the
  cursor advances (capped once the buffer is full, see "Real bug found and fixed" below);
  8/127 (backspace/delete) erase. This never collides with the existing `KEY_*`/increment-picker
  vocabulary -- see the in-code comment on why -- and is a no-op on every screen except
  `TextField` by construction (every other screen's `handleInput()` ignores unrecognized bytes).
  The increment-based picker (`KEY_NEXT`/`KEY_PREV`/`KEY_ENTER`) is untouched and still reachable
  via the trackball's click button's single-button vocabulary, matching phase-6.md's explicit
  "second path, not a replacement" framing.
- **New envs**: `LilyGo_TDeck_companion_radio_forest_ble` (`variants/lilygo_tdeck/
  platformio.ini`) and `SenseCapIndicator-ESPNow_comp_radio_forest_usb`
  (`variants/sensecap_indicator-espnow/platformio.ini`) -- both the same two-line-diff pattern
  (`-I` path, `ui-*/*.cpp` glob) as every other forest env, extending each board's shared base
  section (not the existing `_ble`/`_usb` env) per PLAN.md §5, mirroring `WioTrackerL1_
  companion_radio_forest_ble`'s exact template.

### Real bug found and fixed during tracing (no compiler available, same discipline as Phase 3)

`TextField::handleInput()`'s first draft of the typed-character path advanced `_cursor` up to
`_max_len` unconditionally once the field is full, which doesn't match the invariant the
existing `KEY_PREV` cursor-advance logic already relies on (cursor never reaches `_max_len`
once `_len == _max_len` -- there's no append slot left once full, so `_len` itself isn't a valid
cursor position at that point, only `_len - 1` is). Left as originally written, a full text
field would let the cursor drift one past the last real character and start silently
overwriting the buffer's own null-terminator byte on every subsequent keystroke (each such byte
stays inside the 32-byte `_buf` array -- not a memory-safety bug -- but the string would render/
commit correctly only because `render()`'s `shown` copy and the `KEY_ENTER` commit path both
independently re-truncate/re-terminate; still clearly not the intended behavior). Fixed by
capping the post-write cursor at `_len` when there's still an append slot, or `_len - 1` once
full -- the same value `KEY_PREV`'s own modulo arithmetic already treats as the ceiling. Caught
by tracing the exact cursor-position invariant end to end before calling this done, not by a
compiler/sanitizer -- same category of bug as Phase 3's `TextField::begin()` buffer overflow.

### Decisions / deviations worth knowing about

- **`backHint()` was designed, then removed before committing.** The natural "activate/back/
  move" triad would have included a per-board Cancel-gesture label, but grep found zero existing
  hint-text call sites anywhere in `ui-forest` that mention Cancel/back (`MenuScreen` shows no
  button hints at all; the only five hint-text locations in the whole codebase are the three leaf
  screens and three `FormField` editors this phase already touches, none of which describe
  Cancel). Cut rather than shipped as unused public API -- easy to re-add if a future phase finds
  a real call site.
- **Touch swipe direction mapping (right/up = next, left/down = prev) is a judgment call, not
  something phase-6.md pins down exactly.** phase-6.md explicitly leaves this open ("swipe
  (direction inferred from touch-start/touch-end delta) -> KEY_NEXT/KEY_PREV (or LEFT/RIGHT,
  matching whatever MenuScreen already expects")). Chose "up or right = next" to match a common
  mobile-scroll convention (swiping up reveals content below, i.e. moves the selection down the
  list); easy to flip if it feels backwards on real hardware -- it's a two-line change in
  `InputRouter.cpp`'s `HAS_TOUCH` block.
- **`Transport.h` now reports the wrong transport name for `sensecap_indicator-espnow`** ("USB"
  instead of "ESPNOW") -- see "Resolved open questions" above for why this was flagged rather
  than fixed in this phase.
- **Keyboard input is fed as a raw byte, not translated into a synthetic `KEY_*` code**, on the
  reasoning that plain printable ASCII (32-126) can never collide with the existing vocabulary
  (0xB4-0xF3 plus three control codes already claimed by SELECT/ENTER/CANCEL, which a real
  keyboard's own Enter/Escape keys are expected to send anyway) -- see the in-code comment in
  `InputRouter.cpp`. This means `TextField` is the only screen that needs to know keyboards
  exist at all; `InputRouter`, `MenuScreen`, and every other screen stay completely unaware.

### Not yet done / needs hardware verification

- **Nothing in Phase 6 has been flashed or built with a real compiler.** Same no-`pio`/no-`g++`
  limitation as every prior phase, compounded here by two boards (`lilygo_tdeck`,
  `sensecap_indicator-espnow`) that have never run any phase of `ui-forest` before this one --
  this is a first-time hardware pass for Phases 0-6 combined on both, not a regression check.
- **Highest-priority check: does `tdeck_keyboard`/the trackball pins respond at all** -- see
  "Confidence levels" above. If not, `variants/lilygo_tdeck/target.h`'s `TDECK_TRACKBALL_*`
  macros and `TDeckKeyboard.h`'s I2C address/protocol are the first things to correct against
  real hardware (a logic analyzer or oscilloscope on the trackball pins, and an I2C scanner
  sketch for the keyboard, would resolve this faster than guessing further from here).
- Touch tap/swipe on `sensecap_indicator-espnow` has never been tried against the physical
  panel -- the coordinate/rotation reasoning in "Resolved open questions" is a paper argument
  from reading `LGFXDisplay.cpp`/`SCIndicatorDisplay.h`, not a finger-on-glass confirmation.
  `TOUCH_SWIPE_THRESHOLD`'s 12px value is a guess, not tuned against the panel's actual
  480x480/`UI_ZOOM=3.5` scale -- worth adjusting if taps register as swipes or vice versa.
- The `sensecap_indicator-espnow` forest env's Transport label (known-wrong, "USB" instead of
  "ESPNOW", see "Decisions" above) hasn't been visually confirmed on the actual Diagnostics/
  status-bar screens, only reasoned through from `Transport.h`'s source.
- Regression check called for by phase-6.md's own checklist -- "every screen from Phases 1-5
  still reachable and functional on both boards via their existing single-button/trackball-click
  fallback" -- hasn't been attempted on either board (no device pass at all yet).
- **Regression check: done, on `WioTrackerL1` (`WioTrackerL1_companion_radio_forest_ble`) --
  user flashed and reports everything is fine.** `HAS_TOUCH`/`LILYGO_TDECK` are undefined for
  this board, so none of this phase's new `InputRouter` blocks compile in on it at all -- this
  confirms the shared-file edits that DO apply to every board regardless
  (`DisplayDriver::getTouch()`'s new virtual, the `FormField.cpp`/`Screen_Bluetooth`/`Advert`/
  `Shutdown.cpp` hint-string changes, `InputRouter::activateHint()`/`moveHint()`'s new
  single-button-branch labels) didn't regress anything on the one board with an actual track
  record across every prior phase. Informal pass, not phase-6.md's own checklist run.
- **The user has no `lilygo_tdeck` or `sensecap_indicator-espnow` hardware.** This is the
  important caveat: the WioTrackerL1 pass above is a regression check on *unrelated* code paths,
  not a confirmation of anything this phase actually built. Touch tap/swipe, keyboard typing,
  and trackball navigation remain **completely unverified** -- exactly as unverified as the
  moment this phase's code was written. Whoever next has access to either board (or wants to
  order one) is the one who can actually close out "Confidence levels" above; until then, treat
  every claim in that section as a paper argument, not a test result, regardless of how this
  phase's status line reads.
- `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32`/`lilygo_techo` remain build-unverified for this
  phase (same gap carried since Phase 0) -- only `WioTrackerL1` has any hardware track record.
