# Phase 6 — Input/control expansion

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 5
> (`phase-5-visual-polish.md`) passed before starting. This is the first phase to actually
> exercise `lilygo_tdeck` and `sensecap_indicator-espnow` — nothing before this phase requires
> them, per PLAN.md §2/§6.

## Goal

Items 32–34 from PLAN.md §7: dynamic on-screen control hints, capacitive touch wiring for
`sensecap_indicator-espnow`, and keyboard + trackball wiring for `lilygo_tdeck`.

## Prerequisites (from Phase 5)

All screens/framework classes from Phases 0–5 exist and pass their checklists on the original
three pilots. `FormField`'s `Text` editor exists from Phase 3 (currently increment-based
picker) — this phase gives it a second input path (real keyboard) on `lilygo_tdeck`, it
doesn't replace the increment-based picker for boards without a keyboard.

## Pilot boards for this phase

| Board | What's unused today | Existing envs to mirror |
|---|---|---|
| `lilygo_tdeck` | Hardware has a keyboard matrix + trackball; currently wired as a single `PIN_USER_BTN` (trackball click only) — keyboard input and trackball directional movement are unused (PLAN.md §2) | `LilyGo_TDeck_companion_radio_ble`/`_usb` |
| `sensecap_indicator-espnow` | `LGFXDisplay::getTouch(x,y)` exists at driver level but is used by zero example code today (PLAN.md §2); this board's `platformio.ini` also defines `-D HAS_TOUCH` and uses **ESPNOW**, not BLE, as its transport | `SenseCapIndicator-ESPNow_comp_radio_usb` (variants/sensecap_indicator-espnow/platformio.ini:35 — note the env name is `_comp_radio_usb`, not `_companion_radio_usb`; don't assume the longer naming convention here) |

Add new forest envs for both, following the base-section-extends pattern from Phase 0 (extend
`SenseCapIndicator-ESPNow`/the T-Deck's base section, restate flags/filter, swap only the
`-I`/`+<>` lines for `ui-forest`).

## What to build

1. **Dynamic on-screen control hints** (item 32) — replace `ui-new`'s hardcoded `PRESS_LABEL`
   macro with something `InputRouter` can describe per-board (e.g. "a hint string per screen,
   built from the active input scheme's real gesture vocabulary" rather than a compile-time
   fixed string). This is what makes it possible for touch/keyboard boards to show *their own*
   correct hints instead of button-press language that doesn't apply to them.
2. **Touch wiring for `sensecap_indicator-espnow`** (item 33) — in `InputRouter`, add a touch
   path that reads touch coordinates and maps tap → `KEY_ENTER`, swipe (direction inferred
   from touch-start/touch-end delta) → `KEY_NEXT`/`KEY_PREV` (or LEFT/RIGHT, matching whatever
   `MenuScreen` already expects for list navigation). This board currently "just runs `ui-new`
   on a plain button" (PLAN.md §2) — the forest env should let it tap a menu item directly
   instead of relying on the single fallback button.

   **Correction (checked against the repo before starting):** `getTouch(int*, int*)`
   (`src/helpers/ui/LGFXDisplay.h:38`) is declared only on `LGFXDisplay` itself, not on the
   shared `DisplayDriver` base class — and `InputRouter`/`UITask` only ever hold a
   `DisplayDriver*`, so `LGFXDisplay::getTouch(x,y)` isn't callable as the doc originally
   implied without a cast. Two ways to close that gap, pick whichever fits the existing style
   better once you're in the code:
   - Add a virtual `getTouch(int*, int*)` to `DisplayDriver` (default no-op/`return false`),
     matching the exact pattern `isEink()`/`supportsColor()` already use
     (`DisplayDriver.h:17,23`) — `InputRouter` then calls it generically through the
     `DisplayDriver*` it already has, no cast, no new accessor.
   - Or expose a raw display pointer/accessor from `UITask` and `static_cast<LGFXDisplay*>`
     inside a `#ifdef HAS_TOUCH`-gated block at the call site.

   Leaning towards the virtual-method route since it's the established convention for
   optional per-backend capability queries, but it's an implementation call, not a fixed
   requirement.

   Since `getTouch()` exists at the driver level but has zero prior callers, also budget time
   to verify its actual coordinate semantics (screen-space vs. panel-native orientation)
   against this board's `-D UI_ZOOM=3.5` and rotation setup before wiring gesture thresholds.
   Note too that `HAS_TOUCH` (confirmed real, `variants/sensecap_indicator-espnow/
   platformio.ini:24`, alongside `-D UI_ZOOM=3.5` at line 20) isn't referenced anywhere in
   `InputRouter.cpp` or `DisplayDriver` today — there's no existing gating convention to
   follow for it; this phase establishes one fresh.
3. **Keyboard + trackball wiring for `lilygo_tdeck`** (item 34):
   - Keyboard: feed character input into `FormField`'s `Text` editor as a second, richer input
     path — typing a name directly instead of incrementing through characters one at a time.
   - Trackball: map directional movement to `KEY_LEFT/RIGHT/UP/DOWN` in `InputRouter`, in
     addition to (not replacing) the existing single-click-only wiring.
   - This is real, first-time hardware testing for this board's non-click inputs — nothing in
     Phases 0–5 exercised the keyboard matrix or trackball movement.

## Data / API dependencies

None new — this phase is pure input-plumbing over the existing `FormField`/`MenuScreen`/
`NavStack` framework from earlier phases.

## Open questions to resolve in this phase

None carried from PLAN.md §8 specifically, but two things flagged inline above are effectively
open questions to resolve here (not blocking earlier phases, only this one):
- `LGFXDisplay::getTouch(x,y)`'s actual coordinate/rotation semantics on
  `sensecap_indicator-espnow` — unverified because it has zero prior callers.
- Whether `sensecap_indicator-espnow`'s ESPNOW transport needs any special handling in the
  transport-indicator work from Phase 4 (that phase's pilot set didn't include this board) —
  worth a quick regression check now that this board is in scope.

## Done-when / manual test checklist

```
Board: lilygo_tdeck                 Env: LilyGo_TDeck_companion_radio_forest_ble           Phase: 6
Board: sensecap_indicator-espnow    Env: SenseCapIndicator-ESPNow_comp_radio_forest_usb    Phase: 6

[ ] Builds clean: pio run -e <env>
[ ] T-Deck: can type a node name using the real keyboard instead of the increment-based
    character picker (FormField Text editor, keyboard path)
[ ] T-Deck: trackball directional movement navigates menus (up/down/left/right), independent
    of the existing click-to-select behavior
[ ] SenseCAP: can tap a menu item directly to select it, instead of relying on the single
    fallback button
[ ] SenseCAP: swipe gestures move the selection cursor (next/prev) correctly
[ ] Dynamic control hints on both boards reflect their actual input scheme (touch/keyboard
    hints, not generic button-press language)
[ ] Regression: every screen from Phases 1–5 still reachable and functional on both boards
    via their existing single-button/trackball-click fallback (don't regress the baseline
    while adding the new input paths)
```
