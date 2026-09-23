# Design 3 — Focused Carousel

One stat at a time, shown large, paged through (manually or by slow
auto-rotation) below a persistent name+toggle header. Optimized for
readability over density — the opposite trade-off from Design 2's grid.

## Mockup — 128x64 monochrome OLED

Persistent header (never pages), one big stat below it:

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |  <- status bar, 11px
+----------------------------------------+
| Marnick-Node1     (gps:ON) (buz:OFF)   |  <- persistent header row
+----------------------------------------+
|              Uptime                    |  <- label, text size 1
|             3d 04h 12m                 |  <- value, text size 2 (large)
|          <  o o o * o o o o  >         |  <- page dots, current position lit
+----------------------------------------+
```

One step later (manual NEXT, or auto-rotated after ~4-5s idle):

```
+----------------------------------------+
|(sat)(link)(buzz)      @3      [||||  ] |
+----------------------------------------+
| Marnick-Node1     (gps:ON) (buz:OFF)   |
+----------------------------------------+
|              Contacts                  |
|                  12                    |
|          <  o o * o o o o o  >         |
+----------------------------------------+
```

## Interaction

- PREV/NEXT step the carousel one stat at a time (wrapping); pressing either
  pauses auto-rotation for a while so the device doesn't flip away from the
  stat the user just navigated to.
- Auto-rotate is a reuse of an existing pattern already in this codebase —
  `UIScreen::poll()` already runs every tick regardless of input (used today
  by `StatusBar`'s scroll timer and the status-LED blink timer,
  `UITask.cpp:362-381`), so a "rotate to next stat every N seconds" timer is
  the same shape of code, not a new concept.
- The header's GPS/Buzzer toggles reuse Design 2's inline-flip approach (ENTER
  flips immediately, no pushed screen) rather than Design 1's full-screen
  `ToggleField` push.
- **Open risk, not yet resolved:** this design wants two distinct things from
  the same NEXT/PREV keys — "page through stats" (the carousel's main job) and
  "move focus to the header toggles" (so GPS/Buzzer stay reachable without
  leaving Home). Design 2 only needed one axis (grid focus); this design needs
  a second, and the simplest single-button/analog boards' gesture vocabulary
  (`InputRouter`'s NEXT/PREV/ENTER + a triple-click already claimed for
  `KEY_HOME`) doesn't obviously have a free gesture left for "jump focus to
  header." Needs a decision before this design is buildable as specified —
  e.g. holding NEXT/PREV, a long-press, or accepting that toggles require
  leaving Home on the simplest boards after all.

## Trade-offs

**Pros**
- Best readability of the three designs — large text, one thing to read at a
  time, good for a quick glance from a distance or in poor lighting.
- Auto-rotation means genuinely zero interaction needed to see every stat
  eventually — useful for a device sitting on a desk/dash rather than held.
- Combined with the e-ink note below, this is the gentlest design on the two
  e-ink boards *if* auto-rotate is disabled there (see next point) — a single
  manual page-flip is one full e-ink refresh, same cost as any other screen
  transition already in the app, versus Design 2's grid which may redraw more
  cells whenever any one of several values changes.

**Cons**
- Auto-rotation causes a full-panel redraw every rotation interval regardless
  of whether the displayed value changed — on the two e-ink boards
  (`lilygo_techo`, `ThinkNode_M1`, no partial refresh, real per-refresh
  flicker/latency cost per `ARCHITECTURE.md`) this is the same category of
  problem the existing marquee-scroll-disable-on-e-ink precedent
  (`StatusBar.cpp`'s `_is_eink` branch) already exists to avoid. Recommend
  auto-rotate defaults to **off** on e-ink builds, manual-paging only —
  mirroring that existing precedent rather than introducing a new
  e-ink-unfriendly redraw source.
- Fewest stats visible at once of the three designs (exactly one, versus
  Design 2's four or Design 1's scrollable list) — seeing "the one thing you
  care about" requires either waiting for auto-rotate to cycle to it or
  manually paging there, unlike Design 1 or 2 where several stats are visible
  immediately.
- The header-vs-carousel focus contention (see Interaction) is unresolved and
  is a real open design/input problem, not just an implementation detail —
  worth settling before committing to this design over the other two.
- New code needed: large-text stat rendering, page-dot indicator, wraparound
  paging state, and an auto-rotate timer with pause-on-interaction logic —
  none of it reuses `MenuScreen`, similar implementation cost to Design 2.
