# Gameplay Integration (Merged Track)

## Retirement Snapshot
- Status: `retired`
- Retired on: `2026-02-21`
- Retirement reason: D2/D3/G4/G5 and Phase 6 (`P6-S1`/`P6-S2`/`P6-S3`) are complete; remaining follow-up is optional hardening and not required to keep this project active.
- Superseded active tracks:
  - `m-overseer/projects/cmake.md` for coordinated CMake structure work

## Project Snapshot
- Current owner: `specialist-gameplay-d2`
- Status: `in progress` (D2/D3/G4/G5 landed; P6-S1/P6-S2/P6-S3 shared-unblocker packets landed)
- Supersedes:
  - `docs/archive/physics-refactor-retired-2026-02-19.md`
  - `docs/archive/gameplay-migration-retired-2026-02-19.md`
- Immediate next task: define and execute a bounded post-P6 follow-up packet to tighten client runtime CLI contract coverage against production parser seams without reintroducing transport/profile coupling.
- Validation gate: planning/doc slices run `./docs/scripts/lint-project-docs.sh`; code slices run `./abuild.py -c -d <build-dir>`, `./scripts/test-server-net.sh <build-dir>`, required gameplay `ctest` regex gates, and conditional transport-suite `ctest` only when those tests are registered in the active profile.

## Mission
Port and adapt remaining `m-dev` gameplay behavior into `m-rewrite` using rewrite architecture boundaries:
- engine physics remains generic ECS/sync substrate,
- gameplay rules remain game-owned,
- behavior parity is preserved where intended, but implementation must be rewrite-native (not file-structure mirroring).

## Foundation References
- `docs/AGENTS.md`
- `docs/overseer/execution-policy.md`
- `docs/overseer/operating-model.md`
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
| PlayerLocation transport and authoritative movement replication | `m-dev/src/game/server/game.cpp:203`, `m-dev/src/game/server/client.cpp:51` | codec + transport/runtime + server authoritative actor motion updates + server broadcast/client consume seams are landed | `landed (D2)` |
| Score mutation + scoreboard semantics | `m-dev/src/game/server/game.cpp:263`, `m-dev/src/game/server/client.cpp:94`, `m-dev/src/game/client/game.cpp:191` | authoritative kill-confirmed server score mutation + `ServerMsg_SetScore` wire/send/receive seams and contract tests are landed; UI scoreboard presentation feed remains deferred | `landed (G4 baseline)` |
| Round lifecycle/spawn/respawn | `m-dev/src/game/server/client.cpp`, `m-dev/src/game/server/world_session.cpp` | authoritative spawn/respawn apply + alive-state input authority hardening + server phase resolution semantics (`Stopped` at zero connected clients, `Warmup` when connected but none alive, `Live` when connected alive actors exist) are landed with deterministic contract coverage | `landed (G5)` |
| Predicted shot reconciliation | m-dev netcode behavior lane | rewrite-local pending local-shot tracking + authoritative `global_shot_id` reconciliation baseline, bounded mapping lifecycle, and deterministic trace diagnostics are in place | `landed (D3 baseline)` |

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
  - `docs/overseer/operating-model.md`
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

## Phase 6 Entry Plan
- Track alignment: `shared unblocker`

### Objective
- Establish a deterministic gameplay validation/stabilization sequence after D2/D3/G4/G5 completion, with explicit build-profile evidence and bounded follow-up slices that can execute without reopening landed behavior.

### Non-Goals
- No gameplay/runtime protocol changes.
- No D2/D3/G4/G5 behavior rework.
- No UI/scoreboard implementation expansion.
- No backend/engine refactors outside test/validation plumbing.

### Current Coverage Baseline/Gaps
- Baseline: in `build-a7`, required gameplay gates currently pass for `./abuild.py -c -d build-a7`, `./scripts/test-server-net.sh build-a7`, and required `ctest` regex (`server_net_contract_test`, `server_runtime_event_rules_test`, `server_runtime_shot_physics_integration_test`, `server_runtime_shot_damage_integration_test`, `server_runtime_shot_damage_idempotence_integration_test`, `server_runtime_shot_pilot_smoke_test`, `server_join_runtime_contract_test`, `server_session_runtime_contract_test`, `client_shot_reconciliation_test`).
- Baseline: deterministic round-phase contract coverage exists via `server_round_phase_contract_test`.
- Gap: transport loopback suite registration status in `build-a7` remains `not registered`, so loopback/disconnect/world-package transport integration evidence is absent in this profile.
- Gap: no single gameplay lifecycle contract test currently exercises connect/spawn/death/input-reject/phase-transition/disconnect sequencing in one deterministic packet.
- Gap: gate sequencing expectations are documented but not yet codified as a dedicated Phase 6 stabilization packet with explicit acceptance/risk ownership.

### Prioritized Bounded Slices
1. `P6-S1` (execute first)
- Track label: `shared unblocker`
- Intended files/paths: `m-bz3/scripts/test-server-net.sh`, `m-bz3/src/game/CMakeLists.txt`, `m-bz3/src/tests/transport_environment_probe_test.cpp` (or equivalent existing transport probe target), `m-overseer/projects/gameplay.md`.
- Required validation commands:
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py --claim-lock -d build-a7`
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py -c -d build-a7`
  - `./scripts/test-server-net.sh build-a7`
  - `ctest --test-dir build-a7 -R "server_net_contract_test|server_runtime_event_rules_test|server_runtime_shot_physics_integration_test|server_runtime_shot_damage_integration_test|server_runtime_shot_damage_idempotence_integration_test|server_runtime_shot_pilot_smoke_test|server_join_runtime_contract_test|server_session_runtime_contract_test|client_shot_reconciliation_test" --output-on-failure`
  - `TRANSPORT_REGEX="transport_loopback_integration_test|transport_multiclient_loopback_test|transport_disconnect_lifecycle_integration_test|client_world_package_safety_integration_test"; REGISTERED="$(ctest --test-dir build-a7 -N -R "${TRANSPORT_REGEX}" | awk '/Total Tests:/ {print $3}')"; if [ "${REGISTERED:-0}" != "0" ]; then ctest --test-dir build-a7 -R "${TRANSPORT_REGEX}" --output-on-failure; else echo "transport loopback suite not registered in build-a7"; fi`
  - `cd ../m-overseer && ./scripts/lint-projects.sh`
- Acceptance criteria: gameplay gate sequence is deterministic in one script path, transport registration/skip evidence is explicit and stable in logs for `build-a7`, and no false-negative pass due to silent transport-suite omission.
- Primary risk: environment-specific registration behavior may still diverge across profiles, requiring conditional logic that can mask genuine transport integration regressions if poorly scoped.

2. `P6-S2`
- Track label: `shared unblocker`
- Intended files/paths: `m-bz3/src/tests/server_runtime_lifecycle_contract_test.cpp` (new), `m-bz3/src/tests/server_runtime_event_rules_test.cpp`, `m-bz3/src/tests/server_round_phase_contract_test.cpp`, `m-bz3/src/game/CMakeLists.txt`, `m-overseer/projects/gameplay.md`.
- Required validation commands:
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py --claim-lock -d build-a7`
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py -c -d build-a7`
  - `./scripts/test-server-net.sh build-a7`
  - `ctest --test-dir build-a7 -R "server_runtime_lifecycle_contract_test|server_runtime_event_rules_test|server_round_phase_contract_test|server_join_runtime_contract_test|server_session_runtime_contract_test" --output-on-failure`
  - `cd ../m-overseer && ./scripts/lint-projects.sh`
- Acceptance criteria: one deterministic server lifecycle contract test verifies ordered outcomes for join/spawn/movement+shot reject while dead, alive transition, death, and phase transitions through `Stopped/Warmup/Live` with no network harness dependency.
- Primary risk: overlap with existing unit tests can create brittle duplication if lifecycle assertions are not bounded to contract-level guarantees.

3. `P6-S3`
- Track label: `shared unblocker`
- Intended files/paths: `m-bz3/src/tests/server_net_contract_test.cpp`, `m-bz3/src/tests/client_runtime_cli_contract_test.cpp`, `m-bz3/src/tests/client_shot_reconciliation_test.cpp`, `m-bz3/scripts/test-server-net.sh`, `m-overseer/projects/gameplay.md`.
- Required validation commands:
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py --claim-lock -d build-a7`
  - `export ABUILD_AGENT_NAME=specialist-gameplay-d2 && ./abuild.py -c -d build-a7`
  - `./scripts/test-server-net.sh build-a7`
  - `ctest --test-dir build-a7 -R "server_net_contract_test|client_runtime_cli_contract_test|client_shot_reconciliation_test|server_runtime_event_rules_test" --output-on-failure`
  - `cd ../m-overseer && ./scripts/lint-projects.sh`
- Acceptance criteria: client/server contract matrix for core gameplay net/runtime seams is explicit, deterministic, and mapped to a single bounded validation packet for future parity slices.
- Primary risk: expanding contract matrix breadth too early may increase maintenance cost before transport-suite registration is stabilized.

### First Slice Selection
- Selected first slice: `P6-S1`.
- Justification: transport-suite registration ambiguity in `build-a7` is the highest shared unblocker because it directly affects confidence in all subsequent stabilization packets; resolving validation/registration evidence first prevents downstream slices from producing ambiguous pass/fail signals.

## Validation
From `m-rewrite/`:

```bash
# planning/doc slices
./docs/scripts/lint-project-docs.sh

# code slices (minimum)
./abuild.py -c -d <build-dir>
./scripts/test-server-net.sh <build-dir>
ctest --test-dir <build-dir> -R "server_net_contract_test|server_runtime_event_rules_test|server_runtime_shot_physics_integration_test|server_runtime_shot_damage_integration_test|server_runtime_shot_damage_idempotence_integration_test|server_runtime_shot_pilot_smoke_test|server_join_runtime_contract_test|server_session_runtime_contract_test|client_shot_reconciliation_test" --output-on-failure

# conditional transport loopback suite (run only if registered in this profile)
TRANSPORT_REGEX="transport_loopback_integration_test|transport_multiclient_loopback_test|transport_disconnect_lifecycle_integration_test|client_world_package_safety_integration_test"
REGISTERED="$(ctest --test-dir <build-dir> -N -R "${TRANSPORT_REGEX}" | awk '/Total Tests:/ {print $3}')"
if [ "${REGISTERED:-0}" != "0" ]; then ctest --test-dir <build-dir> -R "${TRANSPORT_REGEX}" --output-on-failure; else echo "transport loopback suite not registered in <build-dir>"; fi
```

## Trace Channels
- `net.client`
- `net.server`
- `engine.server`
- `engine.sim`

## Current Status
- `2026-02-19`: merged `physics-refactor` + `gameplay-migration` into `gameplay.md` to eliminate overlapping/stale planning and align remaining work to rewrite-native behavior migration.
- `2026-02-19`: Phase 5 marked complete in archived physics track; remaining carry-over is Phase 6 entry planning only (no implementation).
- `2026-02-20`: D2 movement-replication slice landed for `ClientMsg_PlayerLocation` ingest, authoritative `ServerGame` actor/session motion application, `ServerMsg_PlayerLocation` send-except broadcast semantics, and client send/consume seams.
- `2026-02-20`: validation passed in assigned slot `build-a7` with `./abuild.py -c -d build-a7`, `./scripts/test-server-net.sh build-a7`, and targeted server-runtime contract/integration `ctest` regex gate.
- `2026-02-20`: D3 client-net predicted-shot reconciliation baseline landed: pending `local_shot_id` tracking on successful `sendCreateShot`, authoritative `ServerMsg_CreateShot` reconciliation for the local client, bounded `global_shot_id -> local_shot_id` lifecycle mapping with `ServerMsg_RemoveShot` cleanup, and reconciliation-state reset on start/shutdown/disconnect/reconnect.
- `2026-02-20`: D3 validation passed in `build-a7` with `./abuild.py -c -d build-a7`, `./scripts/test-server-net.sh build-a7`, required server/client runtime `ctest` regex including `client_shot_reconciliation_test`, and conditional transport-suite registration probe (`not registered` in this profile).
- `2026-02-20`: G4 authoritative score baseline landed: kill-confirmed server score mutation is now session-authoritative in gameplay domain state, server emits `ServerMsg_SetScore` on score changes, transport broadcasts score updates, and client net runtime consumes `SetScore` with deterministic trace logs.
- `2026-02-20`: G4 validation passed in `build-a7` with `./abuild.py -c -d build-a7`, `./scripts/test-server-net.sh build-a7`, contract `SetScore` codec round-trip coverage in `server_net_contract_test`, and kill/idempotence score-event coverage in shot-damage integration tests.
- `2026-02-20`: G5 spawn/respawn-first slice landed: `ClientRequestSpawn` now applies authoritatively in server gameplay state (unknown-client reject, already-alive reject, dead-only accept with deterministic health/pose/velocity restore), `PlayerSpawn` broadcast occurs only on accepted spawns, and runtime traces explicitly record spawn accept/reject outcomes.
- `2026-02-20`: minimal round phase lifecycle behavior landed in server domain: startup `Warmup`, `Warmup -> Live` when at least one connected alive actor exists, and `Live -> Warmup` when none remain, with deterministic transition trace evidence.
- `2026-02-20`: G5 follow-up hardening slice landed: server authority now rejects `ClientPlayerLocation` and `ClientCreateShot` when sender actor state is dead/unspawned, create-shot rejection has an explicit runtime-rule result, and event-loop traces emit deterministic rejection diagnostics.
- `2026-02-20`: validation passed in `build-a7` for hardening scope with `./abuild.py -c -d build-a7`, `./scripts/test-server-net.sh build-a7`, required runtime/contract `ctest` regex gate (including `server_runtime_event_rules_test` and `client_shot_reconciliation_test`), and conditional transport probe (`not registered` in this profile).
- `2026-02-20`: gameplay validation policy aligned to this repo profile by removing nonexistent `./scripts/test-engine-backends.sh` gating and preserving concrete runnable wrapper/`ctest` gates.
- `2026-02-20`: G5 lifecycle completion slice landed: authoritative round-phase resolution now enforces `Stopped` when connected client count is `0`, `Warmup` when connected client count is `>0` with `0` connected alive actors, and `Live` when connected alive actors are `>0`; transition traces remain deterministic and include both decision counts.
- `2026-02-20`: deterministic round-phase contract coverage landed via `server_round_phase_contract_test`, including zero-connected-client precedence edge cases, and validation passed in `build-a7` with required wrapper/net/ctest gates plus the new phase-contract test.
- `2026-02-21`: Phase 6 planning-only entry slice completed (`shared unblocker`): documented objective/non-goals, baseline gaps (including `build-a7` transport-suite `not registered` status), and prioritized bounded P6 execution slices (`P6-S1`, `P6-S2`, `P6-S3`) with explicit validations, acceptance criteria, and risks.
- `2026-02-21`: selected `P6-S1` as first implementation slice to stabilize validation/transport registration evidence before broader lifecycle and contract-matrix expansion.
- `2026-02-21`: `P6-S1` landed (`shared unblocker`): `./scripts/test-server-net.sh <build-dir>` is now the canonical deterministic gameplay gate entry for this track, running wrapper configure/build (`./abuild.py -c -d <build-dir>`), the required gameplay `ctest` regex, and explicit transport registration evidence every run.
- `2026-02-21`: transport-loopback visibility hardening landed in canonical script path: when transport tests are not registered, logs now explicitly report `not registered` and include bounded profile diagnostic evidence (`transport_environment_probe_test` presence/absence) to avoid silent omission.
- `2026-02-21`: `P6-S2` landed (`shared unblocker`): added `server_runtime_lifecycle_contract_test` as one deterministic lifecycle packet covering ordered join/connect state, spawn behavior, dead-state movement/create-shot rejects, alive-state acceptance, authoritative death transition, leave/death callback semantics, and `Stopped/Warmup/Live` phase progression in one contract scenario.
- `2026-02-21`: `P6-S2` validation passed in `build-a7` with canonical `./scripts/test-server-net.sh build-a7`, focused lifecycle contract gate (`server_runtime_lifecycle_contract_test|server_runtime_event_rules_test|server_round_phase_contract_test|server_join_runtime_contract_test|server_session_runtime_contract_test`), and required broad gameplay regression gate regex.
- `2026-02-21`: `P6-S3` landed (`shared unblocker`): stabilized deterministic client/server gameplay contract-matrix assertions in `server_net_contract_test`, `client_runtime_cli_contract_test`, and `client_shot_reconciliation_test` without transport loopback coupling or gameplay/runtime behavior changes.
- `2026-02-21`: `P6-S3` validation passed in `build-a7` with `./abuild.py -c -d build-a7`, canonical `./scripts/test-server-net.sh build-a7` (including explicit `transport loopback suite not registered` + `transport_environment_probe_test` diagnostic evidence), and focused contract gate `ctest --test-dir build-a7 -R "server_net_contract_test|client_runtime_cli_contract_test|client_shot_reconciliation_test|server_runtime_event_rules_test" --output-on-failure`.

## Open Questions
- G4 follow-up boundary: whether to keep scoreboard UI feed deferred until after G5 completion, or land a narrow HUD feed slice in parallel.
- Future profile policy: whether transport-suite registration is required only for `build-a7` or must become a mandatory gate across additional build profiles.
- Post-`P6-S3` client CLI contract depth: whether to keep `client_runtime_cli_contract_test` as transport-agnostic deterministic contract coverage or add a separate production-parser seam test packet once runtime library/linking dependencies are stabilized for this profile.

## Handoff Checklist
- [x] D2 packet drafted with concrete file list and validation set.
- [x] D2 behavior slice landed and validated.
- [x] D3 predicted-shot reconciliation slice landed and validated (baseline local/global shot ID reconciliation).
- [x] G4 scoring/scoreboard semantics slice landed and validated (authoritative score mutation + `SetScore` server/client contract baseline).
- [x] G5 spawn/respawn-first baseline landed (authoritative spawn apply + accepted-only spawn broadcast + minimal Warmup/Live transitions).
- [x] G5 follow-up hardening slice landed and validated (alive-state authority for `ClientPlayerLocation`/`ClientCreateShot`, explicit rejected create-shot runtime-rule result, deterministic rejection traces, and runtime-rule rejection-path tests).
- [x] G5 full round lifecycle slice landed and validated (authoritative `Stopped/Warmup/Live` resolution semantics + deterministic transition traces + round-phase contract test coverage).
- [x] Phase 6 planning-only entry slice documented (no implementation).
- [x] P6-S1 shared-unblocker slice landed and validated (transport registration evidence + canonical gameplay gate sequencing stability in `build-a7`).
- [x] P6-S2 lifecycle contract packet landed and validated.
- [x] P6-S3 client/server contract matrix stabilization packet landed and validated.
