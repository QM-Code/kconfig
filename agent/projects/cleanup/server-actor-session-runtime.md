# Cleanup S2 (`CLN-S2`): Server Actor/Session Runtime Cleanup

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (highest-value active execution lane)`
- Immediate next task: remove non-parity actor tick drift/health decay and introduce O(1) session->actor indexing in server runtime lookup paths.
- Validation gate: `cd m-bz3 && ./abuild.py -c -d <bz3-build-dir> && ./scripts/test-server-net.sh <bz3-build-dir>` plus targeted `ctest`.

## Mission
Normalize authoritative server runtime behavior by removing synthetic drift logic and replacing repeated actor lookup scans with indexed session mappings.

## Foundation References
- `projects/cleanup.md`
- `m-bz3/src/server/domain/actor_system.cpp`
- `m-bz3/src/server/server_game.cpp`
- `m-bz3/src/server/runtime/run.cpp`

## Why This Is Separate
This slice is runtime-critical and independent from renderer/UI/factory cleanup work, making it ideal as the first high-value implementation lane.

## Owned Paths
- `m-bz3/src/server/domain/*`
- `m-bz3/src/server/runtime/*`
- `m-bz3/src/server/server_game.cpp`
- `m-overseer/agent/projects/cleanup/server-actor-session-runtime.md`

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
./abuild.py -c -d <bz3-build-dir>
./scripts/test-server-net.sh <bz3-build-dir>
ctest --test-dir <bz3-build-dir> -R "server_runtime_.*|server_session_runtime_contract_test|server_join_runtime_contract_test" --output-on-failure
```

## Trace Channels
- `server.runtime`
- `server.actor`
- `cleanup.s2`

## Build/Run Commands
```bash
cd m-bz3
./abuild.py -c -d <bz3-build-dir>
```

## Current Status
- `2026-02-21`: selected as first active cleanup implementation slice.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- Should the session->actor index live in `ServerGame`, domain systems, or a shared runtime context?
- Which invariants should be contract-tested first for index correctness?

## Handoff Checklist
- [ ] Synthetic drift/decay removed from authoritative tick path.
- [ ] Session->actor indexing landed with deterministic update/remove semantics.
- [ ] Server runtime contract tests pass and are recorded.
