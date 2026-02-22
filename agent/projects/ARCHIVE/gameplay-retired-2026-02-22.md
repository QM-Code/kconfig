# Gameplay Playable Loop (Localhost)

## Project Snapshot
- Current owner: `specialist-gp-s5`
- Status: `green (GP-S5 landed/validated; pending manual localhost retirement gate)`
- Immediate next task: run manual localhost end-to-end verification for outcomes `0`-`5` and complete retirement prep handoff.
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
| `0` Join + auto name | **implemented** | server fallback now deterministically resolves `<playerAutoNamePrefix><client_id>` from required config and rejects missing/invalid prefix: `m-bz3/src/server/runtime/server_game.cpp`, `m-bz3/data/server/config.json`; client startup preserves empty default join name for server assignment: `m-bz3/src/client/runtime/startup_options.cpp` | manual localhost verification pending |
| `1` First-person tank drive | **implemented** | default client startup path is tank-ready while preserving explicit spawn action gate: `m-bz3/data/client/config.json`, `m-bz3/src/client/game/lifecycle.cpp` | manual localhost verification pending |
| `2` Shoot + kill | **implemented** | client shot create path now sends non-zero spawn/velocity from local tank/camera context; server remains authoritative for hit/damage/death: `m-bz3/src/client/game/lifecycle.cpp`, `m-bz3/src/client/net/connection/outbound.cpp`, `m-bz3/src/server/runtime/shot_pilot_step.cpp` | manual localhost verification pending |
| `3` Server-authoritative running score | **implemented** | client now applies authoritative `ServerMsg_SetScore` into gameplay state/HUD (deterministic ordering) with no local score authority: `m-bz3/src/client/net/connection/inbound/session_events.cpp`, `m-bz3/src/client/game/lifecycle.cpp`, `m-bz3/src/client/game/game.hpp` | manual localhost verification pending |
| `4` Ricochet off buildings | **implemented** | bounded server-shot ricochet now reflects using authoritative hit normals with deterministic termination: `m-bz3/src/server/domain/shot_system.cpp`; runtime routes ricochet to non-actor world hits only: `m-bz3/src/server/runtime/shot_pilot_step.cpp` | manual localhost verification pending |
| `5` Jump + land on buildings | **implemented** | jump edge input now propagates from client tank control through player-location packets; server applies authoritative impulse/gravity/grounded transitions and broadcasts reconciled vertical state: `m-bz3/src/client/game/tank_motion.cpp`, `m-bz3/src/client/net/connection/outbound.cpp`, `m-bz3/src/server/runtime/server_game.cpp`, `m-bz3/src/server/runtime/event_loop.cpp` | manual localhost verification pending |

### Additional High-Impact Findings
1. Prior non-parity actor debug behavior has been removed from authoritative server tick:
- synthetic sinusoidal actor drift and synthetic health drain are no longer active in `m-bz3/src/server/domain/actor_system.cpp`.

2. `m-karma` backend is largely sufficient for baseline gameplay integration:
- backend already exposes gravity toggle, force/impulse, velocity, and raycast APIs: `m-karma/include/karma/physics/backend.hpp:110`, `m-karma/include/karma/physics/backend.hpp:129`, `m-karma/include/karma/physics/backend.hpp:136`, `m-karma/include/karma/physics/backend.hpp:182`.

3. `m-karma` parity seams still remaining after GP-S4:
- generic raycast hit-normal seam is now landed in backend + facade contracts (`RaycastHit.normal` surfaced by Jolt/PhysX and world raycast): `m-karma/include/karma/physics/backend.hpp`, `m-karma/src/physics/backends/jolt.cpp`, `m-karma/src/physics/backends/physx.cpp`, `m-karma/src/physics/facade/world.cpp`.
- `PlayerController::isGrounded()` is intentionally deferred: `m-karma/src/physics/facade/player_controller.cpp:185`.

## Backend Readiness Verdict
- `m-karma` is **ready enough** for outcomes `0`-`4` and likely `5` with game-owned grounded rules.
- `m-bz3` remains the primary implementation surface for playable loop behavior.
- GP-S4 confirmed the minimal shared unblocker path: a generic `m-karma` hit-normal seam was required and sufficient to deliver physically-correct server ricochet in `m-bz3`.

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
- `2026-02-22`: `GP-S5` completed in `m-bz3` (`m-dev parity` track):
  - jump input now participates in tank motion and is propagated via client player-location messages (`jump_pressed` flag) rather than remaining a dead input path,
  - server runtime now applies deterministic authoritative vertical motion (jump impulse, gravity, bounded fall speed, grounded/landing transitions) and returns reconciled local position/velocity state,
  - client local prediction/reconciliation remains intact with explicit spawn gate unchanged (no auto-spawn-on-join behavior introduced),
  - no `m-karma` seam was required for GP-S5; existing raycast/query capabilities were sufficient for grounded rules in `m-bz3`,
  - required validation passed in `build-gp-s5` (`vcpkg_present`, `abuild --claim-lock`, `abuild -c --karma-sdk`, `test-server-net`, targeted `ctest` regex packet for tank/runtime/net/ricochet coverage).
- `2026-02-22`: `GP-S4` completed across `shared unblocker` + `m-dev parity` tracks:
  - minimal generic `m-karma` raycast seam landed (`RaycastHit.normal`) in Jolt and PhysX backends plus world facade propagation; parity checks now assert non-zero unit normals from ray queries,
  - server shot domain now supports bounded deterministic ricochet state (max bounce count, physically-correct normal reflection, deterministic fallback termination when ricochet is not applicable),
  - runtime shot pilot now classifies ricochet as non-actor world-hit behavior while preserving actor-hit removal/damage authority on server,
  - explicit spawn gate and naming policies remained unchanged; no client-side ricochet authority was introduced,
  - validation passed in `build-gp-s4` for both repos (`m-karma`: `abuild -c --install-sdk`, `test-engine-backends`; `m-bz3`: `abuild -c --karma-sdk`, `test-server-net`, targeted `ctest` packet).
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
- [x] `GP-S4` landed and validated.
- [x] `GP-S5` landed and validated.
- [ ] Manual localhost end-to-end verification for outcomes `0`-`5`.
- [ ] Archive this project after completion and update `ASSIGNMENTS.md`.
