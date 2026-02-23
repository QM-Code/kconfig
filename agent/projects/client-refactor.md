# Client Topology Refactor (A): Remove `src/client/game` Layer

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued (specialist-ready)`
- Immediate next task: dispatch `CR-A1` to produce a concrete move map that removes `src/client/game/*` and stages updates in behavior-neutral slices.
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-bz3`: `./abuild.py -c -d <bz3-build-dir> --karma-sdk ../m-karma/out/karma-sdk`

## Mission
Eliminate the client-only `src/client/game` directory layer so client/server source topology is consistently role-based (`domain`, `runtime`, `net`) and no special client-only app bucket remains.

## Foundation References
- `m-overseer/agent/docs/building.md`
- `m-overseer/agent/docs/testing.md`
- `m-bz3/src/client/game/game.hpp`
- `m-bz3/src/client/runtime/*`
- `m-bz3/src/client/domain/*`
- `m-bz3/src/server/runtime/server_game.hpp`
- `m-bz3/cmake/targets/*`

## Why This Is Separate
This is a broad structural migration across CMake paths, include paths, tests, and docs. Isolating it prevents runtime-behavior tracks from absorbing high-churn file movement.

## Scope Baseline (at project start)
- `src/client/game` include-path references: `~93`
- `namespace bz3::client::game` references: `~43`
- `src/client/game/*` CMake path references: `~30`

## Owned Paths
- `m-bz3/src/client/*`
- `m-bz3/cmake/targets/*`
- `m-bz3/src/tests/*` (as needed for include/path updates)
- `m-overseer/agent/projects/client-refactor.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`

## Interface Boundaries
- Inputs consumed:
  - current client/server runtime contracts and existing test expectations
- Outputs exposed:
  - client source topology contract with no `src/client/game/*` subtree
  - updated include/CMake path contracts for all client-related targets/tests
- Coordinate before changing:
  - `m-overseer/agent/projects/fix-gameplay.md`

## Non-Goals
- Do not change gameplay behavior, net protocol semantics, or server authority rules as part of path migration.
- Do not combine this migration with broad server-side directory rework.
- Do not mix style-only rewrites with structural moves in the same slice.

## Execution Plan
### `CR-A1` Inventory + Move Map
- Produce exact move table from `src/client/game/*` into role-based destinations (runtime/domain and supporting subtrees).
- Identify compatibility shims needed for staged migration.

### `CR-A2` Runtime-Orchestration Relocation
- Move orchestration entrypoints currently centered around `game.hpp`/`lifecycle.cpp` into stable runtime-owned locations.
- Keep behavior identical; only path/layout changes.

### `CR-A3` Domain/Mechanics Relocation
- Move tank/collision/pilot/mechanics units out of `src/client/game/*` into domain-owned locations.
- Update include paths and local wiring incrementally.

### `CR-A4` CMake/Test/Doc Convergence
- Update all CMake source lists and test wiring to new paths.
- Remove any stale `src/client/game/*` references in docs/scripts.

### `CR-A5` Optional Namespace Convergence
- If approved by operator, migrate `bz3::client::game::*` namespaces to topology-aligned names.
- Keep this as a separate pass after path migration is green.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh

cd ../m-bz3
export ABUILD_AGENT_NAME=<agent-name>
./abuild.py --claim-lock -d <bz3-build-dir>
./abuild.py -c -d <bz3-build-dir> --karma-sdk ../m-karma/out/karma-sdk
ctest --test-dir <bz3-build-dir> -R "tank_.*|client_.*|server_.*runtime.*" --output-on-failure
./abuild.py --release-lock -d <bz3-build-dir>
```

## Trace Channels
- `refactor.client.layout`
- `refactor.client.includes`
- `refactor.client.cmake`

## Build/Run Commands
```bash
cd m-bz3
./abuild.py -c -d <bz3-build-dir> --karma-sdk ../m-karma/out/karma-sdk
```

## Current Status
- `2026-02-23`: project doc created for specialist delegation; awaiting first implementation packet (`CR-A1`).

## Open Questions
- Should namespace migration (`bz3::client::game::*`) be mandatory in this track, or deferred until path migration is complete?
- Should runtime orchestration keep a `Game` type name or adopt a more topology-neutral name once paths are stabilized?

## Handoff Checklist
- [ ] Move map approved
- [ ] `src/client/game/*` removed
- [ ] CMake/test wiring updated
- [ ] Validation commands run and summarized
- [ ] Remaining naming debt and risks documented
