# Network Backend Encapsulation

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued (P1 medium-high; ready to assign)`
- Immediate next task: execute Slice 1 (docs+inventory) to map all remaining `ENet` usage under `src/game/*` and produce a no-regression migration plan to engine-owned contracts.
- Validation gate: `./bzbuild.py -c` + `./scripts/test-server-net.sh <build-dir>` must pass in both assigned build dirs for code-touching slices; docs lint must pass on every slice.

## Mission
Complete network backend encapsulation so game code consumes only `karma::network` contracts and never directly references `ENet` APIs/types/includes/link wiring.

## Why This Is Separate
Runtime transport ownership has mostly moved into engine contracts; this track finishes boundary cleanup (including tests/build wiring) without changing gameplay or protocol semantics.

## Priority Directive (2026-02-11)
- This is medium-high priority (P1) and should run immediately after active P0 network slice continuity is preserved.
- Target state:
  - `src/game/*` has zero direct `ENet` API/type/include usage,
  - `ENet` remains an engine-internal backend implementation selected via transport contracts.

## Owned Paths
- `docs/projects/network-backend-encapsulation.md`
- `m-rewrite/src/game/CMakeLists.txt`
- `m-rewrite/src/game/server/net/*`
- `m-rewrite/src/game/client/net/*`
- `m-rewrite/src/game/tests/*` (network-related tests)
- `m-rewrite/src/engine/network/*` (only when required for interface/test harness support)
- `docs/projects/ASSIGNMENTS.md`

## Interface Boundaries
- Engine owns:
  - transport backend implementation details (`ENet` and future backend adapters),
  - backend registration/selection and backend-facing lifecycle behavior,
  - generic network test fixtures/harness utilities.
- Game owns:
  - protocol schema and payload semantics,
  - gameplay interpretation and rules.
- Coordinate before changing:
  - `docs/projects/engine-network-foundation.md`
  - `docs/projects/server-network.md`
  - `docs/projects/gameplay-netcode.md`

## Non-Goals
- Do not change `src/game/protos/messages.proto`.
- Do not change `src/game/net/protocol.hpp` or `src/game/net/protocol_codec.cpp`.
- Do not change gameplay semantics in `src/game/*`.
- Do not introduce a new runtime transport backend unless explicitly scoped by a follow-on slice.

## Current Candidate Cleanup Targets
1. Remove direct `ENet` usage from game-side integration tests under `src/game/tests/enet_*` by moving backend-specific fixture behavior into engine test support and/or backend-agnostic contract tests.
2. Remove direct `ENet` link wiring from `src/game/CMakeLists.txt` where game targets should only depend on engine contracts.
3. Ensure game server/client runtime paths remain contract-only (`CreateServerTransport`/`CreateClientTransport`) with no backend includes/types.
4. Keep backend selection config (`network.*TransportBackend`) game-consumable while backend implementation remains engine-internal.

## Validation
From `m-rewrite/`:

```bash
./bzbuild.py -c build-sdl3-bgfx-jolt-rmlui-miniaudio
./bzbuild.py -c build-sdl3-bgfx-physx-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-jolt-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-physx-rmlui-miniaudio
```

Docs lint (from repository root):

```bash
./docs/scripts/lint-project-docs.sh
```

## Trace Channels
- `net.client`
- `net.server`
- `engine.server`

## Build/Run Commands
From `m-rewrite/`:

```bash
./bzbuild.py -c build-sdl3-bgfx-jolt-rmlui-miniaudio
./bzbuild.py -c build-sdl3-bgfx-physx-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-jolt-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-physx-rmlui-miniaudio
```

## First Session Checklist
1. Read `AGENTS.md`, then `docs/AGENTS.md`, then this file.
2. Confirm boundary goal: zero direct `ENet` usage under `src/game/*`.
3. Execute exactly one slice from the queue below.
4. Run required validation.
5. Update this file and `docs/projects/ASSIGNMENTS.md` in the same handoff.

## Slice Queue
1. Slice 1 (docs+inventory only):
   - enumerate all remaining direct `ENet` references under `src/game/*`,
   - classify each as remove/migrate/retain-with-rationale,
   - define exact slice plan with acceptance gates.
   - Status: `Queued`.
2. Slice 2 (game test/backend fixture migration):
   - migrate game ENet loopback tests toward engine-owned test-support contracts,
   - remove direct ENet includes/calls from migrated game tests.
   - Status: `Queued`.
3. Slice 3 (game CMake decoupling):
   - remove direct ENet link dependencies from game runtime targets where no longer required,
   - keep engine-owned network targets responsible for backend linkage.
   - Status: `Queued`.
4. Slice 4 (closure and guardrails):
   - add/strengthen checks so new direct backend usage does not re-enter `src/game/*`.
   - Status: `Queued`.

## Current Status
- `2026-02-11`: Project created at medium-high priority to finish ENet encapsulation and remove backend leakage from game-side code/tests/build wiring.

## Open Questions
- Which legacy game-side ENet tests should be retired vs rewritten as backend-agnostic contract/integration tests?
- Should we add a hard CI grep gate for `ENet` symbols under `src/game/*` after migration completion?
- Which future backend should be first candidate after encapsulation completeness (`SteamNetworkingSockets`, QUIC, or none until product need)?

## Handoff Checklist
- [ ] Slice scope completed
- [ ] No gameplay/protocol semantic drift
- [ ] Required validation run and summarized
- [ ] Project doc updated
- [ ] `docs/projects/ASSIGNMENTS.md` updated
- [ ] Risks/open questions listed
