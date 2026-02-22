# Cleanup S1 (`CLN-S1`): UI Orphaned Subsystem Decision

## Project Snapshot
- Current owner: `overseer`
- Status: `deferred/on hold (operator-directed late-stage integration)`
- Immediate next task: define explicit unpark criteria (integrate vs archive decision gate) and required evidence checklist.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`.

## Mission
Resolve the long-term disposition of `m-bz3/src/ui/*` as either an integrated runtime path or archived legacy intake, with one explicit decision and no ambiguous middle state.

## Foundation References
- `projects/cleanup.md`
- `projects/ui.md`
- `projects/ui/bz3.md`
- `m-bz3/cmake/game/sources.cmake`
- `m-bz3/src/client/game/lifecycle.cpp`

## Why This Is Separate
This is a high-impact architectural decision that changes build/runtime topology and should not block ongoing cleanup slices.

## Owned Paths
- `m-bz3/src/ui/*`
- `m-bz3/cmake/game/*` (when integration decision is active)
- `m-overseer/agent/projects/cleanup/ui-orphaned-subsystem.md`

## Interface Boundaries
- Inputs consumed:
  - UI contract direction from `projects/ui.md` and children.
- Outputs exposed:
  - binary decision (`Integrate` or `Archive`) plus scope/acceptance gate.
- Coordinate before changing:
  - `projects/ui.md`
  - `projects/cleanup.md`

## Non-Goals
- Do not perform full UI refactor in this track.
- Do not execute partial wiring without explicit disposition decision.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

## Trace Channels
- `cleanup.s1`
- `ui.system`

## Build/Run Commands
```bash
# only if/when unparked
cd m-bz3
./abuild.py -c -d <bz3-build-dir>
```

## Current Status
- `2026-02-21`: parked by operator direction for late-stage work.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- What exact trigger unblocks this track (`CLN-S2..S10` completion threshold vs UI superproject milestone)?
- If archived, what minimum docs remain as migration references?

## Handoff Checklist
- [ ] Integrate/archive decision explicitly recorded.
- [ ] Decision propagated to parent cleanup and UI docs.
- [ ] Any wiring changes validated if integration path selected.
