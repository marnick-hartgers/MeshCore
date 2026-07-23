# Phase 1 — Parity with ui-new (+ ui-tiny's best bits)

> Part of the `ui-forest` build plan. Read `../PLAN.md` in full first, and confirm Phase 0
> (`phase-0-framework-skeleton.md`) is actually done — this phase hangs real content screens
> off the `NavStack`/`MenuScreen`/`Layout`/`InputRouter`/`ToastOverlay`/`StatusBar` skeleton
> that phase built. Don't start this phase if Phase 0's checklist hasn't passed on all three
> pilot boards.

## Goal

`ui-forest` becomes a strict superset of today's `ui-new` functionality on the pilot boards —
same information, reorganized as menu entries off Home rather than ui-new's swipe-page ring
(that reorganization is intentional, see PLAN.md item 14 — it's not a regression to fix).
Covers baseline items 1–13 from PLAN.md §7.

## Prerequisites (from Phase 0)

- `NavStack`, `MenuScreen` (Action-kind items working), `ConfirmScreen`, `ToastOverlay`,
  `StatusBar` (static), `Layout`, `InputRouter` all exist and pass Phase 0's checklist on
  `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`.
- `Screen_Splash` exists (minimal version) — this phase brings it to full parity.

## Pilot boards

Same three as Phase 0: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`. Use the same
`*_companion_radio_forest_ble` envs Phase 0 created — no new envs needed this phase.

## What to build

Each screen below is a straight port of behavior already in `examples/companion_radio/ui-new/
UITask.cpp` (and, where noted, `ui-tiny`), rebuilt as a `UIScreen` subclass that reads through
`Layout`/`DisplayDriver` instead of hardcoded pixel offsets:

1. **`Screen_Splash.h/.cpp`** — full parity version (item 1): logo, version/build info if
   `ui-new` shows it, dismiss behavior identical to today.
2. **`Screen_Status.h/.cpp`** — today's `HomePage::FIRST` content: message count, connection
   state, BLE pin (item 2, status portion). This becomes a menu entry, not the default page,
   once Phase 2 restructures Home — for now it's fine for `UITask::begin()` to push it directly
   after splash so there's something to look at.
3. **`Screen_Recents.h/.cpp`** — recently-heard list (item 2, recents portion). Read through
   the same on-demand accessor pattern `ui-new` already uses (don't copy all entries into a new
   buffer — PLAN.md §3.6).
4. **`Screen_RadioInfo.h/.cpp`** — radio params + noise floor/RSSI/SNR (item 2, radio portion).
   Data: `radio_driver.getNoiseFloor()`, `.getLastRSSI()`, `.getLastSNR()` — already used by
   `ui-new` today, zero new API (PLAN.md §3.4 table row "Radio stats").
5. **`Screen_Bluetooth.h/.cpp`** — BLE state/pairing info (item 2, bt portion).
6. **`Screen_Advert.h/.cpp`** — advert send/trigger screen (item 2, advert portion).
7. **`Screen_Gps.h/.cpp`** — `#if ENV_INCLUDE_GPS` gated (item 2, gps portion). None of the
   three Phase-1 pilot boards may have GPS — check each board's `platformio.ini` for
   `ENV_INCLUDE_GPS` before assuming this is testable on the pilot set; if none do, verify by
   compiling with the flag forced on, or defer real-hardware testing to whichever later pilot
   board has a GPS module.
8. **`Screen_Sensors.h/.cpp`** — `#if UI_SENSORS_PAGE` gated (item 2, sensors portion). Same
   caveat as GPS — confirm which pilot board (if any) actually defines `UI_SENSORS_PAGE`.
9. **`Screen_Shutdown.h/.cpp`** — uses `ConfirmScreen`'s hold-to-confirm gesture (built in
   Phase 0) for the shutdown flow. Port the exact hold-duration/feedback behavior from
   `examples/companion_radio/ui-new/UITask.cpp:184-188`.
10. **`Screen_MsgPreview.h/.cpp`** — message preview screen (item 4).
11. **Buzzer tones + vibration** (item 6) — port into `UITask` as-is, same trigger points
    `ui-new` uses today.
12. **Status LED heartbeat** (item 7) — port into `UITask` as-is.
13. **Auto-off + low-battery shutdown** (item 8) — port into `UITask` as-is; this is a
    regression-checked item in every phase's manual test from here on (PLAN.md §9).
14. **Boot-time CLI rescue** (item 9) — port into `InputRouter`/`UITask` as-is (the "hold
    button in first 8s → CLI" behavior PLAN.md §3.1 references).
15. **UTF-8 fallback + text helpers** (item 10) — no new code; confirm `Screen_*` classes call
    the existing `DisplayDriver` helpers rather than raw text-draw calls, so this falls out for
    free.
16. **`StatusBar` real content** — wire in battery icon + muted overlay (item 3) and fold in
    `ui-tiny`'s persistent scrolling status bar behavior (item 11) — `StatusBar` already has
    the change-detection/marquee logic from Phase 0, this step is about feeding it real watched
    fields instead of the placeholder string.
17. **Torch double-click** (item 12) — `#if HAS_TORCH` gated in `InputRouter`, back-button
    double-click on boards that have it (per PLAN.md §2, `lilygo_techo_lite` is the reference
    board for this — not one of the three Phase 1 pilots, so this is compile-gated code that
    may not be testable on real hardware until a torch-equipped board is in the pilot set;
    don't block Phase 1 completion on hardware-testing this specific item if none of
    `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` has `HAS_TORCH`).
18. **Perf habits** (item 13) — cached reads, redraw-on-change. Not a separate class; verify
    `StatusBar`/`Layout` already do this (they should, per PLAN.md §3.2's description of
    `StatusBar::update()`) rather than building something new.

## Data / API dependencies

All from PLAN.md §3.4, rows already marked "already public" / "already used by ui-new today" /
"no new API": Radio stats, Uptime (`rtc_clock` or `millis()/1000`). No new `MyMesh`/`Mesh`
methods needed in this phase — everything is already-public accessors ui-new already calls.

## Open questions relevant to this phase

None from PLAN.md §8 block this phase specifically — the GPS/sensors board-gating caveat above
is new (not in §8) but worth tracking the same way: verify against each pilot board's actual
`platformio.ini` flags rather than assuming.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble             Phase: 1
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble  Phase: 1
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble          Phase: 1

[ ] Builds clean: pio run -e <env>
[ ] Boots to splash, dismisses to home within expected time
[ ] Every control gesture for this board's scheme navigates as expected
[ ] Side-by-side with the same board's existing ui-new env: every screen shows the same
    information as ui-new (organized as separate screens reached in sequence for now —
    proper menu restructuring is Phase 2, not required to pass this checklist)
[ ] Battery icon + mute overlay in StatusBar reflect real device state
[ ] Buzzer/vibration/LED heartbeat fire on the same triggers as ui-new
[ ] Auto-off / low-battery / shutdown-confirm behave correctly
[ ] Boot-time CLI rescue (hold button within first 8s) still works
[ ] No visual artifacts on this board's display type
```

Side-by-side comparison against each board's existing `ui-new` env is the actual acceptance
test for this phase (PLAN.md §6 Phase 1 "Done when") — build and flash both envs and compare.
