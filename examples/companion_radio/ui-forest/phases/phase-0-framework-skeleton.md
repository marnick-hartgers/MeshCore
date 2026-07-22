# Phase 0 — Framework skeleton

> Part of the `ui-forest` build plan. Read `../PLAN.md` in full before starting — it has the
> goals, hardware matrix, architecture rationale, file layout, and rollout mechanism this
> document assumes. This file is the actionable spec for Phase 0 only: enough for an agent
> with no other memory of the planning conversation to execute it end to end.

## Goal

Boots, shows a splash screen, transitions into an empty home screen, and can navigate a
placeholder 3-item menu using each pilot board's native controls. No real feature screens yet
— this phase is pure framework (nav, input, layout, overlays), proven on three boards that
each exercise a different input scheme.

Traceability: this phase builds item 14 (nav stack, partially — applied fully to Home in
Phase 2) and item 31 (adaptive layout, foundational half) from PLAN.md §7.

## Prerequisites

None — this is the first phase. The directory `examples/companion_radio/ui-forest/` exists
today with only `PLAN.md` in it.

## Pilot boards for this phase

| Board | Input scheme | Display | Existing envs to mirror (variants/<dir>/platformio.ini) |
|---|---|---|---|
| `RAK_4631` | single momentary button (`PIN_USER_BTN=9`, also `PIN_USER_BTN_ANA=31`) | SSD1306 OLED | `env:RAK_4631_companion_radio_ble` (variants/rak4631/platformio.ini:206) |
| `gat562_30s_mesh_kit` | joystick + back button | SSD1306 OLED | `env:GAT562_30S_Mesh_Kit_companion_radio_ble` |
| `heltec_rc32` | rotary encoder + button | small color TFT (NV3001B) | `env:heltec_rc32_companion_radio_ble` (with-display variant, not the `_without_display` envs) |

## What to build

All new files live flat in `examples/companion_radio/ui-forest/` (no subfolders — see
PLAN.md §4 and the open question in §8 about `build_src_filter` recursion; don't relitigate
that here, just follow it).

1. **`UITask.h` / `UITask.cpp`** — entry point, same public contract `ui-new`/`ui-tiny`
   already expose to `main.cpp` (`begin()`, `loop()`). Port the skeleton only: construction of
   the screen instances, the render loop, and wiring to `InputRouter`/`NavStack`. Leave
   buzzer/vibration/LED/auto-off/CLI-rescue porting to Phase 1 (PLAN.md §6 Phase 1) — don't
   pull those in yet even though `ui-new/UITask.cpp` has them inline; Phase 0 is nav +
   input + layout only.
2. **`InputRouter.h` / `InputRouter.cpp`** — one class that centralizes the per-board
   `#ifdef` gesture-to-key mapping currently duplicated inline in `ui-new`'s
   `UITask::loop()` (`examples/companion_radio/ui-new/UITask.cpp:713-785` is the reference —
   read it first, copy the gesture semantics, don't redesign them). Cover in this phase:
   - `#if defined(PIN_USER_BTN) || defined(PIN_USER_BTN_ANA)` single-button path using
     `MomentaryButton` (click/double/triple/long → NEXT/PREV/SELECT/ENTER, per PLAN.md §3.1).
   - `#if defined(UI_HAS_JOYSTICK)` path (`joystick_left`/`joystick_right`/`back_btn` →
     native LEFT/RIGHT/ENTER/CANCEL, not forced through single-button vocabulary).
   - `#if defined(UI_HAS_ROTARY_INPUT)` path (`RotaryInput` interface + `user_btn`).
   - Output is a single `char` key code (the existing `KEY_LEFT/UP/DOWN/RIGHT/SELECT/ENTER/
     CANCEL/HOME/NEXT/PREV/CONTEXT_MENU` vocabulary from `src/helpers/ui/UIScreen.h` — reuse
     verbatim, do not invent new codes).
   - `InputRouter::poll()` returns the key; `UITask::loop()` is the *only* call site that
     feeds it to `NavStack::current()->handleInput(c)`.
   - `KEY_HOME` wiring (long-press-anywhere / back-button triple-click → pop-to-root) is
     listed under Phase 2 in PLAN.md §3.1, not this phase — stub the key constant handling in
     `NavStack` now if convenient, but don't build a screen that needs it yet.
3. **`NavStack.h` / `NavStack.cpp`** — fixed-depth (8 entries) stack of `UIScreen*`:
   `push(UIScreen*)`, `pop()`, `popToRoot()`, `current()`. No allocation — screens are
   constructed once in `UITask::begin()` and the stack only moves pointers (PLAN.md §3.2,
   §3.6). Verify `UIScreen` (`src/helpers/ui/UIScreen.h`) has the virtual surface this needs
   (`render(DisplayDriver&)`, `handleInput(char)`) before assuming the interface — it should,
   since `ui-new` already builds screens against it, but confirm the exact method signatures
   before writing `NavStack`/`MenuScreen` against a guessed signature.
4. **`MenuScreen.h` / `MenuScreen.cpp`** — generic list-menu widget: takes a `MenuItem` array
   (label, optional icon, kind, payload) + count (PLAN.md §3.2). For Phase 0, only the
   `Action` kind needs to work (enough to prove NEXT/PREV moves a selection cursor, ENTER
   invokes a callback, CANCEL pops). `Submenu`/`Toggle`/`Stepper`/`Enum`/`Text`/`Info` kinds
   can exist in the enum now but their handling can be stubbed — those land in Phase 2/3.
   Build the placeholder 3-item menu (e.g. "Item A"/"Item B"/"Item C", each just toasting its
   own label via `ToastOverlay`) as the thing pushed onto `NavStack` after splash, to prove
   the framework works end to end.
5. **`ConfirmScreen.h` / `ConfirmScreen.cpp`** — yes/no or hold-N-seconds-to-confirm screen.
   Reference the existing hold-to-confirm gesture in `ui-new`'s shutdown flow
   (`examples/companion_radio/ui-new/UITask.cpp:184-188`, the `_shutdown_init`/
   `isButtonPressed()` pattern) as the model. Not exercised by anything yet in Phase 0 — build
   it now because `NavStack`/input plumbing is being proven anyway, but it has no caller until
   Phase 1's shutdown screen or Phase 3's danger-zone settings.
6. **`ToastOverlay.h` / `ToastOverlay.cpp`** — generalizes `ui-new`'s alert-box-over-current-
   screen logic (the `_alert`/`_alert_expiry` block in `UITask::loop()`) into `show(text,
   millis)` + `composite(DisplayDriver&)`, called from `UITask::loop()` right after
   `NavStack::current()->render()`. Use this to prove the placeholder menu's `Action` items
   do something visible.
7. **`StatusBar.h` / `StatusBar.cpp`** — generalizes `ui-tiny`'s `ScrollingStatusBar`
   (`examples/companion_radio/ui-tiny/ScrollingStatusBar.h`) into an optional always-rendered
   top strip. **Static content only in this phase** (e.g. hardcode a placeholder string) —
   real watched fields (battery, BLE, transport) are Phase 1/4. Keep the change-detection and
   marquee-scroll-if-too-wide behavior intact since it's needed later; just don't wire real
   data yet.
8. **`Layout.h`** — `rowHeight()` and `visibleRows(DisplayDriver&)` computed from
   `display.height()`, current font size, and fixed padding (PLAN.md §3.3). `MenuScreen` must
   use this for its visible-row count and scroll-into-view, not a literal constant — this is
   the mechanism that lets the *same* code show 4 rows on a 64px OLED and 10+ on `heltec_rc32`'s
   color TFT with zero per-board tuning. Prove this specifically: the placeholder menu should
   render correctly on both `RAK_4631` (small mono OLED) and `heltec_rc32` (small color TFT)
   without any board-specific menu code.
9. **`Screen_Splash.h` / `Screen_Splash.cpp`** — minimal splash screen, dismisses (on timer or
   any key) into the placeholder home menu. Port visuals from `ui-new`'s splash if one exists;
   otherwise a simple centered logo/text is fine for this phase — Phase 1 is where splash gets
   full parity treatment.

## Data / API dependencies

None. This phase touches zero mesh/radio data — it's pure UI framework proving nav + input +
layout, wired to a hardcoded placeholder menu.

## Rollout — new PlatformIO envs for this phase

Add **new** envs, don't touch the existing `ui-new`/`ui-orig`/`ui-tiny` envs for these boards.
PLAN.md §5's proposed `extends = env:<existing-env>` syntax does not match this repo's actual
pattern — verified by reading `variants/rak4631/platformio.ini:206-229`: existing per-transport
envs extend the **board's base section** (e.g. `extends = rak4631`, not `extends =
env:RAK_4631_companion_radio_usb`), and each env restates its own `build_flags`/
`build_src_filter` starting from `${rak4631.build_flags}` / `${rak4631.build_src_filter}`.
Follow that real pattern, not §5's guess:

```ini
[env:RAK_4631_companion_radio_forest_ble]
extends = rak4631
board_build.ldscript = boards/nrf52840_s140_v6_extrafs.ld
board_upload.maximum_size = 712704
build_flags =
  ${rak4631.build_flags}
  -I examples/companion_radio/ui-forest      ; only line that differs from _ble's ui-new include
  -D PIN_USER_BTN=9
  -D PIN_USER_BTN_ANA=31
  -D DISPLAY_CLASS=SSD1306Display
  -D MAX_CONTACTS=350
  -D MAX_GROUP_CHANNELS=40
  -D BLE_PIN_CODE=123456
  -D BLE_DEBUG_LOGGING=1
  -D OFFLINE_QUEUE_SIZE=256
build_src_filter = ${rak4631.build_src_filter}
  +<helpers/nrf52/SerialBLEInterface.cpp>
  +<../examples/companion_radio/*.cpp>
  +<../examples/companion_radio/ui-forest/*.cpp>   ; only line that differs
lib_deps =
  ${rak4631.lib_deps}
  densaugeo/base64 @ ~1.4.0
  https://github.com/RAKWireless/RAK13800-W5100S/archive/1.0.2.zip
```

Copy this exactly (base section extends + full flag/filter restatement, changing only the two
lines noted) for:
- `env:RAK_4631_companion_radio_forest_ble` from `variants/rak4631/platformio.ini:206`
- `env:GAT562_30S_Mesh_Kit_companion_radio_forest_ble` from
  `variants/gat562_30s_mesh_kit/platformio.ini:90`
- `env:heltec_rc32_companion_radio_forest_ble` from
  `variants/heltec_rc32/platformio.ini:285` (the **with-display** section, not the
  `_without_display` one at line 145)

Only add one transport variant (BLE) per board for this phase — USB/wifi/ethernet forest envs
aren't needed until later phases exercise those boards' transports specifically (Phase 4's
transport indicator work is the first place transport variety matters).

## Open questions to resolve in this phase

From PLAN.md §8:
- **`build_src_filter` recursion**: not exercised differently by Phase 0 (flat layout sidesteps
  it) but if a subfolder is ever tempted, confirm PlatformIO's `+<dir>` glob does or doesn't
  recurse before relying on it. Not expected to come up if the flat layout is followed.
- **Env `extends` syntax**: resolved above by reading the real `rak4631` env block — use the
  base-section-extends pattern, not `extends = env:...`.

## Done-when / manual test checklist

Run per pilot board (template from PLAN.md §9, filled in for this phase):

```
Board: RAK_4631                  Env: RAK_4631_companion_radio_forest_ble                Phase: 0
Board: gat562_30s_mesh_kit       Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble     Phase: 0
Board: heltec_rc32               Env: heltec_rc32_companion_radio_forest_ble             Phase: 0

[ ] Builds clean: pio run -e <env>
[ ] Boots to splash, dismisses to home (placeholder 3-item menu) within expected time
[ ] Every control gesture for this board's scheme navigates the placeholder menu as expected
    (NEXT/PREV moves cursor, ENTER toasts the selected label, CANCEL is a no-op at root)
[ ] Menu renders correctly with Layout-computed row height/visible-rows on both the OLED
    (RAK_4631/gat562) and color TFT (heltec_rc32) boards — no hardcoded pixel offsets
[ ] No visual artifacts on any of the three displays
```

Do not proceed to Phase 1 until all three pilot boards pass this checklist — every later
phase is menu entries hung off this skeleton (PLAN.md, opening paragraph).
