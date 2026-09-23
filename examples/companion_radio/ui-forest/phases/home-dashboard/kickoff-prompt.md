# Kickoff prompt — implement the Forest UI home dashboard

Self-contained prompt for starting the build. Paste this into a fresh session
(it doesn't assume any prior conversation context) or use it to kick off an
agent/task for the work.

---

You're implementing a planned feature in `examples/companion_radio/ui-forest/`
— the "Forest UI," a PlatformIO/Arduino C++ firmware UI for MeshCore (a
portable multi-hop LoRa mesh routing library). Read `CLAUDE.md` at the repo
root first for project-wide conventions (build commands, testing, repo
layout, contribution style).

## Read these three planning docs in full, in this order, before writing any code

1. `examples/companion_radio/ui-forest/phases/home-dashboard/plan.md` —
   background and decisions (status bar redesign, legacy-screen retirement,
   ambient notification blink). All open questions in it are answered — it's
   not still under discussion.
2. `examples/companion_radio/ui-forest/phases/home-dashboard/design-4-icon-tiles.md`
   — the chosen home-screen design: an 8-tile icon grid (GPS, Buzzer,
   Contacts, Channels, Signal, Diagnostics, Settings, Advert) with a shared
   caption line.
3. `examples/companion_radio/ui-forest/phases/home-dashboard/implementation-plan.md`
   — **this is your primary spec.** It sequences the actual code changes into
   6 phases, each grounded against a real read of the current source (exact
   file paths, function signatures, line numbers). Follow it phase by phase,
   in the order given — each phase depends on the one before it, this isn't
   an arbitrary ordering.

Also skim `examples/companion_radio/ui-forest/ARCHITECTURE.md` for the
existing screen/nav/widget conventions (`UIScreen`, `NavStack`, `MenuScreen`,
`FormField`, `Layout`, `icons.h`) so new code matches what's already there
instead of inventing a different style.

## Work order and checkpoints

- **Phase 1 (new icon assets)**: implement, then stop and report back before
  continuing. New hand-authored XBM bitmaps are easy to get subtly wrong
  (byte order, dimensions, MSB-first convention `icons.h` already uses) and
  are worth a sanity check before every later phase starts drawing them.
- **Phases 2-3 (status bar rewrite, new `Screen_HomeDashboard`)**: proceed
  once Phase 1 is confirmed. Compile-check after each phase (see below)
  rather than waiting until the end to discover a build break.
- **Phase 4 (retire legacy screens into Settings/Diagnostics)**: stop and
  report back again before continuing — this phase deletes/merges existing
  screens (`Screen_Status`, possibly `Screen_RadioInfo`), which is harder to
  reverse than the additive work in earlier phases. Confirm the
  `Screen_RadioInfo` vs. `Screen_DiagRadio` field diff (implementation-plan.md
  names this explicitly) before deleting anything.
- **Phases 5-6 (ambient notification blink, cleanup/verification)**: proceed
  once Phase 4 is confirmed, then report the final result.

## Verification

There's no native test coverage for this code — `pio test -e native` only
compiles `src/Utils.cpp`/`src/Packet.cpp`, never anything under `examples/`.
Compile-checking is the only automated signal available, so after each phase
build at least one env per `DisplayDriver` class named in
implementation-plan.md's Phase 6:
```
pio run -e RAK_4631_companion_radio_ble       # 128x64 mono OLED (dominant target)
pio run -e lilygo_techo_companion_radio_ble    # 128x128 e-ink
pio run -e heltec_rc32_companion_radio_ble     # color
```
(confirm these exact env names with `pio project config | grep 'env:'` first —
they're representative examples, not guaranteed to be the exact strings).

## Conventions to follow (from `CLAUDE.md`)

- 2-space indentation; match the brace/indent style of whichever file you're
  editing rather than reformatting it wholesale.
- No dynamic memory allocation outside setup/begin — this codebase's screens
  are all single, statically-allocated instances built once in
  `UITask::begin()`; new code should follow that pattern, not `new` things
  per-frame.
- Avoid unnecessary abstraction layers — implementation-plan.md's `Screen_HomeDashboard`
  sketch is deliberately plain (hardcoded 8-tile array, not a generic
  configurable-tile-count system) since that's all this feature needs.
- Don't touch anything outside `examples/companion_radio/ui-forest/` unless
  implementation-plan.md explicitly calls for it. It concluded no new
  hardware/`src/` plumbing is required — everything needed already exists one
  function call away.
- Stay on the current branch; don't create commits unless asked.

## Known open items to watch for

- Phase 3.4: the Signal tile's `signal_bars` icons are flagged (in `icons.h`
  itself) as unverified against real hardware RSSI sentinel values per radio
  backend — ship the safe static-placeholder fallback implementation-plan.md
  describes unless that verification has happened in the meantime.
- Phase 3.6: grep for the actual `toggleGPS()`/`toggleBuzzer()`-equivalent
  call sites before wiring the dashboard's inline toggles — implementation-plan.md
  deliberately didn't nail down the exact function names/locations, to avoid
  asserting something not directly confirmed. Reuse whatever `Screen_Gps`/the
  buzzer settings row already calls; don't introduce a second toggle code
  path.
- If anything in the real source has drifted from what implementation-plan.md
  assumed (a renamed function, a moved file), flag it and adjust rather than
  silently forcing the plan's exact wording to fit.
