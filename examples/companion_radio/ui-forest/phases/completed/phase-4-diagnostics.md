# Phase 4 — Diagnostics ("nerd stats")

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 3
> (`phase-3-settings.md`) passed before starting.

## Goal

Items 22–26 from PLAN.md §7. Surface radio/packet/core stats and a transport indicator that
have never been shown on any existing screen, plus an in-RAM diagnostic event log.

## Prerequisites (from Phase 3)

`Screen_Home`'s menu has a Diagnostics `Submenu` placeholder slot (mirror how Settings was
added in Phase 3 — same shape, different content).

## Pilot boards

Same three: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`.

## What to build

1. **One new `MyMesh` passthrough — do this first, everything else in this phase reads from
   it or from already-public accessors:**
   ```cpp
   uint32_t getQueueLen() const { return _mgr->getOutboundTotal(); }
   ```
   `_mgr` is private to `Dispatcher`; `UITask`/screens aren't part of that class hierarchy, so
   they can't reach it directly. This mirrors the existing `getBLEPin()`/`getRecentlyHeard()`
   passthrough pattern already on `MyMesh` — same shape, same file, one line (PLAN.md §3.4,
   "Core/queue stats" row). Add it to wherever those existing passthroughs live in `MyMesh.h`.
2. **`Screen_Diagnostics.h/.cpp`** — root diagnostics menu, `Submenu` entries for Radio/
   Packets/Core/EventLog below.
3. **`Screen_DiagRadio.h/.cpp`** — noise floor / RSSI / SNR via `radio_driver.getNoiseFloor()`,
   `.getLastRSSI()`, `.getLastSNR()` — already used by `ui-new` today, zero new API.
4. **`Screen_DiagPackets.h/.cpp`** — `radio_driver.getPacketsRecv()`/`getPacketsSent()`/
   `getPacketsRecvErrors()`, `the_mesh.getNumSentFlood()`/`getNumRecvFlood()` (public on
   `Dispatcher`, `src/Dispatcher.h:186,188`, inherited by `MyMesh` — zero new API). **Before
   finishing this screen, confirm the direct-path counterparts exist** —
   `getNumSentDirect()`/`getNumRecvDirect()` or equivalent — alongside the confirmed flood
   counters (PLAN.md §8 open question). If they exist, show both flood and direct counts; if
   they genuinely don't exist on `Dispatcher`, that's a small scope decision to flag back
   rather than silently shipping a flood-only packet-stats screen.
5. **`Screen_DiagCore.h/.cpp`** — outbound queue length (via the new `getQueueLen()`), uptime
   (`rtc_clock` or `millis()/1000` — already used the same way by `ui-new`'s `HomeScreen`), and
   a transport-indicator row (see step 6).
6. **Transport indicator** (item 25) — fold into both `StatusBar` (already built in Phase 0/1,
   extend its watched fields) and a row in `Screen_DiagCore`. **Resolve the open question from
   PLAN.md §8 first**: is "which transport is active" knowable only as a compile-time constant
   (transport is picked at build time in `main.cpp` between `SerialBLEInterface`/
   `SerialWifiInterface`/`ArduinoSerialInterface`/`SerialEthernetInterface`), or is there a
   cleaner runtime signal via something like `isSerialEnabled()`/`hasConnection()` on
   `AbstractUITask`? If it resolves to compile-time-only, set the string once in
   `UITask::begin()` (a small `#ifdef`-selected constant), not a per-frame runtime query.
   Note the pilot board `sensecap_indicator-espnow` (Phase 6, not this phase's pilot set) uses
   ESPNOW as its transport per its `platformio.ini` — a useful data point that transport really
   is compile-time-selected, not just BLE-vs-not.
7. **`Screen_EventLog.h/.cpp`** — in-RAM ring buffer viewer (item 26, rescoped per PLAN.md
   §3.4: `companion_radio` has no file-backed log subsystem — `MyMesh::logRxRaw()` is an empty
   override today, and the `log start`/`stop`/`erase` CLI commands belong to `CommonCLI`,
   which `companion_radio` does not use). Build a small ring buffer (e.g. last 20 short event
   strings — advert sent/received, contact discovered, low battery, radio param changed) fed
   by `UITask` itself as those events already become visible to it elsewhere in this codebase
   (i.e. hook into existing code paths in `UITask`/`Screen_*` that already observe these
   events — don't add new callback plumbing on `Mesh`/`MyMesh` to feed this). No new API on
   `MyMesh` needed for this. Keep this ring-buffer class reusable — Phase 7's
   `Screen_RecentEvents` may share the underlying buffer class (PLAN.md §7 note on 26 vs. 35),
   but keep them as **separate screens**: `Screen_EventLog` is the nerd/debug feed (mesh/radio
   internals), `Screen_RecentEvents` (Phase 7) is the user-facing notification history (the
   subset that also buzzes/vibrates) — don't collapse the two audiences into one screen.

## Data / API dependencies

From PLAN.md §3.4:
- Radio stats — already public, zero new API.
- Packet stats — already public (flood counters confirmed; direct counters to verify, step 4).
- Core/queue stats — needs the one new `getQueueLen()` passthrough (step 1).
- Uptime — no new API.
- Transport indicator — resolve compile-time-vs-runtime question (step 6) before implementing.
- Event log — no new API, in-RAM only (step 7).

## Open questions to resolve in this phase (from PLAN.md §8)

- **Direct-path packet counters** — confirm `getNumSentDirect()`/`getNumRecvDirect()` (or
  equivalent) exist on `Dispatcher` before finalizing `Screen_DiagPackets`.
- **Transport-type story** — confirm compile-time-constant vs. runtime signal before finalizing
  `StatusBar`/`Screen_DiagCore`'s transport row.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble             Phase: 4
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble  Phase: 4
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble          Phase: 4

[ ] Builds clean: pio run -e <env>
[ ] RSSI/SNR/noise floor/packet counters/queue length/uptime all show real, changing values
[ ] Values match what the companion app's stats view reports for the same device
    (side-by-side comparison against the phone app, per screen)
[ ] Transport indicator shows the correct transport for this env's build (BLE for these three
    forest_ble envs)
[ ] Event log shows recent events (advert sent/received, contact discovered, etc.) as they
    happen, oldest entries drop once the ring buffer is full
[ ] Auto-off / low-battery / shutdown-confirm still behave correctly (regression check)
[ ] Settings changes from Phase 3 still survive a reboot (regression check)
[ ] No visual artifacts on this board's display type
```
