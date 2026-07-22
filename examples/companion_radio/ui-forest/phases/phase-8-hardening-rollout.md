# Phase 8 — Hardening & rollout

> Part of the `ui-forest` build plan. Read `../PLAN.md` first. Confirm Phase 7
> (`phase-7-notifications.md`) passed before starting. This is the last phase in the plan —
> its output is board-coverage widening plus a **written follow-up decision**, not new UI
> features.

## Goal

Widen board coverage beyond the 5 pilots used in Phases 0–6, fix whatever board-specific
breakage shows up, and produce the actual migration/deprecation decision for `ui-new` as a
separate follow-up document (not made as part of this plan — PLAN.md §1 non-goals explicitly
excludes deciding to make `ui-forest` any board's default from this plan's scope).

## Prerequisites (from Phases 0–7)

Every phase 0–7 checklist has passed on its respective pilot boards
(`RAK_4631`, `gat562_30s_mesh_kit`, `heltec_rc32`, `lilygo_tdeck`,
`sensecap_indicator-espnow`, and the e-ink board chosen in Phase 5). The full feature set from
PLAN.md §7 (items 1–36) is built and working on that pilot set.

## What to build

1. **Add `_forest` envs to a broader board sample** — at minimum one more board per display
   backend not yet covered by a pilot:
   - A **GxEPD e-ink board** other than whichever was chosen in Phase 5 (`heltec_e213` /
     `lilygo_techo`) — e.g. the other one of that pair, or another `GxEPD`-based variant if one
     exists (grep `variants/*/platformio.ini` for `DISPLAY_CLASS=E213Display`/`E290Display` or
     similar GxEPD-backed classes to find candidates).
   - An **LGFX/LovyanGFX RGB-panel board** other than `sensecap_indicator-espnow` — grep for
     other variants using `helpers/ui/LGFXDisplay.cpp` in their `build_src_filter`.
   - Follow the same base-section-extends env pattern established in Phase 0 for each new
     board (mirror its existing `_ble`/`_usb` env, swap only the ui-forest include/filter
     lines).
2. **Fix board-specific breakage** — build and smoke-test each newly-added env; fix whatever
   doesn't compile or doesn't render correctly on that specific board's display/input
   combination. This is expected to surface edge cases the 5 original pilots didn't (different
   RAM budgets, different display driver quirks, different pin/input configs) — triage and fix
   per-board, without special-casing every board individually if a general fix in `Layout`/
   `DisplayDriver` usage covers the class of bug.
3. **Write the migration/deprecation decision as a follow-up, not part of this phase's
   deliverable** — a short doc note: for which boards (if any) does `ui-forest` become the
   default `companion_radio` UI, replacing `ui-new`/`ui-orig`/`ui-tiny`, and on what timeline?
   This is explicitly scoped as "its own follow-up task" (PLAN.md §6 Phase 8 "Done when") — do
   not fold default-UI-switching env changes into this phase's env additions; those stay
   additive/opt-in per PLAN.md §5, same as every earlier phase.

## Data / API dependencies

None new — this phase is board-coverage and bugfixing over the complete feature set from
Phases 0–7, not new functionality.

## Open questions relevant to this phase

None new from PLAN.md §8 — by this phase, every open question from that section should already
be resolved in the phase that raised it (advert-interval reschedule and advert-name validation
in Phase 3, direct-path counters and transport-type story in Phase 4, `build_src_filter`
recursion and env `extends` syntax in Phase 0, icon pipeline in Phase 5). If any surfaces as
still-unresolved when this phase starts, resolve it before widening coverage further — don't
propagate an unverified assumption to a dozen new boards at once.

## Done-when / manual test checklist

Run the standard template (PLAN.md §9) per newly-added board/env, plus:

```
[ ] Builds green across the widened board sample (pio run -e <env> for every new env added)
[ ] Each newly-added board's smoke test passes the standard checklist from PLAN.md §9
[ ] Any board-specific fix made in this phase is a general fix (in Layout/DisplayDriver usage,
    not a per-board special case) wherever the bug class allows it
[ ] A short migration/deprecation-decision doc is written as its own separate follow-up
    artifact — not a code change in this phase — covering: which boards (if any) get
    ui-forest as their default companion_radio UI, and on what timeline
[ ] All existing ui-new/ui-orig/ui-tiny envs remain byte-for-byte unchanged (verify via git
    diff on the variants/*/platformio.ini files touched this phase — only new [env:...]
    blocks should appear, no edits to existing ones)
```
