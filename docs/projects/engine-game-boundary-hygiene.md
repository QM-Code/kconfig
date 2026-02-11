# Engine/Game Boundary Hygiene

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued (P2 low-medium; ready to assign)`
- Immediate next task: execute Slice 1 (docs-only inventory + extraction plan) for game-side boilerplate candidates that can move into engine-owned helpers.
- Validation gate: docs lint must pass on every slice; code-touching slices must pass assigned `bzbuild.py` + wrapper gates.

## Mission
Reduce game-side boilerplate/redundant infrastructure code by moving game-agnostic behavior into engine-owned contracts, while keeping gameplay rules and protocol payload semantics game-owned.

## Why This Is Separate
This is boundary-hygiene work that can be delivered in narrow slices without changing gameplay semantics. It directly supports rewrite goals by shrinking repetitive game scaffolding and strengthening engine ownership.

## Priority Directive (2026-02-11)
- This project is low-medium priority (P2) and should run behind active P0 tracks (`renderer-parity`, `engine-network-foundation`).
- Apply default-first policy: move generic behavior into engine only when semantics remain contract-safe.
- Preserve strict ownership boundaries:
  - engine owns lifecycle/bootstrap/config scaffolding helpers,
  - game owns BZ3 rules, protocol payload semantics, and content decisions.

## Owned Paths
- `docs/projects/engine-game-boundary-hygiene.md`
- Candidate integration points:
  - `m-rewrite/src/game/client/bootstrap.cpp`
  - `m-rewrite/src/game/server/bootstrap.cpp`
  - `m-rewrite/src/game/client/cli_options.cpp`
  - `m-rewrite/src/game/server/cli_options.cpp`
  - `m-rewrite/src/game/client/net/client_connection.cpp`
  - `m-rewrite/src/game/server/net/enet_event_source.cpp`
  - `m-rewrite/src/game/server/runtime.cpp`
- Future engine-owned landing zones (to be decided in-slice):
  - `m-rewrite/src/engine/*`
  - `m-rewrite/include/karma/*`

## Interface Boundaries
- Allowed engine extraction:
  - bootstrap/config validation scaffolding,
  - shared CLI parsing scaffolding,
  - transport backend selection/config mapping helpers (not protocol behavior),
  - generic reconnect policy config mapping helpers.
- Must remain game-owned:
  - protocol schema/payload semantics,
  - gameplay rules/state semantics,
  - game-specific message handling.
- Coordinate before changing:
  - `docs/projects/engine-network-foundation.md`
  - `docs/projects/core-engine-infrastructure.md`
  - `docs/projects/testing-ci-docs.md`

## Non-Goals
- Do not change `src/game/protos/messages.proto`.
- Do not change `src/game/net/protocol.hpp` or `src/game/net/protocol_codec.cpp`.
- Do not move gameplay semantics into engine systems.
- Do not widen scope into renderer/physics/audio feature work.

## Candidate Inventory (Initial Note)
1. Duplicate bootstrap setup paths in client/server (`src/game/client/bootstrap.cpp`, `src/game/server/bootstrap.cpp`).
2. Duplicate CLI scaffolding in client/server (`src/game/client/cli_options.cpp`, `src/game/server/cli_options.cpp`).
3. Repeated transport-backend config parse/plumbing (`src/game/client/net/client_connection.cpp`, `src/game/server/net/enet_event_source.cpp`).
4. Repeated strict required-config validation/error policy (`src/game/client/bootstrap.cpp`, `src/game/server/runtime.cpp`).
5. Client reconnect policy config mapping that may fit an engine default helper (`src/game/client/net/client_connection.cpp`).
6. Lower-confidence candidate: world/package transfer assembler in `src/game/client/net/client_connection.cpp` (protocol-sensitive; likely game-owned unless proven generic).

## Validation
Docs-only slices (from repository root):

```bash
./docs/scripts/lint-project-docs.sh
```

Code-touching slices (from `m-rewrite/`):

```bash
./bzbuild.py -c build-sdl3-bgfx-jolt-rmlui-miniaudio
./bzbuild.py -c build-sdl3-bgfx-physx-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-jolt-rmlui-miniaudio
./scripts/test-server-net.sh build-sdl3-bgfx-physx-rmlui-miniaudio
```

## Trace Channels
- `engine.app`
- `config`
- `net.client`
- `net.server`

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
2. Confirm boundary constraints (engine scaffolding only, no gameplay/protocol semantic movement).
3. Execute exactly one slice from the queue below.
4. Run required validation.
5. Update this file and `docs/projects/ASSIGNMENTS.md` in the same handoff.

## Current Status
- `2026-02-11`: Project created and queued at low-medium priority.
- `2026-02-11`: Initial candidate inventory captured from current game-side duplication hotspots.

## Slice Queue
1. Slice 1 (docs-only inventory + extraction plan):
   - convert candidate list into keep-in-game vs extract-to-engine decisions,
   - define target engine contracts/helpers and acceptance criteria.
   - Status: `Queued`.
2. Slice 2 (bootstrap/config scaffolding extraction):
   - extract shared bootstrap/config-validation helper path,
   - keep strict/non-strict behavior and message semantics unchanged.
   - Status: `Queued`.
3. Slice 3 (network config mapping extraction):
   - extract transport backend and reconnect config mapping helpers,
   - keep protocol and runtime transport semantics unchanged.
   - Status: `Queued`.
4. Slice 4 (CLI scaffolding extraction):
   - extract common CLI option parsing scaffolding,
   - keep client/server option semantics unchanged.
   - Status: `Queued`.

## Open Questions
- What is the preferred engine-owned namespace/path for bootstrap + CLI shared helpers?
- Should backend config-key names remain game-owned while parser/mapping logic moves to engine?
- Is there enough stable shared behavior to extract world/package assembly helpers without leaking protocol semantics into engine?

## Handoff Checklist
- [ ] Slice scope completed
- [ ] Boundary constraints preserved (no gameplay/protocol semantic drift)
- [ ] Validation run and summarized
- [ ] This file updated
- [ ] `docs/projects/ASSIGNMENTS.md` updated
- [ ] Risks/open questions listed
