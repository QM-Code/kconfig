# Cleanup S4 (`CLN-S4`): Physics Sync Decomposition

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued`
- Immediate next task: carve `ecs_sync_system` into classification/reconcile/apply/trace units and define typed decision objects to replace dense bool-matrix flows.
- Validation gate: `cd m-karma && ./abuild.py -c -d <karma-build-dir>` plus targeted parity tests.

## Mission
Reduce complexity in `ecs_sync_system` by splitting concerns and replacing boolean-heavy classification APIs with explicit typed state/decision objects.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/physics/sync/ecs_sync_system.cpp`
- `m-karma/src/physics/sync/ecs_sync_system.hpp`

## Why This Is Separate
This is high-complexity physics-internal refactoring that can run in parallel with server runtime and factory standardization work.

## Owned Paths
- `m-karma/src/physics/sync/*`
- `m-overseer/agent/projects/cleanup/physics-sync-decomposition.md`

## Interface Boundaries
- Inputs consumed:
  - runtime command and ECS sync contracts from parity tests.
- Outputs exposed:
  - smaller cohesive modules and clearer decision surfaces.
- Coordinate before changing:
  - `projects/cleanup/physics-parity-suite.md`

## Non-Goals
- Do not alter high-level physics backend selection behavior.
- Do not collapse trace diagnostics needed by parity checks.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
ctest --test-dir <karma-build-dir> -R "physics_backend_parity_.*" --output-on-failure
```

## Trace Channels
- `physics.sync`
- `physics.sync.trace`
- `cleanup.s4`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
```

## Current Status
- `2026-02-21`: `ecs_sync_system` complexity identified as major `P1` target.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- What is the minimum typed decision model that preserves current trace classifier semantics?
- Which decomposition boundary minimizes merge conflict with parity test work?

## Handoff Checklist
- [ ] `ecs_sync_system` split into cohesive units.
- [ ] Boolean-matrix classifier entrypoints replaced by typed decisions.
- [ ] Physics parity tests remain green.
