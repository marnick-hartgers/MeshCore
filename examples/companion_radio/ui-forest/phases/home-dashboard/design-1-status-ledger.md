# Design 1 — Status Ledger

A scrolling label:value list. Name and the two toggles sit as the first three
rows of the same list the stats live in — everything is one `MenuScreen`-style
scroll, nothing is a distinct widget type. This is the design that reuses the
most existing code: `MenuScreen`, `MenuItemKind::Toggle` + the shared
`ToggleField` editor, and the existing row-rendering/scroll-indicator
conventions every other ui-forest list screen already uses.

## Mockup — 128x64 monochrome OLED (dominant target: 5 of 8 `_forest` boards)

Status bar (11px) + 4 content rows (11px each, per `Layout::visibleRows()`).
First screenful, list scrolled to the top:

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |  <- status bar, 11px
+----------------------------------------+
| Marnick-Node1                          |  <- name (row 1)
| GPS              [ ON ]                |  <- toggle (row 2)
| Buzzer           [ OFF]                |  <- toggle (row 3)
| Uptime           3d 04h            v   |  <- first stat row, "v" = more below
+----------------------------------------+
```

Same screen after the user presses NEXT a few times to scroll past the
toggles into the stat list:

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
| ^ Uptime         3d 04h                |  <- "^" = more above
| Contacts         12                    |
| Packets Rx/Tx     842 / 301            |
| Noise floor      -121 dBm          v   |
+----------------------------------------+
```

Status bar icon legend for this and every design doc in this folder:
`(sat)` = GPS off/no-fix/fix (filled when locked, hollow when searching, absent
when off), `(link)` = transport off/enabled-disconnected/connected, `(buzz)` =
buzzer on/off, `@3` = unread count (envelope + clamped digit), `[||||  ]` =
existing battery gauge, unchanged from today.

## Interaction

- PREV/NEXT scroll the ledger exactly like every other list screen in
  ui-forest today (`MenuScreen`'s existing behavior) — no new input handling.
- ENTER on the GPS/Buzzer rows pushes the existing shared `ToggleField` editor
  screen, flips on ENTER, pops back — identical to how every other
  `MenuItemKind::Toggle` row already works (`FormField.h`). Zero new
  interaction code.
- Contacts/Channels/Recent Events/Diagnostics/Settings become either
  additional rows appended below the stats, or (cleaner) stay reachable via
  the existing triple-click/long-press-to-submenu gesture rather than
  cluttering the ledger further — worth deciding once a design is picked.

## Trade-offs

**Pros**
- By far the lowest implementation risk: it's a `MenuScreen` with a couple of
  non-`Action` row kinds added (or literally re-skinned as a new class that
  copies `MenuScreen`'s row/scroll logic) — no new rendering primitives, no new
  gesture handling, no new widget class.
- Scales for free to bigger displays: `Layout::visibleRows()` already reports
  more rows on a 128x128 panel, so more stats are visible at once there with
  zero extra code.
- Consistent with the rest of the app's visual language — a user who's already
  used Settings/Diagnostics/Contacts sees the same row style on Home.

**Cons**
- Least glanceable of the three designs on the dominant 128x64 panel: Name +
  2 toggles already consume 3 of the 4 visible rows, leaving only **one** stat
  visible without scrolling. The "handy at-a-glance info" goal from
  `phases/ToDo.md` is the weakest fit here of the three designs — this design
  wins on engineering cost, not on the glanceability the request emphasizes.
- Toggling GPS/Buzzer still means a full-screen push-and-pop (via
  `ToggleField`) even though they're "on the home screen" — visually inline,
  but not interaction-wise inline.

## Appendix — Top 10 candidate stats

Independent of which design ships; the same list applies to all three docs in
this folder. Ordered roughly by how cheaply and reliably each is available
today (per the codebase survey backing this plan), not by importance — the
user picks the final subset.

| # | Stat | Source | Notes |
|---|---|---|---|
| 1 | Uptime | `millis()` since boot (already tracked for scheduling elsewhere) | Always available, zero new plumbing. |
| 2 | Contacts count | `BaseChatMesh::getNumContacts()` | Already used by `Screen_Contacts`. |
| 3 | Channels count | Channel table size, already used by `Screen_Channels` | |
| 4 | Packets sent / received | `the_mesh.getNumSent()`, radio driver recv counters | Already surfaced in `Screen_DiagPackets`. |
| 5 | Noise floor / last RSSI / SNR | `radio_driver.getNoiseFloor()/getLastRSSI()/getLastSNR()` | Already surfaced in `Screen_DiagRadio`; signal-bar icons already exist in `icons.h` but are unwired pending per-backend RSSI-sentinel verification. |
| 6 | Battery voltage / estimated runtime | `getBattMilliVolts()`, `isExternalPowered()` | Voltage already read for the status-bar gauge; a runtime estimate would be a first derivative of it, no new hardware read. |
| 7 | Last-heard node (name + time ago) | `the_mesh.getRecentlyHeard()` | Already used by `Screen_Recents`. |
| 8 | Send/receive queue depth | `MyMesh::getQueueLen()` | Already surfaced in Diagnostics; a good "is this thing backed up" signal. |
| 9 | GPS sats-in-view / fix quality | `LocationProvider::satellitesCount()` | Already used by `Screen_Gps`; complements the status-bar fix icon with a number. |
| 10 | Node short-ID / firmware version + build date | `Identity` pubkey prefix; version string already shown on `Screen_Splash` | Cheap, static, good "which device/build am I looking at" confirmation without opening Settings. |
