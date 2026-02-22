# Gameplay Playable Loop (Localhost)

## Project Snapshot
- Current owner: `specialist-gp-s3`
- Status: `green (GP-S3 landed/validated; advancing to GP-S4)`
- Immediate next task: execute `GP-S4` (server-side ricochet behavior with bounded lifecycle and required hit-normal seam if needed).
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-bz3`: `./abuild.py -c -d <bz3-build-dir>`, `./scripts/test-server-net.sh <bz3-build-dir>`, targeted `ctest` packet for touched contracts
  - `m-karma` (only when backend seam touched): `./abuild.py -c -d <karma-build-dir>`, `./scripts/test-engine-backends.sh <karma-build-dir>`

## Mission
Deliver a playable localhost loop where these six outcomes work with default runtime commands:

```bash
# server
bz3-server --listen-port <port>

# client
bz3 --server <host:port>
```

Target outcomes:
0. client connects and can join (auto-assigned name when none provided, e.g. `player14`)
1. player drives tank in first-person play mode
2. player can shoot and kill other players
3. running score is server-authoritative
4. shots ricochet off buildings
5. tanks can jump and land on buildings

## Foundation References
- `m-bz3/src/server/runtime/server_game.cpp`
- `m-bz3/src/server/runtime/shot_pilot_step.cpp`
- `m-bz3/src/server/domain/shot_system.cpp`
- `m-bz3/src/client/game/lifecycle.cpp`
- `m-bz3/src/client/net/connection/outbound.cpp`
- `m-karma/include/karma/physics/backend.hpp`
- `m-karma/src/physics/facade/player_controller.cpp`

## Why This Is Separate
Previous gameplay migration/phasing work (D2/D3/G4/G5/P6) was completed and archived.  
This track is a focused playable product loop bring-up across gameplay/runtime seams, not transport-contract migration.

## Comprehensive Findings (Current State)

### Outcome Status Matrix
| Outcome | Current posture | Evidence | Gap |
|---|---|---|---|
| `0` Join + auto name | **partial** | server fallback name is deterministic `player-<client_id>` when join name is empty: `m-bz3/src/server/runtime/server_game.cpp:208`; client normally sends configured username (`userDefaults.username`): `m-bz3/src/client/runtime/startup_options.cpp:23`, `m-bz3/data/client/config.json:8`; duplicate active names are rejected: `m-bz3/src/server/runtime/server_game.cpp:212` | default client behavior does not reliably produce auto-assigned unique names when no explicit name is passed |
| `1` First-person tank drive | **partial** | tank mode exists but default config disables it: `m-bz3/data/client/config.json:142`; startup reads it default false: `m-bz3/src/client/game/lifecycle.cpp:26`; observer/roaming remains fallback when no tank entity: `m-bz3/src/client/game/lifecycle.cpp:245` | default launch path lands in observer mode, not play mode |
| `2` Shoot + kill | **partial** | authoritative shot/damage/death pipeline exists (`D3/G4/G5` landed); server kill flow emits death + score event: `m-bz3/src/server/runtime/shot_pilot_step.cpp:128`; but client shot send currently uses zero pos/vel: `m-bz3/src/client/net/connection/outbound.cpp:74`; server applies incoming shot vectors directly: `m-bz3/src/server/runtime/event_rules.cpp:65` | gameplay shot spawn/velocity from local tank/camera is not wired |
| `3` Server-authoritative running score | **partial** | server mutates session score on kill: `m-bz3/src/server/runtime/shot_damage.cpp:58`; server broadcasts `SetScore`: `m-bz3/src/server/net/transport_event_source/events.cpp:27` | client receive seam is trace-only, not HUD/game-state presentation: `m-bz3/src/client/net/connection/inbound/session_events.cpp:53` |
| `4` Ricochet off buildings | **not implemented** | shot currently expires on first non-ignored physics hit: `m-bz3/src/server/domain/shot_system.cpp:163` | no bounce/reflection model or ricochet lifecycle state |
| `5` Jump + land on buildings | **not implemented** | `jump` input is declared: `m-bz3/src/ui/console/keybindings.cpp:15`; local tank motion does not consume jump action: `m-bz3/src/client/game/tank_motion.cpp:28`; tank/actor gravity currently disabled in key paths: `m-bz3/src/client/game/tank_entity.cpp:152`, `m-bz3/src/server/runtime/server_game.cpp:357` | no gameplay jump impulse, airtime state, landing rules, or vertical physics behavior |

### Additional High-Impact Findings
1. Non-parity actor debug behavior is still active in server tick path and can corrupt gameplay expectations:
- `m-bz3/src/server/domain/actor_system.cpp:123` moves actors each tick via synthetic sinusoid.
- `m-bz3/src/server/domain/actor_system.cpp:126` continuously drains health.

2. `m-karma` backend is largely sufficient for baseline gameplay integration:
- backend already exposes gravity toggle, force/impulse, velocity, and raycast APIs: `m-karma/include/karma/physics/backend.hpp:110`, `m-karma/include/karma/physics/backend.hpp:129`, `m-karma/include/karma/physics/backend.hpp:136`, `m-karma/include/karma/physics/backend.hpp:182`.

3. `m-karma` seams still likely needed for full-quality parity:
- `RaycastHit` currently has no surface normal field (`body`, `position`, `distance`, `fraction` only): `m-karma/include/karma/physics/backend.hpp:79`.
- `PlayerController::isGrounded()` is intentionally deferred: `m-karma/src/physics/facade/player_controller.cpp:185`.

## Backend Readiness Verdict
- `m-karma` is **ready enough** for outcomes `0`-`3` and likely `5` with game-owned grounded rules.
- `m-bz3` remains the primary implementation surface for playable loop behavior.
- For robust ricochet (`4`), a minimal `m-karma` backend/query contract extension for hit normals is the likely clean path.

## Strategic Track Labels
- Primary track: `m-dev parity`
- Secondary track: `shared unblocker` (for backend seam additions required by gameplay contracts)

## Owned Paths
- `m-overseer/agent/projects/gameplay.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`
- `m-bz3/src/client/game/*`
- `m-bz3/src/client/net/*`
- `m-bz3/src/server/*`
- `m-bz3/src/tests/*`
- `m-karma/include/karma/physics/*` and backend impl/test paths only when explicitly required by a slice

## Interface Boundaries
- Keep gameplay semantics in `m-bz3` game/runtime/domain layers.
- Keep `m-karma` changes generic (no BZ3-specific rules).
- Preserve server authority for spawn, shot lifecycle, damage/death, and score mutation.
- No direct backend leakage into user-facing client game APIs.

## Non-Goals
- No broad rendering or UI-system refactor.
- No transport harness overreach beyond gameplay requirements.
- No reopening archived migration packets unless a direct regression is found.
- No feature scope expansion beyond outcomes `0`-`5`.

## Execution Plan (Bounded Slices)

### `GP-S1` (first)
- Track: `m-dev parity`
- Goal: default into playable join flow and remove non-parity actor tick drift.
- Scope:
  - ensure client join path can rely on server-side auto name assignment when explicit name is absent.
  - make fallback auto-name prefix server-configurable and mandatory from `data/server/config.json`; derive fallback as `<prefix><client_id>` with no hardcoded fallback.
  - default local flow to tank-ready gameplay for network play launch while keeping spawn as explicit user input (no automatic spawn on join).
  - remove synthetic server actor tick drift/health drain behavior from gameplay runtime.
  - keep spawn authority model deterministic.
- Intended files:
  - `m-bz3/src/client/runtime/startup_options.cpp`
  - `m-bz3/src/client/game/lifecycle.cpp`
  - `m-bz3/src/server/runtime/server_game.cpp`
  - `m-bz3/src/server/domain/actor_system.cpp`
  - `m-bz3/src/tests/server_runtime_lifecycle_contract_test.cpp`
  - `m-bz3/src/tests/server_runtime_event_rules_test.cpp`
  - `m-bz3/src/tests/client_runtime_cli_contract_test.cpp`
- Acceptance:
  - default `bz3 --server <host:port>` joins reliably without manual username override.
  - when join name is omitted, server uses required configured prefix + `client_id` and does not use hardcoded fallback naming.
  - spawn remains explicit (`spawn` action required) after join; no automatic spawn side effect.
  - default launch is tank-ready/playable once explicit spawn occurs (not a permanent observer-only path).
  - no synthetic health drain or sinusoidal drift in authoritative actor tick.

### `GP-S2`
- Track: `m-dev parity`
- Goal: authoritative shot creation from actual tank/camera state and kill loop completion.
- Scope:
  - compute/send non-zero shot spawn position + velocity from local gameplay state.
  - preserve local prediction + authoritative reconciliation seams.
  - verify kill path remains server-authoritative.
- Intended files:
  - `m-bz3/src/client/game/*` (shot origin/aim seam)
  - `m-bz3/src/client/net/connection/outbound.cpp`
  - `m-bz3/src/server/runtime/event_rules.cpp` (validation only if needed)
  - `m-bz3/src/tests/client_shot_reconciliation_test.cpp`
  - `m-bz3/src/tests/server_runtime_shot_damage_integration_test.cpp`
- Acceptance:
  - shots are visibly emitted from tank context and can kill targets through existing authority path.

### `GP-S3`
- Track: `m-dev parity`
- Goal: complete running score gameplay surface.
- Scope:
  - wire `ServerMsg_SetScore` into client gameplay state/HUD.
  - keep server as sole score source of truth.
- Intended files:
  - `m-bz3/src/client/net/connection/inbound/session_events.cpp`
  - `m-bz3/src/client/game/*` (state + HUD binding)
  - `m-bz3/src/tests/*score*` and/or runtime contract tests
- Acceptance:
  - score changes reflect in client HUD/state only from authoritative server messages.

### `GP-S4`
- Track: `shared unblocker` then `m-dev parity`
- Goal: ricochet behavior for shots against buildings.
- Scope:
  - add bounded shot ricochet state in server shot domain (bounce count/energy/termination).
  - add/use hit-normal query seam for reflection.
  - if needed, extend `m-karma` raycast hit contract to include normal vector.
- Intended files:
  - `m-bz3/src/server/domain/shot_system.*`
  - `m-bz3/src/server/runtime/shot_pilot_step.cpp`
  - `m-bz3/src/tests/server_runtime_shot_physics_integration_test.cpp`
  - optional `m-karma/include/karma/physics/backend.hpp` + backend implementations/tests
- Acceptance:
  - deterministic ricochet occurs on building hits with bounded bounce lifecycle and removal rules.

### `GP-S5`
- Track: `m-dev parity`
- Goal: jump + landing on building geometry.
- Scope:
  - consume jump input in tank motion with server-authoritative state.
  - enable vertical physics path (gravity/impulse) for tank actor semantics.
  - define landing/grounded contract (game-owned; optionally backend-assisted).
- Intended files:
  - `m-bz3/src/client/game/tank_motion.cpp`
  - `m-bz3/src/client/game/tank_entity.cpp`
  - `m-bz3/src/server/runtime/server_game.cpp`
  - `m-bz3/src/server/runtime/event_loop.cpp`
  - `m-bz3/src/tests/*tank*` + lifecycle/physics integration tests
  - optional `m-karma` grounded/query seam if strictly required
- Acceptance:
  - tank can jump, arc, collide, and land on buildings with deterministic server authority.

## Validation
```bash
# m-bz3 required per implementation slice
cd m-bz3
export ABUILD_AGENT_NAME=specialist-gameplay-loop
./abuild.py --claim-lock -d <bz3-build-dir>
./abuild.py -c -d <bz3-build-dir>
./scripts/test-server-net.sh <bz3-build-dir>

# targeted ctest packet (expand per slice scope)
ctest --test-dir <bz3-build-dir> -R "server_net_contract_test|server_runtime_event_rules_test|server_runtime_lifecycle_contract_test|server_runtime_shot_damage_integration_test|server_runtime_shot_physics_integration_test|server_round_phase_contract_test|client_runtime_cli_contract_test|client_shot_reconciliation_test" --output-on-failure

# m-karma only when backend/query contracts change
cd ../m-karma
export ABUILD_AGENT_NAME=specialist-gameplay-loop
./abuild.py --claim-lock -d <karma-build-dir>
./abuild.py -c -d <karma-build-dir>
./scripts/test-engine-backends.sh <karma-build-dir>
./abuild.py --release-lock -d <karma-build-dir>
```

### Manual Localhost Exit Criteria (required before retirement)
1. Start server:
```bash
./<build-dir>/bz3-server --listen-port 11899
```
2. Start two clients:
```bash
./<build-dir>/bz3 --server 127.0.0.1:11899
./<build-dir>/bz3 --server 127.0.0.1:11899
```
3. Verify outcomes `0`-`5` end-to-end in one runtime session with trace evidence.

## Current Status
- `2026-02-22`: `GP-S3` completed in `m-bz3` (`m-dev parity` track):
  - client `ServerMsg_SetScore` handling now forwards authoritative score updates through a dedicated connection callback event (no client-local score authority),
  - gameplay state now stores authoritative per-client score values and clears lifecycle state on start/shutdown,
  - HUD now surfaces server-authoritative score entries deterministically (sorted by score desc, then client_id asc),
  - explicit spawn-on-action policy and server-owned naming policy remain unchanged,
  - no server authority semantics changed for kill/score mutation (presentation/state wiring only),
  - required validation gates passed in `build-gp-s1` (`abuild -c --karma-sdk`, `test-server-net`, targeted `ctest` packet including `.*score.*`).
- `2026-02-22`: `GP-S2` completed in `m-bz3` (`m-dev parity` track):
  - client fire path now computes deterministic shot spawn vectors from local tank/camera state and sends non-zero shot position/velocity payloads instead of zero vectors,
  - client outbound shot send seam now accepts explicit shot position/velocity while preserving local prediction and authoritative reconciliation flow,
  - server authority boundaries remain unchanged (server still validates create_shot, owns kill/death/score mutation path),
  - contract coverage added for client create_shot payload round-trip and local shot vector computation fallback behavior,
  - required validation gates passed in `build-gp-s1` (`abuild -c --karma-sdk`, `test-server-net`, targeted `ctest` packet).
- `2026-02-22`: `GP-S1` completed in `m-bz3` (`m-dev parity` track):
  - server fallback names now resolve as required `<server.playerAutoNamePrefix><client_id>` with deterministic failure when prefix config is missing/invalid (no hardcoded fallback),
  - client network startup now preserves empty default join name when no explicit username is provided, enabling server-side deterministic auto-assignment,
  - explicit spawn gate preserved (no auto-spawn on join); contract coverage updated to assert join/connect does not auto-spawn,
  - default client config is tank-ready for network play (`gameplay.tank.enabled=true`) while spawn remains explicit,
  - synthetic sinusoidal drift and synthetic health decay removed from authoritative server actor tick,
  - required validation gates passed in `build-gp-s1` (`abuild`, `test-server-net`, targeted `ctest` packet).
- `2026-02-21`: New active project doc created for playable localhost loop bring-up after migration-track retirement.
- `2026-02-21`: Baseline findings captured: join/name fallback mismatch, observer-by-default startup, shot send vector gap, score display seam missing, ricochet/jump not implemented, and residual non-parity actor tick behavior.
- `2026-02-21`: Backend readiness clarified: `m-karma` largely sufficient for baseline gameplay, with likely raycast-normal seam needed for robust ricochet.
- `2026-02-22`: Policy decisions locked for GP execution:
  - fallback auto-name policy: deterministic by `client_id`, with mandatory configurable server-side prefix from `data/server/config.json` and no hardcoded fallback prefix,
  - spawn behavior policy: explicit user-triggered spawn only (no automatic spawn on join),
  - ricochet packet policy: implement physically-correct normal reflection first.

## Open Questions
- none (policy decisions above are locked; raise new questions only if blocking conflicts are discovered during implementation).

## Handoff Checklist
- [x] Comprehensive findings documented with concrete code evidence.
- [x] `GP-S1` landed and validated.
- [x] `GP-S2` landed and validated.
- [x] `GP-S3` landed and validated.
- [ ] `GP-S4` landed and validated.
- [ ] `GP-S5` landed and validated.
- [ ] Manual localhost end-to-end verification for outcomes `0`-`5`.
- [ ] Archive this project after completion and update `ASSIGNMENTS.md`.
