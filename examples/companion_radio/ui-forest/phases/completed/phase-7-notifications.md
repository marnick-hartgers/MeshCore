# Phase 7 — Notifications

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 6
> (`phase-6-input-control-expansion.md`) passed before starting.

## Goal

Items 35–36 from PLAN.md §7: a user-facing recent-events history distinct from Phase 4's
debug event log, and per-event-type notification configuration replacing the single global
mute toggle (while keeping the global mute as a quick top-level action too).

## Prerequisites (from Phase 4 and earlier)

`Screen_EventLog`'s ring-buffer class (Phase 4) may be reused as the underlying storage for
this phase's `Screen_RecentEvents`, per PLAN.md §7's note on item 26 vs. 35 — reuse the class
if convenient, but these remain **separate screens** for two different audiences (Phase 4's
is the nerd/debug feed of mesh/radio internals; this phase's is the subset of events that also
buzz/vibrate, i.e. what a non-technical user would want a history of). `Screen_SettingsDevice`
(Phase 3) has a stub "notifications" menu entry pointing here — this phase fills it in.

## Pilot boards

Back to the original three: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`.

## What to build

1. **`Screen_RecentEvents.h/.cpp`** — user-facing history: last advert sent, last message, last
   contact seen (and any other event type that also triggers a buzz/vibrate/LED notification).
   If reusing Phase 4's ring-buffer class, either share one buffer with a "user-facing" filter
   applied at render time, or maintain a second small buffer fed by the same event trigger
   points — pick whichever avoids duplicating the event-capture call sites in `UITask`/
   `Screen_*`; don't add new callback plumbing on `Mesh`/`MyMesh` to feed this (same constraint
   as Phase 4's event log).
2. **`Screen_NotificationSettings.h/.cpp`** — per-event-type configuration: buzzer tune +
   vibration + LED, one row per event type (message, channel message, ack, advert — at
   minimum; extend to whatever event types Phase 1's ported buzzer/vibration/LED code already
   distinguishes). This **replaces** the single global mute toggle as the primary
   configuration surface, but the global mute action must still exist and still work as a
   fast top-level action (e.g. reachable directly from Home or via the existing gesture
   `ui-new` uses for it today) — don't remove it, this phase adds granularity on top, it
   doesn't take away the quick path.
3. Wire `Screen_SettingsDevice`'s notifications stub (built in Phase 3) to push
   `Screen_NotificationSettings`.

## Data / API dependencies

None new. This phase builds entirely on:
- Phase 1's buzzer/vibration/LED trigger points (item 6/7 from PLAN.md §7).
- Phase 4's ring-buffer pattern (reused or paralleled, per above).
- Phase 3's `_node_prefs`-write pattern, extended to whatever new per-event-type config fields
  are needed (these are new fields on `NodePrefs`-adjacent storage, not new mesh/radio API —
  scope them as UI-local state unless there's already a companion-app-facing pref for this;
  check `NodePrefs` for existing per-event notification fields before assuming new ones are
  needed).

## Open questions to resolve in this phase

None carried from PLAN.md §8. One to check while building: whether `NodePrefs` already has any
per-event-type notification fields (in case the companion app already has an equivalent
settings surface) before adding new storage — reuse if it exists, add new fields only if it
doesn't.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble             Phase: 7
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble  Phase: 7
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble          Phase: 7

[ ] Builds clean: pio run -e <env>
[ ] Triggering each event type (message, channel message, ack, advert) shows up in
    Screen_RecentEvents with correct content/ordering
[ ] Each event type respects its own notification config (buzzer/vibration/LED) independently
    of the others — muting one type doesn't mute the others
[ ] Global mute toggle still exists as a quick top-level action and still silences everything
[ ] Screen_EventLog (Phase 4) and Screen_RecentEvents remain distinct screens with distinct,
    correct content — not merged into one feed
[ ] Settings changes from Phase 3, diagnostics values from Phase 4 unaffected (regression)
[ ] No visual artifacts on this board's display type
```
