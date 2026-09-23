# Home dashboard & status bar redesign — planning

Companion doc to [`phases/ToDo.md`](../ToDo.md). Covers all four asks from that
file except the 3 home-screen designs, which are their own docs in this folder
(see bottom of this file). Grounded against the current implementation, not
just the wishlist — see "Current state" under each section.

## 1. Status bar redesign

### Current state
`StatusBar` (`StatusBar.h/.cpp`) renders one scrolling/static text line built
by `UITask::updateStatusBar()` (`UITask.cpp:399-419`):

```
<name> | BUZ:ON/OFF | GPS:ON/OFF | <TRANSPORT>:ON/OFF -
```

plus a battery gauge drawn separately, top-right, by `StatusBar::renderBattery()`.
Only the battery is icon-based today. Specifics worth noting before redesigning:

- **GPS** only reports the on/off *setting* (`getGPSState()`), never fix state.
  Fix data (`LocationProvider::isValid()`, `satellitesCount()`) already exists
  and is used by `Screen_Gps` — it's just never reached `StatusBar`.
- **"BLE"** actually reports `isSerialEnabled()` — whether the *configured
  transport* (BLE/WiFi/Ethernet/USB, see `Transport.h`) is turned on, not
  whether a client is connected. A real tri-state connection signal already
  exists (`hasConnection()`, fed from `_serial->isConnected()` in
  `MyMesh.cpp:2274`) but today only `Screen_Status` reads it.
- **Buzzer** is 2-state and already has an icon (`muted_icon`, 8x8), but it's
  drawn as a conditional overlay next to the battery gauge, not a normal status
  slot — it appears/disappears rather than switching between an on-icon and an
  off-icon.
- **Unread count** isn't in the status bar at all today. `_msgcount` exists
  (`UITask.h:173`) and drives the status LED blink pattern, but `StatusBar`
  never sees it.

No `DeviceStatus`-style struct exists — `updateStatusBar()` just calls five
independent getters every tick. A redesign can keep doing that, or introduce
one aggregate struct if that reads cleaner once there are 5 indicators instead
of 3.

### Proposal
Replace the scrolling text line with a fixed icon cluster: battery · GPS ·
transport · buzzer · unread count, left-to-right or evenly spaced across the
bar. Concretely:

- **Drop the node name from the status bar entirely.** It's the one variable-
  width element forcing the marquee/scroll logic to exist, and the ToDo.md
  already wants a name line on the home screen body. Removing it from the bar
  turns the whole strip into fixed-width icons — no scroll state, no
  e-ink-vs-non-e-ink branch, no width math. Every board gets a static bar,
  simplifying `StatusBar` regardless of which home-screen design ships.
- **GPS**: 3-state icon — off / no-lock / lock. The existing `gps_fix_icon` /
  `gps_nofix_icon` (hollow vs. filled diamond, `icons.h:184-193`) already
  establish the right visual language for fix vs. no-fix; "off" would be
  either the no-fix glyph dimmed/omitted or a distinct empty-outline glyph.
  These two existing icons are 16x16 — sized for menu rows, not an 11px status
  strip. See "Icon budget" below.
- **Transport (BLE/USB/WiFi/Ethernet)**: 3-state icon — off / enabled-but-
  disconnected / connected. Wire `hasConnection()` in alongside the existing
  `isSerialEnabled()` so the icon can distinguish "turned on, nobody's talking
  to it" from "actually connected," which the current text (`BLE:ON`/`BLE:OFF`)
  can't. Open question: a literal Bluetooth glyph is wrong for USB/WiFi/
  Ethernet builds — probably wants a generic link/plug icon with the
  Bluetooth-specific glyph reserved for BLE builds only, or one generic icon
  for all transports and let users check Settings for which transport is
  configured.
- **Buzzer**: on/off icon in a fixed slot (not a conditional overlay). Reuses
  the existing muted glyph for "off"; needs a matching "on" glyph.
- **Unread count**: no digit-icon set exists anywhere in this codebase, and
  building one is disproportionate for a single number. Simplest: draw the
  count as small text (`display.print()`) next to a fixed "envelope" icon,
  clamped (e.g. "9+") so it can't blow the bar's fixed width budget on boards
  with many queued messages.

### Icon budget
`Layout::statusBarHeight(true)` is `rowHeight(1)` = 11px. The existing 16x16
icon set (`gps_fix_icon`, `gps_nofix_icon`, `bluetooth_on/off` at 32x32) is
sized for menu rows and full-screen glyphs — none of it fits vertically in an
11px strip. Two options, worth deciding before implementation:

1. Hand-author a small (~8x8, matching `muted_icon`'s scale) icon set
   specifically for the status bar: GPS off/no-fix/fix, transport off/
   disconnected/connected, buzzer on/off. Keeps the bar at its current 11px.
2. Grow `statusBarHeight()` to fit the existing 16x16 icons directly (reusing
   `gps_fix_icon`/`gps_nofix_icon` as-is). Costs ~5px of the ~53px content area
   on the dominant 128x64 panels (5 of 8 `_forest` boards) — one fewer content
   row on those boards specifically.

Recommend option 1 — reusing the existing hand-authored-XBM process (see
`icons.h`'s Phase 5 comment block) at status-bar scale — since it doesn't cost
any of the already-tight content area on the smallest boards.

### Data plumbing summary

| Indicator | Getter | Status |
|---|---|---|
| Battery % | `getBattMilliVolts()` | wired today |
| GPS enabled | `getGPSState()` | wired today (on/off only) |
| GPS fix | `LocationProvider::isValid()` / `satellitesCount()` via `SensorManager` | data exists, not reaching `StatusBar` yet |
| Transport enabled | `isSerialEnabled()` | wired today |
| Transport connected | `hasConnection()` | exists on `AbstractUITask`, unused by `StatusBar` |
| Buzzer | `isBuzzerQuiet()` | wired today, needs a fixed icon slot instead of a conditional overlay |
| Unread count | `_msgcount` / `getMsgCount()` | tracked already, never surfaced in the bar |

Net: **no new hardware plumbing required** — every value either already
reaches `UITask`, or is one function call away via a class `UITask` already
holds a reference to. The work is in `StatusBar`'s rendering, a small icon
set, and wiring `hasConnection()` + `LocationProvider` into
`updateStatusBar()`.

## 2. Home / dashboard screen

Today `_home` (`UITask.cpp:199`) is a plain `MenuScreen` — a 13-row flat list.
Per the architecture, a dashboard needs a genuinely new `UIScreen` subclass
(dashboards aren't list-shaped, so `MenuScreen` isn't the right base), wired as
the new `_home` in `UITask::begin()`.

Required contents, per `phases/ToDo.md`:
- Name (compacted/truncated — now that it's off the status bar, this is its
  only home)
- GPS toggle
- Buzzer toggle
- Stats/details block

Toggling today is a **full-screen push**: `MenuItemKind::Toggle` rows open the
shared `ToggleField` editor screen (`FormField.h:48-64`), ENTER flips the value
immediately, then it pops back (`FormField.h:22-32`'s staged-commit comment —
Toggle is the one field type that commits on every ENTER, no staging). Reusing
that exact mechanism for the two home-screen toggles is the lowest-risk path
(zero new interaction code); an inline toggle-without-navigating-away is
possible but is new interaction surface, not a reuse of anything that exists
today — flagged per-design in the three design docs, since it affects each
design's complexity differently.

## 3. Retiring the ui-new-derived screens from Home

The 8 screens `ARCHITECTURE.md:402-433` documents as straight ports of
`ui-new`'s swipeable `HomeScreen` pages, currently flat rows on Home:

| Screen | Disposition |
|---|---|
| `Screen_Status` | Delete outright — msg count and transport connection state are fully superseded by the redesigned status bar (§1) + home screen; nothing left it uniquely shows. |
| `Screen_Recents` | Move under Settings (e.g. `Settings > Network > Recently Heard`) — it's a data browser, not a config screen, so it's a slightly odd fit for "Settings," but it doesn't belong on Home either. Flagged as a judgment call for whoever finalizes the settings tree — could equally live as a sibling of Contacts/Channels instead of under Settings if "Settings" is meant to stay config-only. |
| `Screen_RadioInfo` | Check for overlap with `Screen_DiagRadio` (Phase 4) first — likely a near-duplicate. If so, delete and let Diagnostics cover it; otherwise fold its unique fields into `Screen_DiagRadio`. |
| `Screen_Bluetooth` | Fold into `Settings > Device` (or `Network`) as a toggle row — it's just a transport-enable toggle + pairing PIN display today. |
| `Screen_Advert` | Judgment call: it's an action (send now), not a setting, so it may belong as a direct Home shortcut rather than buried in Settings. Default recommendation: `Settings > Network > Send Advert`, matching "everything from the legacy Home rows moves to Settings" — but flag this one for the user to confirm, since it's the one legacy row that's arguably still a Home-level action rather than configuration. |
| `Screen_Gps` | `Settings > Device > GPS` (toggle + fix/sats/lat/lon/alt readout stays here); the toggle itself is *also* promoted to Home directly per §2. |
| `Screen_Sensors` | `Settings > Device > Sensors`, same `UI_SENSORS_PAGE` gating as today. |
| `Screen_Shutdown` | `Settings > Device > Shutdown/Restart` — same `ConfirmScreen` flow, just relocated off the flat Home list. |

Net effect on Home: the 13-row flat list (Status, Recent, Radio, Bluetooth,
Advert, Contacts, Channels, [GPS], [Sensors], Recent Events, Diagnostics,
Settings, Shutdown) collapses to the new dashboard screen, with Contacts,
Channels, Recent Events, Diagnostics, and Settings the only direct
Home-adjacent entry points left (reachable from the dashboard, exact
presentation depends on which design ships). This table assumes Designs 1-3,
where Home is stats-only and everything else moves under Settings. Design 4
(`design-4-icon-tiles.md`) resolves most of these rows differently —
Contacts/Channels/Diagnostics/Settings become Home tiles directly instead of
moving to Settings, and Advert becomes a Home tile too rather than a Settings
row — but Shutdown still follows this table's disposition (`Settings > Device
> Shutdown/Restart`) even under Design 4. If that design is chosen, revisit
this table for everything except Shutdown rather than applying it as-is.

## 4. Ambient notification blink while connected

### Current state
`UITask::newMsg()` (`UITask.cpp:214-229`) always does `_nav.reset(_msg_preview)`
— a full-screen interrupt overlay — regardless of connection state. The only
connection-aware branch is display wake: `if (!_display->isOn() &&
!hasConnection()) _display->turnOn();`. So today:
- Display already on + connected → full-screen preview still interrupts
  whatever the user was looking at.
- Display off + connected → **nothing visible on-device at all** (message
  goes to the companion app only; screen stays off).

Neither of those is quite "a small blink" — one is a full takeover, the other
is silence. That's the gap `phases/ToDo.md` is pointing at.

### Proposal
When `hasConnection()` is true at the moment `newMsg()` fires, skip the full
`nav.reset(_msg_preview)` takeover (the phone app already owns showing the
message) and instead:
- Rely on the redesigned status bar's unread-count indicator (§1) for the
  persistent "you have unread messages" state — already covered once §1 ships.
- Add a brief transient cue at the moment the message arrives: e.g. a 1-2s
  invert/flash of the unread badge, or a short pulse of the status LED
  (`PIN_STATUS_LED` path already exists, `UITask.cpp:362-381`) — without
  calling `_display->turnOn()` or extending `_auto_off`, so it doesn't wake a
  sleeping display or steal focus from the current screen.
- Keep the existing full-interrupt `Screen_MsgPreview` behavior exactly as-is
  for the `!hasConnection()` case — there the device is the only place the
  user will ever see the message, so the current takeover behavior is correct
  and shouldn't change.

## Top 10 candidate stats
See [`design-1-status-ledger.md`](design-1-status-ledger.md)'s appendix — the
list is identical across all four designs (it's independent of layout), so
it's written up once there rather than duplicated in every design doc. Short
version: uptime, contacts/channels count, packets sent/received, last-heard
node, queue depth, noise floor, battery voltage/runtime estimate, node ID
short-form, firmware version/build date, GPS sats-in-view. Full rationale +
data source per item in that doc.

## Designs
Four alternative home-screen layouts, one doc each, all built against the
same status bar (§1), toggle mechanism (§2), and stats list (above):

1. [`design-1-status-ledger.md`](design-1-status-ledger.md) — scrolling
   label:value list, reusing the existing `MenuScreen`/list paradigm. Lowest
   implementation risk.
2. [`design-2-glanceable-grid.md`](design-2-glanceable-grid.md) — dense 2-column
   stat grid, everything visible at once, no scrolling.
3. [`design-3-focused-carousel.md`](design-3-focused-carousel.md) — one large
   stat at a time, paged/rotated, optimized for readability over density.
4. [`design-4-icon-tiles.md`](design-4-icon-tiles.md) — icon-first tile grid;
   nearly everything on screen is an icon rather than text, and the tiles
   double as navigation shortcuts (Contacts/Channels/Settings/Diagnostics) plus
   a direct Advert action, which changes §3's migration story if this design
   ships — see that doc's closing section. Shutdown still follows the default
   Settings disposition even in this design (rarely used, high-consequence,
   deliberately not a one-tap-away Home tile).

## Status: Design 4 chosen — all open questions answered
Design 4 (Icon Tile Launcher) has been selected, so Designs 1-3 are no longer
live alternatives — the answers below are final for this build, not
conditional. The sequenced code changes live in
[`implementation-plan.md`](implementation-plan.md); this doc and
`design-4-icon-tiles.md` are the design rationale behind those decisions.

1. **Which home-screen design?** Design 4 — Icon Tile Launcher
   (`design-4-icon-tiles.md`).
2. **Which of the top-10 stats ship?** None yet, and that's intentional, not
   an oversight: Design 4's 8 fixed tiles (GPS, Buzzer, Contacts, Channels,
   Signal, Diagnostics, Settings, Advert) already saturate the 128x64 grid —
   there's no free slot for a stat pick in v1. The top-10 list
   (`design-1-status-ledger.md`'s appendix) becomes relevant only if/when the
   bigger-panel expansion (128x128 boards growing to a 4x3 grid, per
   `design-4-icon-tiles.md`'s second mockup) is undertaken — that's deferred
   work (`implementation-plan.md`'s "Deferred / future work"), and picking
   which stats fill those extra slots is left for that point, per the
   original `phases/ToDo.md` request ("Ill handpick them later").
3. **`Screen_Advert` disposition?** Home tile — the dashboard's Advert tile
   pushes the existing `Screen_Advert` screen (`_nav.push(_advert)`), reusing
   its existing press-to-arm confirmation + toast/log behavior rather than
   moving it to Settings. Confirmed against `Screen_Advert.cpp`: it's not a
   fire-and-forget action, so the tile has to route through it, not around it
   (`implementation-plan.md`, decisions table + Phase 3.6).
4. **Status bar icon budget?** New ~8x8 icon set, `statusBarHeight()` stays at
   11px (`implementation-plan.md` Phase 1/2) — doesn't cost any of the tight
   content area on the dominant 128x64 boards.
5. **Transport icon?** One generic link/plug glyph for every transport
   (BLE/USB/WiFi/Ethernet) rather than a Bluetooth-specific glyph gated per
   build — cheap to swap for a BLE-specific icon later if it reads
   ambiguously in practice.
