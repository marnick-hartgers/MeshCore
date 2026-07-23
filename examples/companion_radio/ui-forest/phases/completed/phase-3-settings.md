# Phase 3 — Settings

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 2
> (`phase-2-navigation-and-data-browsing.md`) passed before starting. This phase is the one
> PLAN.md explicitly flags as having the easiest correctness bug to introduce
> (radio-param settings that write prefs but don't live-apply) — read §3.4's radio-params row
> and the note below carefully before writing `Screen_SettingsRadio`.

## Goal

Items 19–21 from PLAN.md §7. A real on-device Settings area wired to `NodePrefs` fields, with
dangerous actions gated behind `ConfirmScreen`.

## Prerequisites (from Phase 2)

`Screen_Home` is a `MenuScreen` with a Settings `Submenu` placeholder entry already present
(Phase 2 built the slot; this phase fills it in). `ConfirmScreen` exists from Phase 0.

## Pilot boards

Same three: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`. **`RAK_4631` gets extra
attention this phase**: re-test on it specifically after any radio-param change to confirm the
radio actually re-tunes live, not just on next boot (PLAN.md §6 Phase 3).

## What to build

1. **`FormField.h/.cpp`** — field editor widgets pushed by `MenuScreen` when a `MenuItem`'s
   kind is `Toggle`/`Stepper`/`Enum`/`Text` (PLAN.md §3.2). Four editor behaviors:
   - `Toggle` — on/off, ENTER flips it immediately.
   - `Stepper` — numeric with min/max/step, LEFT/RIGHT or UP/DOWN adjusts.
   - `Enum` — cycles a fixed string list.
   - `Text` — character-by-character editor (increment-based picker is fine for Phase 3;
     real keyboard input for `lilygo_tdeck` is Phase 6, don't build that here).
2. **`Screen_Settings.h/.cpp`** — root settings menu, `Submenu` entries for Radio/Advert/
   Network/Device/Danger below.
3. **`Screen_SettingsRadio.h/.cpp`** — freq/bw/sf/cr, TX power. **Critical correctness
   requirement**: writing `_node_prefs->freq/bw/sf/cr` (or `tx_power_dbm`) and calling
   `the_mesh.savePrefs()` is *not* sufficient by itself — must **also** call
   `radio_driver.setParams(freq,bw,sf,cr)` / `radio_driver.setTxPower(power)` to apply live.
   This mirrors `MyMesh::handleCmdFrame`'s `CMD_SET_RADIO_PARAMS`/`CMD_SET_RADIO_TX_POWER`
   handlers (`MyMesh.cpp:1373-1418` — read this handler body before writing the settings
   screen, don't reimplement from the prefs struct alone). A settings screen that only writes
   prefs and skips the `radio_driver` call will silently not apply the change until reboot —
   this is exactly the bug PLAN.md calls out as easiest to introduce here; specifically
   regression-test this on `RAK_4631` per the pilot-board note above.
4. **`Screen_SettingsAdvert.h/.cpp`** — advert name + advert/flood-advert interval.
   - Advert name: mirror the validation `MyMesh::handleCmdFrame`'s `CMD_SET_ADVERT_NAME`
     handler does (`MyMesh.cpp:1206`) — **read the full handler body first**, PLAN.md flags
     that only the line number was confirmed during planning, not the exact validation rule
     (length limit? character allowlist? both?). Don't guess the rule; copy it.
   - Advert/flood-advert interval: **verify before assuming write-and-save is enough** —
     PLAN.md §8 flags this as an open question. Check whether changing
     `advert_interval`/`flood_advert_interval` in `NodePrefs` takes effect on the live
     device's own next timer tick, or whether `companion_radio` needs an explicit reschedule
     call (the way `CommonCLI`'s `updateAdvertTimer()`/`updateFloodAdvertTimer()` callbacks do
     for the *other* example apps — `companion_radio` doesn't use `CommonCLI`, so check
     `Dispatcher`/`Mesh`'s advert-scheduling code path directly for what triggers a
     reschedule). If a reschedule call exists and companion_radio isn't already invoking it
     somewhere, this screen needs to call it after writing the interval.
5. **`Screen_SettingsNetwork.h/.cpp`** — repeat, rx-boost, telemetry mode, autoadd policy,
   duty cycle. Toggle/enum fields use the exact `_node_prefs->field = value;
   the_mesh.savePrefs();` pattern `ui-new` already uses for GPS/buzzer toggles
   (`ui-new/UITask.cpp:896-946`, `toggleGPS()`/`toggleBuzzer()`) — just extended to these
   fields. No live-apply concern here the way radio params have one (verify per-field, but
   these are expected to be plain prefs writes).
6. **`Screen_SettingsDevice.h/.cpp`** — device name, buzzer, vibration, notifications entry
   point (the notifications entry point itself is a stub linking toward Phase 7's
   `Screen_NotificationSettings` — don't build that screen's content yet, just leave the menu
   slot).
7. **`Screen_SettingsDanger.h/.cpp`** — erase, rekey, reboot, restore-defaults. **Every one of
   these must go through `ConfirmScreen` first** — no direct-invoke path from the menu.
8. **Restore-defaults per section** (item 21) — each `Screen_Settings*` sub-menu gets a
   restore-defaults action scoped to its own fields, gated the same way as Danger actions.

## Data / API dependencies

From PLAN.md §3.4:
- Radio params: `_node_prefs->freq/bw/sf/cr`/`tx_power_dbm` write + `the_mesh.savePrefs()` +
  `radio_driver.setParams()`/`setTxPower()` — see the critical note above.
- Toggle/enum settings: `_node_prefs->field = value; the_mesh.savePrefs();` — exact pattern
  already in `ui-new`.
- Advert name: validation to be read from `MyMesh.cpp:1206`, not assumed.

## Open questions to resolve in this phase (from PLAN.md §8)

- **Advert-interval reschedule** — resolve before shipping `Screen_SettingsAdvert`'s interval
  fields; see step 4 above.
- **Advert-name validation** — resolve before shipping the name field; see step 4 above.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble             Phase: 3
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble  Phase: 3
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble          Phase: 3

[ ] Builds clean: pio run -e <env>
[ ] Every setting in the catalog (radio, advert, network, device) can be changed on-device
[ ] Radio param changes apply live (verify via a second radio/companion app seeing the new
    params take effect immediately, not just after reboot) — RAK_4631 specifically
[ ] Every changed setting survives a reboot
[ ] Advert name change enforces the same validation as CMD_SET_ADVERT_NAME
[ ] Advert/flood-advert interval changes take effect (confirm reschedule behavior resolved,
    not assumed)
[ ] Every dangerous action (erase/rekey/reboot/restore-defaults) requires the ConfirmScreen
    gate first — cannot be triggered by a single accidental keypress
[ ] Restore-defaults works per settings section independently
[ ] Auto-off / low-battery / shutdown-confirm still behave correctly (regression check)
[ ] No visual artifacts on this board's display type
```
