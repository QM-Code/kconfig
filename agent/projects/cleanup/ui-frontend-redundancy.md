# Cleanup S7 (`CLN-S7`): UI Frontend Redundancy Reduction

## Project Snapshot
- Current owner: `overseer`
- Status: `deferred/on hold (UI integration lane intentionally late-stage)`
- Immediate next task: capture extraction candidates for backend-neutral panel logic without enabling `m-bz3/src/ui/*` runtime wiring yet.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`.

## Mission
Reduce duplicated UI business logic across ImGui/RmlUi frontends by extracting backend-neutral presenters/controllers once UI integration is unparked.

## Foundation References
- `projects/cleanup.md`
- `projects/ui.md`
- `projects/ui/bz3.md`
- `m-bz3/src/ui/frontends/imgui/*`
- `m-bz3/src/ui/frontends/rmlui/*`

## Why This Is Separate
UI frontend dedupe is meaningful but intentionally deferred to avoid competing with current non-UI cleanup priorities.

## Owned Paths
- `m-bz3/src/ui/frontends/*`
- `m-bz3/src/ui/controllers/*`
- `m-overseer/agent/projects/cleanup/ui-frontend-redundancy.md`

## Interface Boundaries
- Inputs consumed:
  - UI integration sequencing from `projects/ui.md`.
- Outputs exposed:
  - extraction plan and eventual shared frontend-neutral logic modules.
- Coordinate before changing:
  - `projects/ui.md`
  - `projects/ui/bz3.md`
  - `projects/cleanup/ui-orphaned-subsystem.md`

## Non-Goals
- Do not wire orphaned UI subtree into runtime while this track is parked.
- Do not force one-backend UI behavior.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

## Trace Channels
- `cleanup.s7`
- `ui.system`

## Build/Run Commands
```bash
# when unparked
cd m-bz3
./abuild.py -c -d <bz3-build-dir> -b bgfx,diligent,imgui,rmlui
```

## Current Status
- `2026-02-21`: explicitly deferred by operator direction.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- Which panel/controller seams give best dedupe payoff with minimal migration risk?
- Should shared logic be hosted under `src/ui/controllers` or a new frontend-neutral layer?

## Handoff Checklist
- [ ] Extraction candidate list finalized.
- [ ] Parking status updated when unblocked.
- [ ] Any implementation slices validated across both UI backends.
