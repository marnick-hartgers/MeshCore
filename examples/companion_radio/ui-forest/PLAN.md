# ui-forest — build plan

Status: **planning only, no code written yet.** This document is the complete spec for a
new `companion_radio` UI variant, `ui-forest`, meant to replace `ui-new` as the polished,
"nerd stats and settings" UI once it reaches parity + the new feature set below. It was
written by researching the existing `ui-new` / `ui-orig` / `ui-tiny` implementations, the
shared `src/helpers/ui/` framework, and `MyMesh.cpp`'s binary protocol handlers, so that a
fresh session with no memory of that research can build straight from this file. Every
class name, method name, and file path below was verified against the current codebase at
plan-writing time — where something couldn't be verified, it's called out explicitly in
[Section 8](#8-open-questions--verify-during-build) rather than assumed.

Build one phase at a time, in order (each depends on the previous). Don't skip ahead to
Settings/Diagnostics before the navigation framework in Phase 0 exists — every later phase
is a set of menu entries hung off it.

## Contents

1. [Goals & non-goals](#1-goals--non-goals)
2. [Hardware reality this has to survive](#2-hardware-reality-this-has-to-survive)
3. [Architecture](#3-architecture)
4. [File layout](#4-file-layout)
5. [Rollout mechanism (non-destructive)](#5-rollout-mechanism-non-destructive)
6. [Phased roadmap](#6-phased-roadmap)
7. [Feature → phase → class traceability](#7-feature--phase--class-traceability)
8. [Open questions / verify during build](#8-open-questions--verify-during-build)
9. [Manual test checklist template](#9-manual-test-checklist-template)

---

## 1. Goals & non-goals

**Goals**
- One coherent, navigable UI (real menus, not a flat ring of swipeable pages) that scales
  from a 128×64 monochrome OLED up to a 480×480 color touchscreen without three separate
  hand-tuned implementations.
- Surface data that already exists in the firmware today but has never been shown on any
  screen: radio/packet/core stats, transport type, full contact/channel address book.
- A real on-device Settings area wired to the same `NodePrefs` fields the companion app
  edits today, so the device is usable without a phone once this ships.
- Works across every control scheme currently in the repo: single button, joystick+back,
  rotary encoder, analog button, capacitive touch, keyboard+trackball.
- Visual polish (icons, separators, color convention) applied consistently, not per-screen
  ad hoc.

**Non-goals (explicitly out of scope for this plan)**
- Migrating any existing board's *default* build env from `ui-new`/`ui-orig`/`ui-tiny` to
  `ui-forest`. `ui-forest` ships as new, additive, opt-in envs (see
  [Section 5](#5-rollout-mechanism-non-destructive)); deciding to make it the default for a
  given board is a separate, later decision.
- Deleting or refactoring `ui-new`/`ui-orig`/`ui-tiny`.
- Promoting any generic widget out to `src/helpers/ui/` for reuse by other example apps.
  Everything lives inside `examples/companion_radio/ui-forest/`, matching how the user
  framed this project — revisit only if a future session explicitly decides otherwise.
- A file-backed packet/debug log. `companion_radio` has no log-file subsystem today (see
  [Section 8](#8-open-questions--verify-during-build)); the "log viewer" idea is rescoped to
  an in-RAM ring buffer, not a log file.
- Full companion-app-equivalent contact/channel *editing* (add/delete/rename from the
  device). Phase 2 is read-only browsing; editing is a stretch goal, not in this plan.

---

## 2. Hardware reality this has to survive

Verified by grep across `variants/*/platformio.ini` and `target.h`, not assumed:

| Control scheme | How it shows up in code | Example board (verified) |
|---|---|---|
| Single momentary button | `PIN_USER_BTN` + `MomentaryButton` (click/double/triple/long) | `RAK_4631` — SSD1306 OLED |
| 3-button joystick + back | `UI_HAS_JOYSTICK=1`, separate `joystick_left`/`joystick_right`/`back_btn` | `gat562_30s_mesh_kit` — SSD1306 OLED |
| Rotary encoder + button | `UI_HAS_ROTARY_INPUT`, `RotaryInput` interface + `user_btn` | `heltec_rc32` — NV3001B color TFT |
| Analog resistor-ladder button | `PIN_USER_BTN_ANA`, same click/double/triple/long vocabulary | several ESP32 boards |
| Capacitive backlight toggle | `BACKLIGHT_BTN` / `PIN_BUTTON2` | LilyGo T-Echo family |
| Torch double-click | `HAS_TORCH`, gated on back-button double-click | `lilygo_techo_lite` |
| Capacitive touchscreen | `LGFXDisplay::getTouch(x,y)` exists at driver level, **used by zero example code today** | `sensecap_indicator-espnow` — 480×480 ST7701 + FT5x06 touch, currently just runs `ui-new` on a plain button |
| Keyboard + trackball | Hardware exists, **currently wired as a single `PIN_USER_BTN` (trackball click only)**, keyboard matrix and directional trackball movement unused | `lilygo_tdeck` — ST7789LCD color |

Display backends behind the shared `DisplayDriver` abstraction (`src/helpers/ui/`):
monochrome OLED (SSD1306/SH1106 via U8g2), color TFT (ST7735/ST7789/ST7789LCD, NV3001B,
LGFX/LovyanGFX for RGB panels), e-ink (GxEPD-based, custom E213/E290). `DisplayDriver`
exposes logical colors (`DARK/LIGHT/RED/GREEN/BLUE/YELLOW/ORANGE`) that collapse to on/off
on monochrome, and `isEink()` for refresh-cadence decisions — reuse both, don't reinvent.

**Pilot boards for this build** (one per axis above, chosen because each already builds
`companion_radio` + `ui-new` today, confirmed via each board's `platformio.ini`):

1. `RAK_4631` — single button, monochrome OLED, tight RAM (nRF52840). Baseline correctness
   + memory-budget canary.
2. `gat562_30s_mesh_kit` — joystick+back, monochrome OLED. Multi-button navigation canary.
3. `heltec_rc32` — rotary + button, small color TFT. Rotary input + color canary.
4. `lilygo_tdeck` — keyboard+trackball, color TFT. Deferred to Phase 6; Phases 0–5 treat it
   as "single button" (trackball click) same as today.
5. `sensecap_indicator-espnow` — touchscreen, large color panel. Deferred to Phase 6.
6. Pick one e-ink board (`heltec_e213` or `lilygo_techo`) once Phase 5 (transitions/e-ink
   gating) is reached — not needed before then.

Don't try to build/test against all ~85 variants during development. Add new envs only to
these pilot boards' `platformio.ini` files (see Section 5); widen coverage in Phase 8.

---

## 3. Architecture

### 3.1 Input abstraction

Reuse the existing logical key vocabulary from `src/helpers/ui/UIScreen.h` verbatim —
`KEY_LEFT/UP/DOWN/RIGHT/SELECT/ENTER/CANCEL/HOME/NEXT/PREV/CONTEXT_MENU`. Don't invent a
new event enum; every screen already speaks this language via `UIScreen::handleInput(char c)`.

What's new: centralize the per-board `#ifdef` block that `ui-new`'s `UITask::loop()` uses
today (single button vs. joystick vs. rotary vs. analog, each mapped inline in one giant
function) into one `InputRouter` that all boards go through. Concretely:

- `InputRouter::poll()` runs the same board-specific `#ifdef UI_HAS_JOYSTICK` / `#elif
  defined(PIN_USER_BTN)` / `#if defined(UI_HAS_ROTARY_INPUT)` / `#if defined(PIN_USER_BTN_ANA)`
  branches `ui-new`'s `loop()` already has (`examples/companion_radio/ui-new/UITask.cpp:713-785`
  is the reference implementation — copy the gesture-to-key mapping, don't redesign it) but
  returns a single `char` key code instead of directly calling `curr->handleInput()`.
- `UITask::loop()` stays the single call site that takes that key code and feeds it to
  `NavStack::current()->handleInput(c)` — this is the only place gesture-to-key mapping
  lives, so adding a new control scheme later (or wiring up touch/keyboard in Phase 6) means
  editing `InputRouter` once, not every screen.
- Single-button devices keep the existing gesture vocabulary: click=NEXT, double=PREV,
  triple=SELECT (context menu / mute, screen-dependent), long=ENTER-ish (CLI rescue in first
  8s, else app-defined). Joystick/rotary boards keep using their native LEFT/RIGHT/ENTER
  gestures directly — don't force every board through the single-button vocabulary.
- New in Phase 2: a long-press-anywhere (or, on joystick boards, the back-button
  triple-click that today only exists for mute) maps to `KEY_HOME`, handled once in
  `NavStack` (pop to root) rather than by individual screens.

### 3.2 Navigation & menu framework

This is the structural change everything else depends on. `ui-new` today has one `curr`
pointer and a hand-rolled `_page` enum per screen with no "back" concept
(`examples/companion_radio/ui-new/UITask.cpp:87-101`, `HomePage` enum). Replace with:

- **`NavStack`** — a small fixed-depth stack (8 entries is plenty; this is a UI, not a call
  graph) of `UIScreen*`. `push(UIScreen*)`, `pop()`, `popToRoot()`, `current()`. Screens
  don't own navigation decisions about *other* screens — a screen calls
  `nav.push(&contactDetailScreen)` when the user selects a contact, and `KEY_CANCEL`/back
  pops it. All screens are constructed once in `UITask::begin()` (same "no allocation
  outside setup" convention `ui-new` already follows — it does `new SplashScreen(...)` etc.
  once in `begin()`) and reused; `NavStack` just moves pointers, never allocates.
- **`MenuScreen : public UIScreen`** — the single generic widget that implements Home,
  Settings, Diagnostics, Contacts list, Channels list, and every settings sub-menu. Takes a
  `MenuItem` array (label, optional icon, kind, payload) + count. Handles NEXT/PREV to move
  a selection cursor with scroll-into-view once the list exceeds the visible-row count
  (computed by `Layout`, see below), ENTER to activate the selected item, CANCEL to
  `nav.pop()`. `MenuItem::kind` is one of: `Action` (invoke a callback), `Submenu` (push
  another `MenuScreen`), `Toggle`/`Stepper`/`Enum`/`Text` (push the matching field editor from
  `FormField`, see 3.4), `Info` (non-interactive row, e.g. a stat readout).
- **`ConfirmScreen : public UIScreen`** — yes/no or "hold N seconds to confirm" (reuse the
  existing shutdown-hold pattern from `ui-new`'s `HomeScreen::poll()` /
  `_shutdown_init`/`isButtonPressed()` — `examples/companion_radio/ui-new/UITask.cpp:184-188`
  — as the reference for the hold-to-confirm gesture on single-button devices). Reused for
  shutdown, factory erase, identity rekey, reboot, restore-defaults.
- **`ToastOverlay`** — generalizes `ui-new`'s existing alert-box-over-current-screen logic
  (`UITask::loop()`, the `_alert`/`_alert_expiry` block that draws a bordered box over
  whatever `curr->render()` just drew) into its own small class with `show(text, millis)` +
  `composite(DisplayDriver&)` called from `UITask::loop()` after `NavStack::current()->render()`.
  Behavior is identical to today, just no longer duplicated ad hoc.
- **`StatusBar`** — generalizes `ui-tiny`'s `ScrollingStatusBar`
  (`examples/companion_radio/ui-tiny/ScrollingStatusBar.h`) into an optional, always-rendered
  top strip. Keep its change-detection (`update()` only rebuilds the string when a watched
  value actually changed) and marquee-scroll-if-too-wide behavior as-is — it's already
  well-built for this. Extend the fields it watches to include transport type (Phase 4) and
  render small icons instead of `"BLE:ON"`-style text where the display is wide/tall enough
  (`display.height() > ~80px`, otherwise keep the text form for tiny screens).

### 3.3 Adaptive layout

`ui-new`'s screens hardcode pixel y-offsets per row (`y=18`, `y=20`, `y+=11`, scattered
through `examples/companion_radio/ui-new/UITask.cpp`). That's why it needed three separate
UIs for three screen sizes. Instead:

- A `Layout` helper computes `rowHeight()` (font height for the current `setTextSize()` +
  fixed padding) and `visibleRows(DisplayDriver&)` (`(display.height() - headerHeight -
  statusBarHeight) / rowHeight()`) once per frame. `MenuScreen` and every list-style screen
  use this instead of a literal constant, so the same code shows 4 rows on a 64px OLED and
  10+ rows on a 240px+ color TFT with zero per-board tuning.
- A screen-header convention (icon + title, ~one row tall) is drawn by a shared helper, not
  copy-pasted per screen — this is what gives every screen the "where am I" consistency
  called for in the feature list.
- E-ink gating: anywhere a redraw cadence or animation is decided, check
  `display.isEink()` first (same pattern `ui-new` already uses for the shutdown-delay: `if
  (_display->isEink() == false) { delay(3000); }`). No transitions, longer minimum
  redraw intervals, and no marquee-scrolling `StatusBar` on e-ink — show the truncated/static
  form instead.

### 3.4 Data integration — verified call sites

This is the part most likely to go wrong if guessed instead of checked, so every entry
below was traced through `MyMesh.cpp`/`.h` and `BaseChatMesh.h` before being written down.

| Feature | Call from `UITask`/screens | Status |
|---|---|---|
| Contacts list | `the_mesh.getNumContacts()`, `the_mesh.getContactByIdx(idx, ContactInfo&)` | **Already public** (`BaseChatMesh.h:176-177`) — zero new API |
| Channels list | `the_mesh.getChannel(idx, ChannelDetails&)`, iterate `0..MAX_GROUP_CHANNELS-1`, skip empty `name[0]==0` slots (same pattern as today's recents list) | **Already public** — zero new API |
| Contact detail fields | `ContactInfo`: `name`, `type` (`ADV_TYPE_*`), `out_path_len` (`0xFF`=flood), `last_advert_timestamp`, `gps_lat/gps_lon`, `id.pub_key` | Struct already has everything needed (`src/helpers/ContactInfo.h`) |
| Radio params / TX power settings | Write `_node_prefs->freq/bw/sf/cr` or `tx_power_dbm`, call `the_mesh.savePrefs()`, **and also** `radio_driver.setParams(freq,bw,sf,cr)` / `radio_driver.setTxPower(power)` to apply live | **Must mirror `MyMesh::handleCmdFrame`'s `CMD_SET_RADIO_PARAMS`/`CMD_SET_RADIO_TX_POWER` handlers** (`MyMesh.cpp:1373-1418`) — a settings screen that only writes prefs and skips the `radio_driver` call will silently not apply until reboot. This is the single easiest correctness bug to introduce in Phase 3 — call it out in review. |
| Simple toggle/enum settings (repeat, rx-boost, telemetry mode, autoadd policy, etc.) | `_node_prefs->field = value; the_mesh.savePrefs();` | **Exact pattern `ui-new` already uses** for GPS/buzzer toggles today (`UITask::toggleGPS()`/`toggleBuzzer()`, `ui-new/UITask.cpp:896-946`) — just extend it to more fields |
| Advert name change | Mirror validation in `MyMesh::handleCmdFrame`'s `CMD_SET_ADVERT_NAME` handler (`MyMesh.cpp:1206`) before writing `node_name` | Verify exact validation rule during Phase 3 (see Section 8) |
| Radio stats (noise floor / RSSI / SNR) | `radio_driver.getNoiseFloor()`, `.getLastRSSI()`, `.getLastSNR()` | **Already used by `ui-new` today** (its Radio page calls `radio_driver.getNoiseFloor()` already) — zero new API |
| Packet stats | `radio_driver.getPacketsRecv()/getPacketsSent()/getPacketsRecvErrors()`, `the_mesh.getNumSentFlood()/getNumRecvFlood()` | **Already public** — `getPacketsRecv` etc. are on the `radio_driver` global; `getNumSentFlood`/`getNumRecvFlood` are public on `Dispatcher` (`src/Dispatcher.h:186,188`), inherited by `MyMesh` — zero new API. Confirm the `*_direct` counterparts exist alongside (likely `getNumSentDirect`/`getNumRecvDirect` on the same class) when building this screen. |
| Core/queue stats | Outbound queue length | **Needs one new one-line passthrough on `MyMesh`**: `uint32_t getQueueLen() const { return _mgr->getOutboundTotal(); }`. `_mgr` is private to `Dispatcher`; `UITask` isn't part of that hierarchy, so it can't reach it directly. This mirrors the existing `getBLEPin()`/`getRecentlyHeard()` passthrough pattern already on `MyMesh` — same shape, same file, one line. |
| Uptime | `rtc_clock` (already an extern in every board's `target.h`, already passed into `ui-new`'s `HomeScreen` as `&rtc_clock`) or simply `millis()/1000` | No new API |
| Transport indicator | `the_mesh.isSerialEnabled()`-equivalent per transport already exists for BLE (`isSerialEnabled()`/`hasConnection()` on `AbstractUITask`); confirm whether USB/WiFi/Ethernet builds expose an equivalent transport-type flag or whether this needs a small addition — the four transports are picked at compile time in `main.cpp` (`SerialBLEInterface`/`SerialWifiInterface`/`ArduinoSerialInterface`/`SerialEthernetInterface`), so "which transport" can likely just be a compile-time constant string set once in `UITask::begin()`, not a runtime query | Verify during Phase 4 |
| Event log ("nerd" debug log) | No existing subsystem — `companion_radio`'s `MyMesh::logRxRaw()` override is empty today, and the `log start`/`log stop`/`log erase` CLI commands found during research belong to `CommonCLI`, which **`companion_radio` does not use** (that's wired up in `simple_repeater`/`simple_room_server`/`simple_sensor` instead) | **Rescoped**: build a small in-RAM ring buffer (e.g. last 20 short event strings — advert sent/received, contact discovered, low battery, radio param changed) fed by `UITask` itself as things already visible to it happen, *not* a file-backed log. No new API on `MyMesh` needed. |

### 3.5 Icons

Icons stay hand-authored monochrome XBM byte arrays, same as `ui-new/icons.h` and
`ui-tiny/u8g2_icons.h` today — there's no runtime image decoding anywhere in this stack, and
adding one would violate the no-heap-after-setup convention for no real benefit at these
sizes. `DisplayDriver::drawXbm()` + `setColor()` already tints a monochrome bitmap for color
displays, so one bitmap set serves OLED/TFT/e-ink alike; no separate color-icon path needed.

Adopt two fixed sizes: 16×16 for inline list-row icons (contacts, channels, menu entries),
32×32 for full-screen glyphs (advert, bluetooth, power, torch — same size `ui-new` already
uses). New icons needed beyond today's set (bluetooth on/off, power, advert, muted, logo):
signal-bars (multi-frame, driven by RSSI/SNR thresholds), GPS fix/no-fix, contact, channel,
gear/settings, warning, torch, log/event. Author as 1-bit images and convert to XBM — check
first whether the repo already has a conversion script/convention before reaching for an
external tool (none was found during this planning pass, so budget time for this in Phase 5).

### 3.6 Memory/footprint discipline

Same rule as the rest of the codebase: no dynamic allocation outside `setup`/`begin`. All
~25 screens get `new`'d once in `UITask::begin()`, exactly like `ui-new` does today for its 3
screens — more instances, same pattern, not a new one. `MenuItem` arrays for fixed menus
(Settings tree, Diagnostics tree) should be `static const` tables, not built at runtime.
Contacts/Channels screens must read through `getContactByIdx`/`getChannel` on demand by
index (same as today's `getRecentlyHeard`) — never copy all 100 contacts into a new buffer.
`RAK_4631` (nRF52840, tightest RAM among the pilot boards) is the canary: if it doesn't fit,
trim there first rather than special-casing every other board.

---

## 4. File layout

Flat directory, no subfolders — this matches `ui-new`/`ui-orig`/`ui-tiny` exactly and avoids
an unverified assumption about whether PlatformIO's `build_src_filter` glob recurses into
subdirectories (see [Section 8](#8-open-questions--verify-during-build)). Organize by
filename prefix instead:

```
examples/companion_radio/ui-forest/
  PLAN.md                        <- this file
  UITask.h  UITask.cpp           <- entry point, same contract as ui-new/ui-orig/ui-tiny

  NavStack.h  NavStack.cpp
  InputRouter.h  InputRouter.cpp
  Layout.h
  MenuScreen.h  MenuScreen.cpp
  ConfirmScreen.h  ConfirmScreen.cpp
  ToastOverlay.h  ToastOverlay.cpp
  StatusBar.h  StatusBar.cpp
  FormField.h  FormField.cpp       <- Toggle/Stepper/Enum/Text field editors

  Screen_Splash.h/.cpp
  Screen_Home.h/.cpp                 <- top-level menu (Phase 2+), simple status page (Phase 1)
  Screen_Status.h/.cpp               <- msg count / connection / BLE pin (today's HomePage::FIRST)
  Screen_Recents.h/.cpp
  Screen_RadioInfo.h/.cpp
  Screen_Bluetooth.h/.cpp
  Screen_Advert.h/.cpp
  Screen_Gps.h/.cpp                  <- #if ENV_INCLUDE_GPS
  Screen_Sensors.h/.cpp              <- #if UI_SENSORS_PAGE
  Screen_Shutdown.h/.cpp
  Screen_MsgPreview.h/.cpp

  Screen_Contacts.h/.cpp
  Screen_ContactDetail.h/.cpp
  Screen_Channels.h/.cpp

  Screen_Settings.h/.cpp              <- root settings menu
  Screen_SettingsRadio.h/.cpp
  Screen_SettingsAdvert.h/.cpp
  Screen_SettingsNetwork.h/.cpp       <- repeat, rx-gain, duty cycle, telemetry, autoadd
  Screen_SettingsDevice.h/.cpp        <- name, buzzer, vibration, notifications entry point
  Screen_SettingsDanger.h/.cpp        <- erase, rekey, reboot, restore-defaults (all confirm-gated)

  Screen_Diagnostics.h/.cpp           <- root diagnostics menu
  Screen_DiagRadio.h/.cpp
  Screen_DiagPackets.h/.cpp
  Screen_DiagCore.h/.cpp
  Screen_EventLog.h/.cpp              <- in-RAM ring buffer viewer (rescoped, see 3.4)

  Screen_RecentEvents.h/.cpp          <- user-facing notification history (distinct from EventLog — see 7)
  Screen_NotificationSettings.h/.cpp

  icons.h
```

Add files as needed if a screen turns out to need a helper class; don't pre-create empty
stubs for all of these before Phase 0 — this is the target shape, not a checklist to fill in
mechanically on day one.

---

## 5. Rollout mechanism (non-destructive)

Same wiring mechanism `ui-new` already uses — verified against
`variants/rak4631/platformio.ini`:

```ini
build_flags =
  ...
  -I examples/companion_radio/ui-new       ; <- becomes ui-forest
build_src_filter = ${rak4631.build_src_filter}
  +<../examples/companion_radio/*.cpp>
  +<../examples/companion_radio/ui-new/*.cpp>   ; <- becomes ui-forest/*.cpp
```

For each of the pilot boards in [Section 2](#2-hardware-reality-this-has-to-survive), add a
**new** env alongside its existing `companion_radio` env(s), don't edit the existing ones:

```
[env:RAK_4631_companion_radio_forest_ble]
extends = env:RAK_4631_companion_radio_ble
build_flags = ${env:RAK_4631_companion_radio_ble.build_flags}
  ; swap -I examples/companion_radio/ui-new for ui-forest
build_src_filter = ${rak4631.build_src_filter}
  +<../examples/companion_radio/*.cpp>
  +<../examples/companion_radio/ui-forest/*.cpp>
```

(Exact `extends`/override syntax to be confirmed against how this repo's `platformio.ini`
already layers env sections — follow whatever pattern the transport variants of a single
board already use for each other, e.g. how `_ble`/`_usb`/`_wifi` envs for the same board
relate today.) This keeps every existing env byte-for-byte unchanged; `ui-forest` only
exists in newly-added envs until a future, separate decision promotes it to default.

---

## 6. Phased roadmap

Each phase lists: goal, what gets built, data dependencies, pilot board(s), and a
done-when check. There's no automated UI test harness (the native `pio test -e native` suite
only covers `src/Utils.cpp`/`Packet.cpp` logic, not display/input code), so "done" means a
manual smoke pass using the checklist template in [Section 9](#9-manual-test-checklist-template)
on real hardware for each pilot board in that phase.

### Phase 0 — Framework skeleton
**Goal:** boots, shows splash → an empty home, navigates nothing yet, on pilot boards 1–3.
**Build:** `UITask`/`AbstractUITask` wiring, `NavStack`, `MenuScreen` (with zero real items),
`ConfirmScreen`, `ToastOverlay`, `StatusBar` (static content only), `Layout`, `InputRouter`
covering single-button + joystick + rotary mappings.
**Pilot:** `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`.
**Done when:** all three boot, splash dismisses into a home screen, and a placeholder 3-item
menu can be navigated (up/down/select/back) using each board's native controls.

### Phase 1 — Parity with ui-new (+ ui-tiny's best bits)
**Goal:** `ui-forest` is a strict superset of today's `ui-new` functionality on the pilot
boards. Covers baseline items 1–13 from the original feature list.
**Build:** `Screen_Splash`, `Screen_Status`, `Screen_Recents`, `Screen_RadioInfo`,
`Screen_Bluetooth`, `Screen_Advert`, `Screen_Gps`, `Screen_Sensors`, `Screen_Shutdown`,
`Screen_MsgPreview`; buzzer/vibration/LED notify; auto-off + low-battery shutdown; boot-time
CLI rescue; UTF-8 fallback (reuse `DisplayDriver` helpers as-is, no changes needed there);
fold in `ui-tiny`'s persistent `StatusBar` and torch double-click.
**Data:** all already-public accessors from row 1–3 and row 5 of the table in
[3.4](#34-data-integration--verified-call-sites); no new API yet.
**Pilot:** boards 1–3.
**Done when:** side-by-side with the same board's existing `ui-new` env, every screen shows
the same information (organized as menu entries off Home rather than swipe-pages — that's
the intentional structural change from item 14, not a regression).

### Phase 2 — Navigation & data browsing
**Goal:** items 14–18. Home becomes a real top-level `MenuScreen`; add Contacts and Channels.
**Build:** `Screen_Home` converts from status-page to menu; `Screen_Contacts` +
`Screen_ContactDetail`; `Screen_Channels`; wire `KEY_HOME` (jump-to-root) into `NavStack`.
**Data:** `getNumContacts()`/`getContactByIdx()`/`getChannel()` — already public, see 3.4.
**Pilot:** boards 1–3.
**Done when:** can reach every Phase-1 screen through the menu, browse all seeded
contacts/channels, and jump home from three levels deep with one gesture.

### Phase 3 — Settings
**Goal:** items 19–21. On-device settings wired to real `NodePrefs` fields.
**Build:** `FormField` (Toggle/Stepper/Enum/Text editors), `Screen_Settings` root +
`Screen_SettingsRadio`/`SettingsAdvert`/`SettingsNetwork`/`SettingsDevice`/`SettingsDanger`.
**Data:** direct `_node_prefs->field = value; the_mesh.savePrefs();` pattern, **plus the
`radio_driver.setParams()`/`setTxPower()` live-apply calls flagged in 3.4 — don't skip
these.** Confirm advert-interval changes take effect without an explicit reschedule call
(see Section 8) before assuming the simple write-and-save pattern is sufficient there too.
**Pilot:** boards 1–3. Specifically re-test on `RAK_4631` after any radio-param change to
confirm the radio actually re-tunes live, not just on next boot.
**Done when:** every setting in the catalog can be changed on-device and survives a reboot;
every dangerous action (erase/rekey/reboot/restore-defaults) requires the `ConfirmScreen`
gate first.

### Phase 4 — Diagnostics ("nerd stats")
**Goal:** items 22–26.
**Build:** `Screen_Diagnostics` root + `Screen_DiagRadio`/`DiagPackets`/`DiagCore`,
transport indicator (fold into `StatusBar` + a row in `Screen_DiagCore`), `Screen_EventLog`
(in-RAM ring buffer, rescoped per 3.4 — not a file log).
**Data:** add the one new `MyMesh::getQueueLen()` passthrough (3.4); everything else already
public. Verify the transport-type story (compile-time constant vs. runtime query) per the
open item in 3.4's table.
**Pilot:** boards 1–3.
**Done when:** RSSI/SNR/noise floor/packet counters/queue length/uptime all show real,
changing values that match what the companion app's stats view reports for the same device.

### Phase 5 — Visual polish
**Goal:** items 27–31 (31 is really "keep applying the Phase-0 `Layout` work consistently" —
this phase is the polish pass, not new foundational work).
**Build:** expanded icon set (3.5), separators/card-style grouping on multi-stat screens,
screen-transition animation gated behind `!display.isEink()`, one applied color convention
(green=good/connected, yellow=info, red=warning) swept across every screen built so far.
**Pilot:** boards 1–3, plus first e-ink pilot board (`heltec_e213` or `lilygo_techo`) to
verify the animation-off / no-marquee-status-bar gating actually holds on a real slow panel.
**Done when:** a visual pass across every existing screen shows consistent header style,
color meaning, and separators; e-ink pilot shows zero flicker/animation artifacts.

### Phase 6 — Input/control expansion
**Goal:** items 32–34. The three boards deferred since Section 2.
**Build:** dynamic on-screen control hints (replace `ui-new`'s hardcoded `PRESS_LABEL`
macro with something `InputRouter` can describe per-board); touch tap/swipe mapped to
KEY_ENTER/NEXT/PREV in `InputRouter` for `sensecap_indicator-espnow`; keyboard character
input feeding `FormField`'s Text editor + trackball directional movement mapped to
LEFT/RIGHT/UP/DOWN for `lilygo_tdeck`.
**Pilot:** `lilygo_tdeck`, `sensecap_indicator-espnow` (first real test on these two boards
— nothing before this phase requires them).
**Done when:** T-Deck can type a node name using its real keyboard instead of an
increment-based character picker; SenseCAP can tap a menu item directly instead of relying
on its single fallback button.

### Phase 7 — Notifications
**Goal:** items 35–36.
**Build:** `Screen_RecentEvents` (user-facing history — last advert sent, last message,
last contact seen; distinct from Phase 4's `Screen_EventLog`, see
[Section 7](#7-feature--phase--class-traceability) for the difference), and
`Screen_NotificationSettings` (per-event-type buzzer tune + vibration + LED config, replacing
the single global mute toggle — keep the global mute as a quick top-level action too, don't
remove it).
**Pilot:** boards 1–3.
**Done when:** triggering each event type (message, channel message, ack, advert) shows up
in Recent Events and respects its own notification config independent of the others.

### Phase 8 — Hardening & rollout
**Goal:** widen board coverage beyond the 5 pilots; decide on default-UI migration.
**Build:** add `_forest` envs to a broader board sample (at minimum one more per display
backend — e.g. a GxEPD e-ink board, an LGFX RGB-panel board); fix whatever board-specific
breakage shows up; write the actual migration/deprecation decision for `ui-new` as a
follow-up, not as part of this plan.
**Done when:** builds green across the widened board sample; a short doc note (where and
whether `ui-forest` becomes a board's default) is written as its own follow-up task.

---

## 7. Feature → phase → class traceability

Every item from the original brainstorm, mapped so nothing gets silently dropped.

| # | Feature | Phase | Primary class(es) |
|---|---|---|---|
| 1 | Boot splash | 1 | `Screen_Splash` |
| 2 | Home paginated content (status/recent/radio/bt/advert/gps/sensors/shutdown) | 1 (content) / 2 (restructured as menu) | `Screen_Status`, `Screen_Recents`, `Screen_RadioInfo`, `Screen_Bluetooth`, `Screen_Advert`, `Screen_Gps`, `Screen_Sensors`, `Screen_Shutdown` |
| 3 | Battery icon + muted overlay | 1 | `StatusBar` |
| 4 | Message preview screen | 1 | `Screen_MsgPreview` |
| 5 | Alert popup | 1 | `ToastOverlay` |
| 6 | Buzzer tones + vibration | 1 | `UITask` (ported as-is) |
| 7 | Status LED heartbeat | 1 | `UITask` (ported as-is) |
| 8 | Auto-off + low-battery shutdown | 1 | `UITask` (ported as-is) |
| 9 | Boot-time CLI rescue | 1 | `InputRouter` / `UITask` |
| 10 | UTF-8 fallback + text helpers | 1 | reused from `DisplayDriver`, unchanged |
| 11 | Persistent scrolling status bar | 1 | `StatusBar` (generalized from `ui-tiny`) |
| 12 | Torch double-click | 1 | `InputRouter` (board-gated) |
| 13 | Perf habits (cached reads, redraw-on-change) | 1 | baked into `StatusBar`/`Layout`, not separate |
| 14 | Real menu system + nav stack | 0 (framework) / 2 (applied to Home) | `NavStack`, `MenuScreen` |
| 15 | Consistent header/title per screen | 3.3 (foundational, Phase 5 polish pass) | `Layout` |
| 16 | Jump-to-home gesture | 2 | `NavStack`, `InputRouter` |
| 17 | Contacts list + detail | 2 | `Screen_Contacts`, `Screen_ContactDetail` |
| 18 | Channels list | 2 | `Screen_Channels` |
| 19 | Settings menu wired to prefs | 3 | `Screen_Settings*`, `FormField` |
| 20 | Confirm-guarded dangerous actions | 3 | `Screen_SettingsDanger`, `ConfirmScreen` |
| 21 | Restore defaults per section | 3 | `Screen_Settings*` |
| 22 | Radio stats screen | 4 | `Screen_DiagRadio` |
| 23 | Packet stats screen | 4 | `Screen_DiagPackets` |
| 24 | Core/system stats screen | 4 | `Screen_DiagCore` |
| 25 | Transport indicator | 4 | `StatusBar`, `Screen_DiagCore` |
| 26 | Log viewer (rescoped, see 3.4) | 4 | `Screen_EventLog` |
| 27 | Expanded icon set | 5 | `icons.h` |
| 28 | Separators / card grouping | 5 | `Layout` + per-screen |
| 29 | Screen transitions (non-eink) | 5 | `NavStack` (transition hook) |
| 30 | Consistent color convention | 5 | swept across all screens |
| 31 | Adaptive layout (OLED→TFT→eink) | 0 (foundational) / 5 (polish) | `Layout` |
| 32 | Dynamic on-screen control hints | 6 | `InputRouter`, `Layout` |
| 33 | SenseCAP touch wiring | 6 | `InputRouter` |
| 34 | T-Deck keyboard/trackball wiring | 6 | `InputRouter`, `FormField` (Text editor) |
| 35 | Recent-events strip | 7 | `Screen_RecentEvents` |
| 36 | Per-event notification config | 7 | `Screen_NotificationSettings` |

**Note on 26 vs. 35:** `Screen_EventLog` (Phase 4) is a debug/diagnostic feed — mesh/radio
internals (advert TX/RX, contact discovery, radio param changes) for the "nerd stats"
audience. `Screen_RecentEvents` (Phase 7) is a user-facing notification history — the
subset of events that also buzz/vibrate. They can share the same underlying ring-buffer
class if that turns out convenient, but keep them as separate *screens* — different
audiences, different content.

---

## 8. Open questions / verify during build

Things that couldn't be verified from static reading and need a real build/device to answer
— resolve these when the relevant phase reaches them, don't block earlier phases on them:

- **Advert-interval reschedule:** does changing `advert_interval`/`flood_advert_interval` in
  `NodePrefs` on a live device take effect on its own next timer tick, or does
  `companion_radio` need an explicit reschedule call the way `CommonCLI`'s
  `updateAdvertTimer()`/`updateFloodAdvertTimer()` callbacks do for the *other* example apps?
  Check `Dispatcher`/`Mesh`'s advert-scheduling code path before assuming Phase 3's simple
  write-and-save pattern is sufficient for these two fields specifically.
- **Advert-name validation:** exact character/length rule to mirror from
  `MyMesh::handleCmdFrame`'s `CMD_SET_ADVERT_NAME` handler (`MyMesh.cpp:1206`) — read the
  full handler body (only the line range was confirmed during planning, not the validation
  logic itself).
- **Direct-path packet counters:** confirm `getNumSentDirect()`/`getNumRecvDirect()` (or
  equivalent) exist alongside the confirmed `getNumSentFlood()`/`getNumRecvFlood()` on
  `Dispatcher`, for a complete Phase 4 packet-stats screen.
- **Transport-type story:** confirm whether "which transport is active" is knowable only as
  a compile-time constant (since the transport is chosen at build time in `main.cpp`) or
  whether there's a cleaner runtime signal — affects how `Screen_DiagCore`/`StatusBar`
  surface it.
- **`build_src_filter` recursion:** Section 4 sidesteps this by going flat, but if a future
  session still wants subfolders, confirm PlatformIO's directory-form `+<dir>` src_filter
  entry recurses into subdirectories before relying on it — the root `platformio.ini`'s
  existing pattern (one explicit `+<>` line per subfolder for `helpers/`, `helpers/radiolib/`,
  etc.) suggests it does **not**, but this wasn't tested directly.
- **Env `extends` syntax for the new `_forest` envs:** Section 5's proposed env block syntax
  should be checked against how this repo's existing per-transport envs for the same board
  (e.g. `_ble`/`_usb`/`_wifi` variants of one board) already relate to each other, and matched
  exactly rather than introduced fresh.
- **Icon authoring pipeline:** no XBM conversion script/convention was found in the repo
  during this planning pass. Confirm there really isn't one (check `docs/`, `tools/`, or
  ask upstream) before spending time writing one from scratch for Phase 5.

---

## 9. Manual test checklist template

Copy this per phase/pilot-board combination (no automated UI test harness exists — see
Phase intros above):

```
Board: __________  Env: __________  Phase: __________

[ ] Builds clean: pio run -e <env>
[ ] Boots to splash, dismisses to home within expected time
[ ] Every control gesture for this board's scheme (see Section 2) navigates as expected
[ ] Back/cancel from N levels deep returns one level; KEY_HOME (once Phase 2+) returns to root
[ ] Values shown match a second source of truth (companion app stats view, or CLI `get`
    equivalent on a board where the other example apps' CommonCLI is comparable)
[ ] Auto-off / low-battery / shutdown-confirm still behave correctly (regression check,
    every phase)
[ ] No visual artifacts on this board's display type (tearing, flicker on e-ink, clipped
    text on the smallest screen in the pilot set)
[ ] Settings changes (Phase 3+) survive a reboot
[ ] Dangerous actions (Phase 3+) cannot be triggered without the confirm gate
```
