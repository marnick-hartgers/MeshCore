# Design 2 — Glanceable Grid

A dense, static 2x2 stat grid below a single combined name+toggle row.
Nothing scrolls — every value on screen updates in place. This is the design
that leans hardest into "handy info at a glance" from `phases/ToDo.md`, at the
cost of being the most new code: a bespoke `UIScreen` with its own 2D focus
cursor, not a reskin of `MenuScreen`'s linear list.

## Mockup — 128x64 monochrome OLED

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |  <- status bar, 11px
+----------------------------------------+
| Marnick-Node1     (gps:ON) (buz:OFF)   |  <- name + inline toggle icons
+------------------------+---------------+
| Contacts          12   | Uptime 3d 04h |
+------------------------+---------------+
| Pkt Rx/Tx    842 / 301 | Noise -121dBm |
+----------------------------------------+
```

Focused-tile state (NEXT/PREV moves a highlight across the 4 tiles,
row-major — GPS/Buzzer icons count as the first two focus stops):

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
| Marnick-Node1     [gps:ON] (buz:OFF)   |  <- [brackets] = focused, ENTER flips
+------------------------+---------------+
| Contacts          12   | Uptime 3d 04h |
+------------------------+---------------+
| Pkt Rx/Tx    842 / 301 | Noise -121dBm |
+----------------------------------------+
```

Same layout scaled up on a 128x128 panel (the two e-ink boards, or the color
`sensecap_indicator`/`heltec_rc32` boards) — the grid grows to 3x2 for free,
no new logic, just more tiles fit in the extra vertical space:

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
| Marnick-Node1     (gps:ON) (buz:OFF)   |
+------------------------+---------------+
| Contacts          12   | Channels   3  |
+------------------------+---------------+
| Uptime         3d 04h  | Queue      0  |
+------------------------+---------------+
| Pkt Rx/Tx    842 / 301 | Noise -121dBm |
+----------------------------------------+
```

## Interaction

- NEXT/PREV move a focus highlight across tiles in row-major order (wrapping
  at the end back to the first tile) — this is **new** input handling, since
  `MenuScreen`'s existing NEXT/PREV only ever walks a single vertical list, not
  a 2D grid. The GPS/Buzzer icons are the first two focus stops, so they're
  reachable the same way as any stat tile, not a special case.
- ENTER on the GPS/Buzzer tile **flips it immediately, inline** — no pushed
  screen, no pop. This is actually a simplification over Design 1, not added
  complexity: `FormField.h`'s existing comment already establishes that Toggle
  is the one field type that "commits on every ENTER" with no staged/working
  copy, precisely because a 2-state value has nothing worth staging. That same
  logic applies just as well to an inline flip as to the current pushed-editor
  flip — this design just skips the push/pop, calling the get/set pair
  directly from the grid's own `handleInput()`.
- ENTER on a stat tile: recommend no-op (glance-only) rather than drilling into
  a detail screen — keeping the grid's job purely "at a glance" and leaving
  drill-down to Diagnostics/Contacts/Channels, reachable via the existing
  triple-click-to-submenu gesture, same as Design 1.

## Trade-offs

**Pros**
- Best glanceability of the three designs on the dominant 128x64 panel: 4
  stats + name + both toggle states are all visible simultaneously, zero
  scrolling, matching `phases/ToDo.md`'s "handy and useful info" framing most
  directly.
- Genuinely improves (not just tolerates) on the two e-ink boards
  (`lilygo_techo`, `ThinkNode_M1`, 128x128, no partial refresh): a fully static
  grid with nothing scrolling is the *best-case* layout for e-ink specifically
  — there's no marquee to disable (unlike the current text-based status bar)
  and no scroll-position state to manage across refresh cycles.
- Scales up cleanly on bigger panels — more tiles fit, not just "more rows
  before you need to scroll" the way a list does.
- Inline toggle flip (see Interaction) is arguably a net *simplification* of
  the toggle mechanism versus what exists today, not an addition.

**Cons**
- Highest implementation cost of the three: a new `UIScreen` subclass with its
  own layout math (tile geometry, row-major focus cursor with wraparound) and
  its own `handleInput()` — none of it reuses `MenuScreen`.
- New 2D-grid input handling is a first for ui-forest; every other screen in
  the app is a single vertical list or a leaf form field, so this introduces a
  navigation pattern (and a wraparound-focus convention) that doesn't exist
  anywhere else to copy from or stay consistent with.
- Only 4 stat tiles are visible at once on the dominant 128x64 panel (before
  any scrolling) — fewer than Design 1 could show if scrolled, and far fewer
  than the top-10 list in `design-1-status-ledger.md`'s appendix, so most of
  the 10 candidate stats stay hidden regardless of which 4 are picked, with no
  affordance shown here for reaching the rest (would need e.g. a "more..."
  fifth tile linking to Diagnostics).
- Tile labels are necessarily abbreviated ("Pkt Rx/Tx", "Noise") to fit a
  ~10-11 character-wide half-row — less scannable for a first-time user than
  Design 1's full-width label:value rows.
