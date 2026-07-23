# ui-forest — migration/deprecation decision (follow-up)

Not part of the `ui-forest` build plan's code deliverables. `PLAN.md` §1's non-goals and
`phases/completed/phase-8-hardening-rollout.md` both explicitly scope "decide to make `ui-forest` any
board's default" out of the plan itself, as a separate follow-up task for whoever is
responsible for that call. This document lays out the tradeoffs; it does not make the call.

## The question

For which boards, if any, should `ui-forest` replace `ui-new`/`ui-orig`/`ui-tiny` as the
*default* `companion_radio` UI (the env without `_forest` in its name), and on what timeline?

This is strictly about which env is the default. Nothing here proposes deleting `ui-new`/
`ui-orig`/`ui-tiny` — `PLAN.md` §1 rules that out regardless of what's decided below, and
`ui-forest` stays available as an opt-in `_forest` env on every board either way.

## Where things actually stand (facts, not opinion)

- **8 boards have a `_forest` env today**: `RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`,
  `WioTrackerL1`, `lilygo_techo`, `lilygo_tdeck`, `sensecap_indicator-espnow`, plus `ThinkNode_M1`
  as of this phase. None of these is currently anyone's default — all are additive,
  opt-in envs alongside each board's existing `ui-new`/`ui-orig`/`ui-tiny` env(s).
- **Exactly one board has any real hardware track record: `WioTrackerL1`.** Every phase from 0
  through 4 got at least an informal flash-and-poke pass on it from the user; Phase 3 additionally
  got radio-param live-apply and reboot-survival specifically confirmed. Phases 5, 6 (its own
  regression-only slice), 7, and 8 have **not** been flashed by anyone at all as of this writing.
- **No board has ever had a formal checklist run.** Every phase defines a manual test checklist
  (`PLAN.md` §9); none has been filled in and recorded for any board on any phase. All hardware
  confirmation so far has been informal ("works great", a bug report, a specific claim the user
  happened to check).
- **`RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32`** — the three boards `PLAN.md` actually named as
  pilots — **have zero hardware passes across all 8 phases.** Everything built for them is
  build-unverified-or-hand-traced only.
- **`lilygo_tdeck`/`sensecap_indicator-espnow`** (Phase 6) and **`lilygo_techo`** (Phase 5, e-ink)
  and **`ThinkNode_M1`** (Phase 8, e-ink) have never been flashed either, and nobody currently has
  hardware for the first three of those four.
- **Known, documented, currently-open gaps**, several with real user-facing consequences if this
  became a daily-driver default UI:
  - No persisted "vibration enabled" setting (Phase 3) and no persisted per-event-type
    notification config (Phase 7) — both reset to a fixed default every reboot, because adding
    them means hand-editing `NodePrefs`' unversioned binary format with no compiler available to
    verify the change.
  - The message-preview screen can flash back to Home almost immediately if a phone app is
    connected and syncs the message first (Phase 4 finding, not yet addressed).
  - `ConfirmScreen`'s auto-fire-after-N-seconds design (used for shutdown, erase, rekey, reboot,
    every "restore defaults") has never been validated as the right UX on real hardware, despite
    gating every irreversible action in the UI.
  - The `KEY_HOME` triple-click mechanism (the only way single-button/analog/rotary boards escape
    a list screen) has never been button-mashed on real hardware.
  - Diagnostics values have only been checked by reading the same wire-protocol code the companion
    app parses, never by an actual side-by-side comparison against the app.
  - Touch (SenseCAP) and keyboard/trackball (T-Deck) input are reasoned through from source code,
    not finger-tested — nobody currently has that hardware.
- **Feature scope, by contrast, is genuinely ahead of `ui-new`.** `ui-forest` is a real menu
  system (not swipe-pages), has on-device Settings wired to live `NodePrefs`, a Diagnostics/"nerd
  stats" area, full contact/channel browsing, and per-event notification config — none of which
  `ui-new`/`ui-orig`/`ui-tiny` have. If it's device-confirmed correct, it is a strictly better UI
  for boards that can run it.

## Options (not a recommendation)

These aren't mutually exclusive in the strict sense — a real decision could pick different
answers per board — but they represent distinct postures:

1. **No default change, indefinitely.** Keep every `_forest` env opt-in on every board until a
   formal checklist pass (`PLAN.md` §9) has actually happened on that specific board. Lowest risk,
   but means `ui-forest`'s feature advantage never reaches a user who doesn't already know to seek
   out the `_forest` env, and there's no forcing function to ever get the hardware-verification
   work done.
2. **Default only where there's an actual hardware track record.** Today that's `WioTrackerL1`
   alone, and even that's informal passes through Phase 4 with nothing at all past it. Smallest
   defensible scope, but `WioTrackerL1` becoming the first (and for now only) board with
   `ui-forest` as default is arguably a proxy for "whichever board the person building this
   happened to own," not a judgment about which boards are actually ready.
3. **Default across the three named `PLAN.md` pilots once each gets a first real hardware pass.**
   Matches the trust model the project has followed everywhere else (build → hand-trace → device
   pass → trust), extended to the "which env is default" question. Requires someone to actually do
   that verification work on `RAK_4631`/`gat562_30s_mesh_kit`/`heltec_rc32` first — a resourcing
   question as much as a code one.
4. **Default across all 8 boards with a `_forest` env today, on a fixed timeline (e.g. "next
   major release"), independent of hardware-verification status.** Fastest path to actually
   retiring `ui-new` as the thing most users see, but directly at odds with the "trace carefully,
   don't assume, get real hardware confirmation before trusting it" discipline this project has
   held to for every other decision across all 8 phases — and this is firmware people flash to
   real, sometimes unattended mesh nodes, not a web app that can hotfix a bad default.

## What's needed regardless of which option is picked

- At least one formal `PLAN.md` §9 checklist run, recorded, per board that becomes a default —
  not another round of informal "flashed it, seems fine" passes.
- A resolution for the two non-persisted-settings gaps (vibration, per-event notification config)
  if either board's default users would actually rely on them surviving a reboot.
- Someone with a compiler/PlatformIO toolchain available at all — every phase of this build was
  done in a dev environment with no `pio`/`g++` on PATH, so literally nothing described in
  `PROGRESS.md` has been compiler-checked, only hand-traced.

## The ask

This document intentionally stops short of recommending one of the four options (or some other
option not listed here) — see this file's own framing above and `PLAN.md` §1/phase-8's explicit
instruction that this call belongs to whoever owns that decision, not to whoever happened to build
the feature. Whenever you're ready to make this call, flag it and it can be discussed against
whichever of the above (or a variant) fits your actual rollout plans and risk tolerance.
