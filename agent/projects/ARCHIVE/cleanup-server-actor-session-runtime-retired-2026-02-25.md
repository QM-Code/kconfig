# Cleanup S2 (`CLN-S2`): Server Actor/Session Runtime Cleanup

## Project Snapshot
- Current owner: `overseer`
- Status: `completed (archived; runtime cleanup objectives landed)`
- Immediate next task: none (archived record only).
- Validation gate: `cd m-bz3 && ./abuild.py -a <agent> -c -d <bz3-build-dir> -k ../m-karma/out/karma-sdk` plus canonical gameplay/targeted `ctest` regex gates.

## Mission
Normalize authoritative server runtime behavior by removing synthetic drift logic and replacing repeated actor lookup scans with indexed session mappings.

## Foundation References
- `projects/cleanup.md`
- `m-bz3/src/server/domain/actor_system.cpp`
- `m-bz3/src/server/runtime/server_game.cpp`
- `m-bz3/src/server/runtime/run.cpp`

## Why This Is Separate
This slice is runtime-critical and independent from renderer/UI/factory cleanup work, making it ideal as the first high-value implementation lane.

## Owned Paths
- `m-bz3/src/server/domain/*`
- `m-bz3/src/server/runtime/*`
- `m-bz3/src/server/runtime/server_game.cpp`
- `m-overseer/agent/projects/ARCHIVE/cleanup-server-actor-session-runtime-retired-2026-02-25.md`

## Interface Boundaries
- Inputs consumed:
  - server parity/runtime policy from gameplay and networking contracts.
- Outputs exposed:
  - deterministic server actor/session update semantics.
- Coordinate before changing:
  - `projects/cleanup.md`

## Non-Goals
- Do not rework client prediction/reconciliation in this slice.
- Do not alter transport protocol semantics.

## Validation
```bash
cd m-bz3
./abuild.py -a <agent> -c -d <bz3-build-dir> -k ../m-karma/out/karma-sdk
ctest --test-dir <bz3-build-dir> -R "server_net_contract_test|server_runtime_event_rules_test|server_runtime_shot_physics_integration_test|server_runtime_shot_damage_integration_test|server_runtime_shot_damage_idempotence_integration_test|server_runtime_shot_pilot_smoke_test|server_join_runtime_contract_test|server_session_runtime_contract_test|client_shot_reconciliation_test" --output-on-failure
ctest --test-dir <bz3-build-dir> -R "server_runtime_.*|server_session_runtime_contract_test|server_join_runtime_contract_test" --output-on-failure
```

## Trace Channels
- `server.runtime`
- `server.actor`
- `cleanup.s2`

## Build/Run Commands
```bash
cd m-bz3
./abuild.py -a <agent> -c -d <bz3-build-dir> -k ../m-karma/out/karma-sdk
```

## Current Status
- `2026-02-21`: selected as first active cleanup implementation slice.
- `2026-02-22`: moved under cleanup superproject child structure.
- `2026-02-25`: landed `ServerGame` O(1) session->actor lookup path (`session_entity_by_id_`, `actor_entity_by_session_id_`) and removed full-world actor scan from runtime hot paths (`findActorForSession`).
- `2026-02-25`: removed synthetic actor drift integration (`ActorSystem::onTick` no longer integrates `motion->position += motion->velocity * dt`) and set actor spawn velocity baseline to zero.
- `2026-02-25`: validation passed on `build-cln-s2`: canonical gameplay gate regex `9/9` pass, targeted runtime regex `8/8` pass.
- `2026-02-25`: observed tooling drift: `scripts/test-server-net.sh` still calls `abuild.py` without required `--agent/--karma-sdk`.
- `2026-02-25`: tooling drift fixed in `scripts/test-server-net.sh` by adding required `abuild.py` args (`--agent`, `--karma-sdk`) with lock-aware/default agent resolution; gate rerun passed.
- `2026-02-25`: follow-on landed: `SessionSystem::findSessionById` now uses indexed `session_entity_by_id_` (O(1)); redundant `ServerGame` session-id index removed to keep single source-of-truth in `SessionSystem`; gate rerun passed.
- `2026-02-25`: `CLN-S2` archived by overseer as `projects/ARCHIVE/cleanup-server-actor-session-runtime-retired-2026-02-25.md` after closeout confirmation.

## Open Questions
- None (closed).

## Handoff Checklist
- [x] Synthetic drift removed from authoritative tick path.
- [x] Session->actor indexing landed with deterministic update/remove semantics.
- [x] Server runtime contract tests pass and are recorded.
