# Phase 2 — Navigation & data browsing

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 1
> (`phase-1-parity-with-ui-new.md`) passed its checklist on all three pilot boards before
> starting — this phase restructures Home into the real top-level menu that every later
> phase's screens get hung off, and adds the first two real data-driven list screens.

## Goal

Items 14–18 from PLAN.md §7. Home stops being a status page and becomes a real top-level
`MenuScreen`; add Contacts and Channels browsing; wire the home/root-jump gesture.

## Prerequisites (from Phase 1)

All Phase 1 screens (`Screen_Status`, `Screen_Recents`, `Screen_RadioInfo`,
`Screen_Bluetooth`, `Screen_Advert`, `Screen_Gps`, `Screen_Sensors`, `Screen_Shutdown`,
`Screen_MsgPreview`) exist and are reachable in some form. `MenuScreen`'s `Action` kind
already works from Phase 0; this phase is where its `Submenu` kind gets exercised for real.

## Pilot boards

Same three: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`. Same forest envs as Phase 0/1.

## What to build

1. **`Screen_Home` conversion** — Home changes from Phase 1's sequential status page into a
   `MenuScreen` instance whose items are `Submenu`-kind entries pointing at each Phase-1 screen
   (Status, Recents, Radio Info, Bluetooth, Advert, Gps/Sensors if compiled in, Shutdown,
   MsgPreview) plus the not-yet-built Contacts/Channels/Settings/Diagnostics entries (Settings
   and Diagnostics submenus are empty placeholders until Phase 3/4 — don't build their content
   now, just make sure the menu structure has room for them so Phase 3/4 are additive, not a
   Home-menu rewrite).
2. **`Screen_Contacts.h/.cpp`** — list screen driven by `the_mesh.getNumContacts()` +
   `the_mesh.getContactByIdx(idx, ContactInfo&)`. Already public on `BaseChatMesh`
   (`src/helpers/BaseChatMesh.h:176-177`) — zero new API (PLAN.md §3.4). Read contacts
   on-demand by index for the visible window only (per `Layout::visibleRows`) — never copy
   all contacts into a new buffer (PLAN.md §3.6, this is the specific correctness concern
   called out there for this exact screen).
3. **`Screen_ContactDetail.h/.cpp`** — pushed via `nav.push(&contactDetailScreen)` when a
   contact is selected in `Screen_Contacts`; `KEY_CANCEL` pops back. Fields to show, all
   already on `ContactInfo` (`src/helpers/ContactInfo.h`, per PLAN.md §3.4): `name`, `type`
   (`ADV_TYPE_*`), `out_path_len` (`0xFF` = flood), `last_advert_timestamp`, `gps_lat`/
   `gps_lon`, `id.pub_key`.
4. **`Screen_Channels.h/.cpp`** — iterate `the_mesh.getChannel(idx, ChannelDetails&)` over
   `0..MAX_GROUP_CHANNELS-1`, skip empty slots (`name[0]==0`) — same pattern the existing
   recents list already uses (PLAN.md §3.4). Already public — zero new API.
5. **`KEY_HOME` wiring** — the long-press-anywhere (or back-button triple-click on joystick
   boards) gesture from Phase 0/1's `InputRouter` now actually does something: pop to root via
   `NavStack::popToRoot()`. This is handled once in `NavStack`, not per-screen (PLAN.md §3.1) —
   don't add a `KEY_HOME` case to every individual screen's `handleInput`.

## Data / API dependencies

Everything in this phase is already-public per PLAN.md §3.4:
- `getNumContacts()` / `getContactByIdx()` — `BaseChatMesh.h:176-177`.
- `getChannel()` — iterate `0..MAX_GROUP_CHANNELS-1`.
- `ContactInfo` struct fields — `src/helpers/ContactInfo.h`.

No new methods needed on `MyMesh`/`Mesh`/`BaseChatMesh` for this phase.

## Open questions relevant to this phase

None from PLAN.md §8 specifically block Phase 2. Worth a sanity check while building
`Screen_ContactDetail`: confirm `ADV_TYPE_*` values map to a small enough set that a plain
switch/lookup table (not a new helper) is enough to render a human-readable type string.

## Done-when / manual test checklist

```
Board: RAK_4631                Env: RAK_4631_companion_radio_forest_ble             Phase: 2
Board: gat562_30s_mesh_kit     Env: GAT562_30S_Mesh_Kit_companion_radio_forest_ble  Phase: 2
Board: heltec_rc32             Env: heltec_rc32_companion_radio_forest_ble          Phase: 2

[ ] Builds clean: pio run -e <env>
[ ] Boots to splash, dismisses to a real top-level menu (Home)
[ ] Every Phase-1 screen is reachable through the Home menu (Submenu items push correctly)
[ ] Back/cancel from N levels deep returns exactly one level
[ ] KEY_HOME (long-press-anywhere / joystick back-triple-click) returns to root from any depth
[ ] Contacts list shows every seeded contact; scroll-into-view works once the list exceeds
    visible-row count; selecting one opens Screen_ContactDetail with correct fields
[ ] Channels list shows every seeded channel, skips empty slots
[ ] No full-contact-list buffer copy anywhere (spot-check Screen_Contacts's read path)
[ ] Auto-off / low-battery / shutdown-confirm still behave correctly (regression check)
[ ] No visual artifacts on this board's display type
```
