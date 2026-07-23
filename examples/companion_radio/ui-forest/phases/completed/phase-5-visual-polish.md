# Phase 5 — Visual polish

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 4
> (`phase-4-diagnostics.md`) passed before starting. This phase is a polish pass over
> everything built in Phases 0–4, plus the first e-ink pilot board — it is not expected to
> add new screens or new data dependencies.

## Goal

Items 27–31 from PLAN.md §7 (31 is really "keep applying the Phase 0 `Layout` work
consistently" — this phase is the sweep, not new foundational work). Consistent header style,
icon set, separators, color convention, and transitions across every screen built so far, plus
proof that e-ink gating (no animation, no marquee) actually holds on real slow hardware.

## Prerequisites (from Phases 0–4)

Every screen listed in Phases 0–4's "What to build" sections exists and passes its own phase's
checklist. This phase touches all of them but adds no new ones (icons.h grows, but that's
assets, not a screen).

## Pilot boards

`RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32` (same three, regression sweep), **plus the
first e-ink pilot board** — pick one of `heltec_e213` (existing envs:
`Heltec_E213_companion_radio_ble`/`_usb`) or `lilygo_techo` (existing envs:
`LilyGo_T-Echo_companion_radio_ble`/`_usb`) per PLAN.md §2/§6. Add a new
`*_companion_radio_forest_ble` env for whichever is chosen, following the same base-section-
extends pattern established in Phase 0 (see `phase-0-framework-skeleton.md`'s rollout section
for the worked example and the correction to PLAN.md §5's proposed syntax).

## What to build

1. **Expanded icon set (`icons.h`)** (item 27) — PLAN.md §3.5: hand-authored monochrome XBM
   byte arrays, same convention as `ui-new/icons.h`/`ui-tiny/u8g2_icons.h` — no runtime image
   decoding (violates the no-heap-after-setup rule for no benefit at these sizes).
   `DisplayDriver::drawXbm()` + `setColor()` already tints monochrome bitmaps for color
   displays, so one bitmap set serves OLED/TFT/e-ink alike. Two fixed sizes: 16×16 (inline
   list-row icons — contacts, channels, menu entries) and 32×32 (full-screen glyphs — advert,
   bluetooth, power, torch, same size `ui-new` already uses). New icons needed beyond today's
   set (bluetooth on/off, power, advert, muted, logo): signal-bars (multi-frame, driven by
   RSSI/SNR thresholds), GPS fix/no-fix, contact, channel, gear/settings, warning, torch,
   log/event. **Before hand-authoring from scratch, check for an existing conversion
   script/convention** (PLAN.md §8 flags that none was found during planning — check `docs/`,
   `tools/`, or ask upstream before spending time writing a converter). Author as 1-bit images,
   convert to XBM.
2. **Separators / card-style grouping** (item 28) — on multi-stat screens (Diagnostics screens
   from Phase 4 are the main beneficiary), add visual grouping via `Layout` helpers, applied
   consistently rather than per-screen ad hoc.
3. **Screen-transition animation** (item 29) — gate behind `!display.isEink()`, same pattern
   `ui-new` already uses for its shutdown delay (`if (_display->isEink() == false) { delay(3000);
   }`). Hook this into `NavStack` (a transition hook on push/pop), not per-screen.
4. **Consistent color convention** (item 30) — green=good/connected, yellow=info, red=warning,
   swept across every screen built in Phases 1–4. Use `DisplayDriver`'s logical colors
   (`DARK/LIGHT/RED/GREEN/BLUE/YELLOW/ORANGE`, which collapse to on/off on monochrome) — don't
   introduce new color constants.
5. **Adaptive layout polish pass** (item 31) — re-verify every screen from Phases 1–4 actually
   uses `Layout::rowHeight()`/`visibleRows()` rather than any pixel offset that crept in during
   earlier phases under time pressure. This is a review pass, not new code, unless a violation
   is found.
6. **E-ink gating verification** — on the chosen e-ink pilot board:
   - No screen-transition animation fires (step 3's gate).
   - `StatusBar`'s marquee-scroll is disabled — show the truncated/static form instead (PLAN.md
     §3.3 explicitly calls this out: "no marquee-scrolling `StatusBar` on e-ink").
   - Redraw cadence respects `display.isEink()` wherever a cadence/animation decision exists.

## Data / API dependencies

None new — this phase is presentation-only over data already wired in Phases 1–4.

## Open questions to resolve in this phase (from PLAN.md §8)

- **Icon authoring pipeline** — confirm there really isn't an existing XBM conversion
  script/convention in the repo before building one from scratch.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble               Phase: 5
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble    Phase: 5
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble            Phase: 5
Board: <heltec_e213|lilygo_techo>  Env: <..._companion_radio_forest_ble>              Phase: 5

[ ] Builds clean: pio run -e <env>, all four boards
[ ] Visual pass across every existing screen shows consistent header style (icon + title),
    consistent separator/card style on multi-stat screens, and consistent color meaning
[ ] Screen transitions animate on the three non-eink pilots, do NOT animate on the e-ink pilot
[ ] StatusBar shows truncated/static text on e-ink (no marquee scroll)
[ ] Zero flicker/animation artifacts on the e-ink pilot across normal navigation
[ ] Regression: nav, input, settings persistence, diagnostics values, auto-off/shutdown all
    still behave correctly on all four boards
```
