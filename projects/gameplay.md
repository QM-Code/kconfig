# Gameplay Integration (Merged Track)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress` (merged planning baseline created; Phase 5 physics integration is complete and gameplay behavior parity migration remains)
- Supersedes:
  - `docs/archive/physics-refactor-retired-2026-02-19.md`
  - `docs/archive/gameplay-migration-retired-2026-02-19.md`
- Immediate next task: execute one bounded G0 re-baseline slice to codify D2 movement-replication seams (`PlayerLocation` transport + authoritative server actor updates) with concrete file-level implementation packet.
- Validation gate: planning/doc slices run `./docs/scripts/lint-project-docs.sh`; code slices run `./scripts/test-server-net.sh <build-dir>` and add `./scripts/test-engine-backends.sh <build-dir>` whenever physics/gameplay integration paths are touched.

## Mission
Port and adapt remaining `m-dev` gameplay behavior into `m-rewrite` using rewrite architecture boundaries:
- engine physics remains generic ECS/sync substrate,
- gameplay rules remain game-owned,
- behavior parity is preserved where intended, but implementation must be rewrite-native (not file-structure mirroring).

## Foundation References
- `docs/AGENTS.md`
- `docs/foundation/policy/execution-policy.md`
- `docs/foundation/architecture/core-engine-contracts.md`
- `docs/projects/ui-engine.md`
- `docs/projects/ui-game.md`

## Why This Is Separate
`physics-refactor` and `gameplay-migration` developed overlapping tracks (shots, authority, movement, scoring/round lifecycle dependencies). This merged track removes duplicated planning and aligns remaining behavior migration to the new rewrite physics foundation.

## Landed Baseline (Do Not Replan)
These are already implemented and should be treated as foundation, not open migration work:
- Phase 5 physics/game integration foundation complete (archived from `physics-refactor`):
  - client tank collision/motion now uses rewrite ECS physics intent flows,
  - server shot runtime has rewrite-owned shot lifecycle + physics-hit attribution/damage/death pipeline,
  - shot pilot guard/state/autotune/diagnostics/calibration seams and tests are in place.
- Gameplay D1 drivable baseline landed:
  - rewrite-local tank drive + camera baseline and hardening already present.
- UI tree import is already handled by `ui-engine.md` and `ui-game.md`; this project does not reopen that import track.

## Remaining Behavior Migration (From m-dev, Adapted to Rewrite)
1. D2 movement replication (high priority)
- Migrate behavior equivalent to m-dev authoritative player-location flow into rewrite server/client runtime:
  - `ClientMsg_PlayerLocation` ingest and validation,
  - authoritative server actor transform/velocity updates,
  - server->client player-location broadcast semantics,
  - client-side actor/world-state consumption path.
- Rewrite adaptation requirement:
  - integrate with rewrite ECS actor/session components and physics ownership contracts,
  - avoid reintroducing ad-hoc engine/game leakage.

2. D3 predicted-shot reconciliation
- Keep local responsiveness while preserving server authority:
  - local shot prediction tracking by `local_shot_id`,
  - reconciliation against authoritative `global_shot_id` lifecycle,
  - deterministic smoothing/telemetry for correction paths.

3. G4 scoring + scoreboard semantic migration
- Migrate authoritative score mutation semantics from m-dev server behavior into rewrite game domain.
- Feed rewrite UI scoreboard data through existing UI systems (without reopening UI-engine/UI-game scopes).

4. G5 round lifecycle + spawn/respawn control flow
- Migrate round start/end and spawn/respawn semantics into rewrite server gameplay domain systems.
- Keep spawn and death behavior authoritative and deterministic.

5. Physics carry-over (from archived `physics-refactor`)
- Only remaining carry-over is planning-only:
  - Phase 6 entry planning (bounded plan for higher-level parity/stabilization tests),
  - no new Phase 6 implementation in this track until a planning slice is accepted.

## Parity Gap Matrix (Current)
| Domain | m-dev behavior reference | rewrite current posture | Status |
|---|---|---|---|
| Shot lifecycle + hit attribution + damage | `m-dev/src/game/server/game.cpp`, `m-dev/src/game/server/shot.*` | rewrite-owned server shot domain/runtime + pilot hardening landed | `landed` |
| Client tank movement baseline | `m-dev/src/game/client/player.cpp` | rewrite tank drive/collision/motion authority seams landed | `landed` |
| PlayerLocation transport and authoritative movement replication | `m-dev/src/game/server/game.cpp:203`, `m-dev/src/game/server/client.cpp:51` | protobuf has messages, but rewrite protocol/runtime handlers for this behavior are not migrated | `pending (D2)` |
| Score mutation + scoreboard semantics | `m-dev/src/game/server/game.cpp:263`, `m-dev/src/game/server/client.cpp:94`, `m-dev/src/game/client/game.cpp:191` | UI score types exist; gameplay score feed/authority not yet migrated | `pending (G4)` |
| Round lifecycle/spawn/respawn | `m-dev/src/game/server/client.cpp`, `m-dev/src/game/server/world_session.cpp` | partial scaffolding only | `pending (G5)` |
| Predicted shot reconciliation | m-dev netcode behavior lane | local fire feedback exists; full prediction/reconcile loop not complete | `pending (D3)` |

## Owned Paths
- `docs/projects/gameplay.md`
- `docs/projects/ASSIGNMENTS.md`
- `src/game/*`
- `include/karma/*` and `src/engine/*` only when explicit seam changes are required and coordinated

## Interface Boundaries
- Inputs consumed (read-only references):
  - `../m-dev/src/game/client/*`
  - `../m-dev/src/game/server/*`
- Coordination files before changing:
  - `src/game/protos/messages.proto`
  - `src/game/net/protocol.hpp`
  - `src/game/net/protocol_codec/*`
  - `src/engine/CMakeLists.txt`
  - `docs/foundation/architecture/core-engine-contracts.md`
- UI dependencies:
  - scoreboard/chat/hud presentation integration coordinates with `ui-engine.md` and `ui-game.md`.

## Non-Goals
- No mirroring of `m-dev` file layout.
- No reintroduction of game-specific logic into engine backends.
- No Phase 6 implementation work in this project until a planning slice is accepted.
- No reopening of UI import scope already tracked by `ui-engine.md`/`ui-game.md`.

## Execution Plan
1. G0 re-baseline slice (doc + packet)
- Replace stale migration assumptions with current landed reality.
- Emit one concrete D2 implementation packet with bounded files and tests.

2. D2 movement replication slice
- Add protocol codec and runtime event seams for `PlayerLocation`.
- Wire authoritative server actor update + outbound replication.
- Add/extend server-net integration coverage.

3. D3 predicted-shot reconciliation slice
- Add rewrite-local predicted shot lifecycle and deterministic reconciliation with authoritative events.

4. G4 scoring/scoreboard semantics slice
- Add authoritative score mutation paths and broadcast contracts.
- Feed gameplay score state into existing UI model seams.

5. G5 round/spawn lifecycle slice
- Add rewrite-owned spawn/respawn/round control semantics and bounded tests.

6. P0 planning-only slice (Phase 6 carry-over)
- Define first bounded Phase 6 validation/stabilization plan (tests only planning, no implementation).

## Validation
From `m-rewrite/`:

```bash
# planning/doc slices
./docs/scripts/lint-project-docs.sh

# code slices (minimum)
./abuild.py -c -d <build-dir>
./scripts/test-server-net.sh <build-dir>

# when gameplay+physics integration seams are touched
./scripts/test-engine-backends.sh <build-dir>
```

## Trace Channels
- `net.client`
- `net.server`
- `engine.server`
- `engine.sim`

## Current Status
- `2026-02-19`: merged `physics-refactor` + `gameplay-migration` into `gameplay.md` to eliminate overlapping/stale planning and align remaining work to rewrite-native behavior migration.
- `2026-02-19`: Phase 5 marked complete in archived physics track; remaining carry-over is Phase 6 entry planning only (no implementation).

## Open Questions
- D2 modeling choice: direct transport-to-component application vs dedicated server movement domain seam first.
- D3 scope boundary: how much reconciliation telemetry is required for first acceptance.
- G4 ordering: whether to land server score authority before or alongside initial UI scoreboard feed wiring.

## Handoff Checklist
- [ ] D2 packet drafted with concrete file list and validation set.
- [ ] D2 behavior slice landed and validated.
- [ ] D3 predicted-shot reconciliation slice landed and validated.
- [ ] G4 scoring/scoreboard semantics slice landed and validated.
- [ ] G5 round lifecycle slice landed and validated.
- [ ] Phase 6 planning-only entry slice documented (no implementation).
