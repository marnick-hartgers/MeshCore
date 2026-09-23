# Design 4 — Icon Tile Launcher

Icons instead of text, wherever an icon can carry the meaning on its own. The
home screen is a grid of icon tiles — some are stat glances (Signal, GPS
state), some are toggles (GPS, Buzzer), some are navigation shortcuts
(Contacts, Channels, Settings, Diagnostics), and one is a direct action
(Advert). A single shared caption line below the grid names whatever's
currently focused; the device name shows there the rest of the time. This is
the design that leans hardest into "icons instead of text where possible,"
and it happens to be the cheapest of the four on new icon assets — most of the
icons it needs already exist in `icons.h`.

Shutdown is deliberately **not** on this grid — it's a rarely-used, high-
consequence action, so it stays at its default disposition from `plan.md` §3
(`Settings > Device > Shutdown/Restart`) instead of occupying one of the 8
prime Home slots. Advert takes that 8th slot instead: it's used often enough,
and low-consequence enough (broadcasting an advert is harmless and reversible
in effect), to earn a one-tap Home spot — this resolves `plan.md` §3's
"judgment call" on `Screen_Advert` in favor of keeping it a direct shortcut.

## Mockup — 128x64 monochrome OLED

Idle state (nothing focused yet — caption line shows the device name):

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |  <- status bar, 11px
+----------------------------------------+
|  (fix)    (mute)   (pers)    (hash)    |  <- row 1: GPS  Buzzer  Contacts  Channels
|                                        |
|  (bars)   (list)   (gear)   (advt)    |  <- row 2: Signal  Diag  Settings  Advert
|                                        |
+----------------------------------------+
|            Marnick-Node1               |  <- caption: device name (idle)
+----------------------------------------+
```

After NEXT has moved focus to the Contacts tile (caption swaps to show it):

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
|  (fix)    (mute)  [ pers ]   (hash)    |  <- [brackets] = focused tile
|                                        |
|  (bars)   (list)   (gear)   (advt)    |
|                                        |
+----------------------------------------+
|            Contacts: 12                |  <- caption: focused tile's label + value
+----------------------------------------+
```

Icon key for this mockup (all but `(mute)`-on and `(advt)` reuse existing
`icons.h` assets as-is):

| Tile | Icon | Source |
|---|---|---|
| GPS | `(fix)`/`(nofix)` diamond, dimmed/slashed when GPS is off | `gps_fix_icon` / `gps_nofix_icon` — already exist |
| Buzzer | `(mute)` speaker-slash when off | `muted_icon` exists (8x8, status-bar scale); a 16x16 "on" glyph doesn't exist yet — same gap `plan.md` §1 already flags |
| Contacts | `(pers)` person silhouette | `contact_icon` — already exists |
| Channels | `(hash)` `#` glyph | `channel_icon` — already exists |
| Signal | `(bars)` 0-4 signal bars | `signal_bars[0..4]` — already exist, but flagged in `icons.h` as unwired/unverified against real RSSI sentinel values per radio backend, same caveat `plan.md`'s stat table already notes |
| Diagnostics/Event Log | `(list)` bullet list | `event_log_icon` — already exists |
| Settings | `(gear)` equalizer-style stand-in | `settings_icon` — already exists |
| Advert | `(advt)` broadcast glyph | `advert_icon` exists, but only at 32x32 (used today as a full-screen glyph on `Screen_Advert`) — a 16x16 version is new, same category of gap as the Buzzer-on icon |

## Mockup — 128x128 panel (e-ink / color boards)

More room means the grid can grow to 4x3 (12 tiles) *and* keep a persistent
name row separate from the caption line, rather than sharing one row for both:

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
|            Marnick-Node1               |  <- persistent name row (room to spare here)
+----------------------------------------+
|  (fix)   (mute)  (pers)  (hash)        |
|  (bars)  (list)  (gear)  (advt)        |
|  ( ?  )  ( ?  )  ( ?  )  ( ?  )        |  <- 3rd row: 4 free slots for stats
+----------------------------------------+
|            Contacts: 12                |  <- caption: focused tile only
+----------------------------------------+
```

The 4 empty slots on the bigger panel are exactly where hand-picked items from
the top-10 stats list (`design-1-status-ledger.md`'s appendix) would go —
Uptime, Queue depth, Noise floor, etc. — once the user picks which ones matter
most, since the smallest 128x64 panel doesn't have room to spare for them.

## Interaction

- NEXT/PREV move a focus highlight across tiles in row-major order, wrapping —
  same mechanism as Design 2's grid (`design-2-glanceable-grid.md`), not a new
  invention on top of that one.
- ENTER on the GPS/Buzzer tiles flips inline immediately — same reuse of
  `FormField.h`'s "Toggle commits on every ENTER, no staging" precedent that
  Design 2 already established for its own toggle icons.
- ENTER on Contacts/Channels/Settings/Diagnostics pushes the corresponding
  existing screen — this is exactly `MenuItemKind::Submenu`'s existing
  behavior (`MenuScreen.h:9`, "push another `UIScreen` onto the nav stack"),
  just triggered from a grid cursor position instead of a linear list index.
  No new navigation primitive, only a new way of picking which item is
  "selected."
- ENTER on the Advert tile fires the send-advert action directly (`MenuItemKind::Action`,
  the same kind `Screen_Advert`'s existing menu row already uses) and shows the
  result the same way `Screen_Advert` does today (toast), rather than pushing a
  screen.
- The caption line shows the device name when idle, swaps to the focused
  tile's label + live value while the user is navigating, and reverts to the
  name after a short idle timeout. This reuses the same "timer-driven poll()"
  shape as Design 3's auto-rotation, but the cost is much smaller — it redraws
  one 11px text row, not the whole panel, so it's cheap even on the two e-ink
  boards.

## Trade-offs

**Pros**
- Cheapest of the three icon/grid-style designs on new icon assets: of the 8
  tiles above, 6 reuse existing 16x16 icons (`gps_fix_icon`, `gps_nofix_icon`,
  `contact_icon`, `channel_icon`, `event_log_icon`, `settings_icon`) and one
  more reuses an existing 5-frame set (`signal_bars`) — only two are genuinely
  new: the Buzzer-on glyph (a gap `plan.md` §1 already flags independent of
  this design) and a 16x16 Advert glyph (today's `advert_icon` is 32x32 only).
- Directly resolves the open question `plan.md` §3 leaves dangling for
  Contacts/Channels/Diagnostics/Settings ("reachable from the dashboard, exact
  presentation depends on which design ships") — here they *are* the
  dashboard, not a separate gesture away from it. It also resolves §3's
  `Screen_Advert` judgment call in favor of a direct Home shortcut, while
  Shutdown — rarely used and high-consequence — correctly stays off Home
  entirely and follows the default `Settings > Device` disposition. If this
  design ships, most of `plan.md`'s migration table simplifies to "becomes a
  Home tile," except Shutdown, which still moves to Settings as planned.
- Most literally matches the request: nearly every element on the primary
  128x64 screen is an icon, with text confined to a single shared caption line
  and the status bar.
- Caption-row-only redraw on focus change is gentler on e-ink than Design 3's
  full-panel auto-rotate, while still being cheaper to build than Design 2's
  always-on multi-cell text grid.
- Scales well to bigger panels — more tiles fit directly, and there's slack
  left over for hand-picked top-10 stats once the core nav/toggle tiles are
  placed (see the 128x128 mockup).
- Keeping Shutdown off Home means all 8 tiles are the same 16x16 size — no
  special-cased oversized tile needed, and the rarely-used, high-consequence
  action simply isn't reachable by an accidental tap on Home at all, which is
  a stronger guarantee than making its tile merely bigger would have been.

**Cons**
- Weakest fit for the top-10 stats list of the four designs on the *smallest*
  panel: with GPS/Buzzer/Contacts/Channels/Diagnostics/Settings/Advert already
  claiming 7 of 8 tiles, only Signal has room on a 128x64 board — the rest of
  the top-10 list has nowhere to live here unless some nav tiles are cut or
  deferred to a submenu, which somewhat undercuts "home screen shows handy
  stats" from `phases/ToDo.md` in favor of "home screen is the app launcher."
  Worth deciding whether that's the goal or a side effect.
- Icon-only legibility risk: distinguishing the Diagnostics (`event_log_icon`,
  a bullet list) tile from the Settings (`settings_icon`, an equalizer
  stand-in) tile at 16x16 monochrome is less immediately obvious than reading
  the word "Diagnostics" or "Settings" — the caption line mitigates this once
  a tile is focused, but a new user's *first* glance at the unfocused grid is
  less self-explanatory than Design 1's full-width labeled rows.
- Same new-input-handling cost as Design 2 (2D grid focus cursor, not reused
  from `MenuScreen`'s linear list).
- Depends on the same unresolved `signal_bars` verification caveat `plan.md`
  already carries (RSSI "no packet yet" sentinel differs per radio backend,
  never checked against real hardware) — the Signal tile can't ship confidently
  until that's settled, same blocker as the status bar's signal icon idea.

## Status
This design has been chosen. See [`implementation-plan.md`](implementation-plan.md)
for the sequenced, code-grounded build plan (new icon assets, status bar
rewrite, the new `Screen_HomeDashboard` class, legacy-screen retirement, and
the ambient notification blink).

## Where this leaves the "Designs" comparison

Versus the other three: this one is the most icon-forward and the only one
that folds navigation shortcuts into the home screen itself, at the cost of
having the least room left for stats on the smallest displays. If the user's
priority is "glanceable status + a few key toggles," Design 2 fits better; if
it's "minimize how many things live in Settings, make Home feel like the
whole app," this one fits better.
