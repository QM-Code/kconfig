# Cleanup S5 (`CLN-S5`): Physics Parity Suite Decomposition

## Project Snapshot
- Current owner: `specialist-cln-s5`
- Status: `complete (CLN-S5 closed with intentional shared-core decision for residual common.cpp checks)`
- Immediate next task: hand off to cleanup parent sequencing for CLN-S5 archive/retire workflow and lane-capacity reallocation.
- Validation gate: `cd m-karma && ctest --test-dir <karma-build-dir> -R "physics_backend_parity_.*" --output-on-failure`.

## Mission
Complete the parity-suite split so file boundaries reflect real test domains, not wrapper stubs over a single giant implementation unit.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/physics/tests/parity/main.cpp`
- `m-karma/src/physics/tests/parity/common.hpp`
- `m-karma/src/physics/tests/parity/common.cpp`
- `m-karma/cmake/sdk/tests.cmake`

## Why This Is Separate
This is a test-architecture cleanup track that can proceed in parallel with runtime and renderer refactors while guarding physics behavior contracts.

## Owned Paths
- `m-karma/src/physics/tests/parity/*`
- `m-karma/cmake/sdk/tests.cmake` (parity target wiring only)
- `m-overseer/agent/projects/cleanup/physics-parity-suite.md`

## Interface Boundaries
- Inputs consumed:
  - current parity assertions and backend matrix semantics.
- Outputs exposed:
  - maintainable domain-split parity suite with clear fixture/helper ownership.
- Coordinate before changing:
  - `projects/cleanup/physics-sync-decomposition.md`

## Non-Goals
- Do not weaken parity coverage to reduce file size.
- Do not alter command-line backend selection semantics in `parity/main.cpp`.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
ctest --test-dir <karma-build-dir> -R "physics_backend_parity_jolt|physics_backend_parity_physx" --output-on-failure
```

## Trace Channels
- `cleanup.s5`
- `physics.tests.parity`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
./<karma-build-dir>/physics_backend_parity_test --backend jolt
./<karma-build-dir>/physics_backend_parity_test --backend physx
```

## Current Status
- `2026-02-22`: split scaffold exists (`parity/*.cpp` wrappers + `main.cpp`), but implementation remains concentrated in `parity/common.cpp` (~8.5k LOC).
- `2026-02-22`: moved under cleanup superproject child structure.
- `2026-02-22`: executed S5-1 migration by moving `RunEcsSyncSystemPolicyChecks(BackendKind)` from `src/physics/tests/parity/common.cpp` to `src/physics/tests/parity/ecs_sync_checks.cpp`, keeping `RunEcsSyncCheckSuite(BackendKind)` as the suite entrypoint and preserving parity CLI/backend selection surfaces in `parity/main.cpp`.
- `2026-02-22`: established local helper ownership in `ecs_sync_checks.cpp` for the migrated domain (`NearlyEqual`, `NearlyEqualVec3`, `EqualCollisionMask`, and `RuntimeTraceCaptureSink`) and consumed CLN-S4 runtime-command seam via `physics/sync/runtime_command_sync.hpp` for trace-classification tag checks.
- `2026-02-22`: measured file-shape change from pre-slice HEAD baseline to migrated state: `common.cpp` `8539 -> 6506` lines (`-2033`), `ecs_sync_checks.cpp` `9 -> 2137` lines (`+2128`).
- `2026-02-22`: executed S5-2 migration by moving engine-sync domain checks from `src/physics/tests/parity/common.cpp` to `src/physics/tests/parity/engine_sync_checks.cpp`: `RunEngineFixedStepSyncValidationChecks(BackendKind)`, `RunAppEngineSyncLifecycleSmokeChecks(BackendKind)`, and `RunRepeatabilityChecks(BackendKind)`, while keeping `RunEngineSyncCheckSuite(BackendKind)` as the domain suite entrypoint.
- `2026-02-22`: moved engine-sync-local support types/helpers into `engine_sync_checks.cpp`: `ServerEngineSmokeGame`, `ClientEngineSmokeGame`, `EngineSyncLifecycleCapture`, `ClientLifecycleBootstrap`, and `ScopedClientLifecycleConfigOverrides`.
- `2026-02-22`: measured S5-2 file-shape change from pre-slice HEAD baseline to migrated state: `common.cpp` `8539 -> 4689` lines (`-3850`), `engine_sync_checks.cpp` `18 -> 1865` lines (`+1847`).
- `2026-02-22`: transient lock-owner blocker on `build-cln-s5` was cleared (`overseer-verify` lock released), then S5-2 validation completed green: `./abuild.py -c -d build-cln-s5 -b jolt,physx`, `./scripts/test-engine-backends.sh build-cln-s5`, and `ctest --test-dir build-cln-s5 -R "physics_backend_parity_jolt|physics_backend_parity_physx" --output-on-failure`.
- `2026-02-22`: executed S5-3 migration by moving collider/runtime domain checks from `src/physics/tests/parity/common.cpp` to `src/physics/tests/parity/collider_runtime_checks.cpp`: `RunColliderShapeAndRuntimePropertyChecks(BackendKind)`, `RunColliderShapeOffsetQueryChecks(BackendKind)`, `RunBodyVelocityApiChecks(BackendKind)`, `RunBodyForceImpulseApiChecks(BackendKind)`, `RunBodyDampingApiChecks(BackendKind)`, `RunBodyMaterialApiChecks(BackendKind)`, `RunBodyKinematicApiChecks(BackendKind)`, `RunBodyAwakeApiChecks(BackendKind)`, `RunBodyMotionLockApiChecks(BackendKind)`, `RunUninitializedApiChecks(BackendKind)`, `RunInvalidBodyApiChecks(BackendKind)`, `RunBodyGravityFlagChecks(BackendKind)`, `RunBodyRotationLockChecks(BackendKind)`, `RunBodyTranslationLockChecks(BackendKind)`, `RunVelocityIntegrationChecks(BackendKind)`, `RunRaycastQueryChecks(BackendKind)`, `RunGroundCollisionChecks(BackendKind)`, and `RunRestingStabilityChecks(BackendKind)`, while keeping `RunColliderRuntimeCheckSuite(BackendKind)` as the domain suite entrypoint.
- `2026-02-22`: established collider/runtime-local helper ownership in `collider_runtime_checks.cpp` for migrated checks (`kEpsilon`, `kGravityDropMin`, `NearlyEqual`, `NearlyEqualVec3`, `NearlyEqualQuat`, `EqualCollisionMask`, and `ValidateTransform`) and kept non-collider lifecycle `RunReinitCycleChecks(BackendKind)` in `common.cpp`.
- `2026-02-22`: measured S5-3 file-shape change from pre-slice baseline to migrated state: `common.cpp` `4689 -> 1551` lines (`-3138`), `collider_runtime_checks.cpp` `63 -> 3274` lines (`+3211`).
- `2026-02-22`: completed S5-3 validation green in `build-cln-s5`: `./abuild.py -c -d build-cln-s5 -b jolt,physx`, `./scripts/test-engine-backends.sh build-cln-s5`, and `ctest --test-dir build-cln-s5 -R "physics_backend_parity_jolt|physics_backend_parity_physx" --output-on-failure`.
- `2026-02-22`: executed S5-4 migration by moving remaining common-owned parity checks `RunRuntimeCommandTraceClassificationChecks()` and `RunScenePhysicsComponentContractChecks()` from `src/physics/tests/parity/common.cpp` to `src/physics/tests/parity/ecs_sync_checks.cpp`, preserving `RunEcsSyncCheckSuite(BackendKind)` as the ECS domain entrypoint and keeping parity CLI/backend selection behavior unchanged.
- `2026-02-22`: S5-4 helper boundary outcome: no new shared helper surface introduced; the moved functions were transferred verbatim into `ecs_sync_checks.cpp` and now consume existing ECS/runtime-command includes and local utilities already owned by that domain unit.
- `2026-02-22`: measured S5-4 file-shape change from pre-slice baseline to migrated state: `common.cpp` `1551 -> 662` lines (`-889`), `ecs_sync_checks.cpp` `2137 -> 3028` lines (`+891`).
- `2026-02-22`: completed S5-4 validation green in `build-cln-s5`: `./abuild.py -c -d build-cln-s5 -b jolt,physx`, `./scripts/test-engine-backends.sh build-cln-s5`, and `ctest --test-dir build-cln-s5 -R "physics_backend_parity_jolt|physics_backend_parity_physx" --output-on-failure`.
- `2026-02-22`: executed CLN-S5 closeout decision path `1` (docs-only): keep remaining `common.cpp` checks as intentional shared core (`RunLifecycleChecks`, `RunReinitCycleChecks`, `RunBackendSelectionChecks`, `RunFacadeScaffoldChecks`) and close the lane without further residual split.
- `2026-02-22`: closeout rationale: remaining checks are cross-domain preflight/scaffold responsibilities rather than concentrated domain families. `RunBackendSelectionChecks` is a CLI-level preflight contract invoked directly by `parity/main.cpp` before backend-domain suites, while `RunLifecycleChecks` + `RunReinitCycleChecks` compose a compact shared lifecycle baseline and `RunFacadeScaffoldChecks` anchors the facade-wide trace/scaffold contract. Additional extraction would add wiring churn with limited maintainability gain and higher parity drift risk.
- `2026-02-22`: required closeout evidence captured: `wc -l src/physics/tests/parity/common.cpp` -> `662`; `rg -n "^bool Run[A-Za-z0-9_]+\\(" src/physics/tests/parity/common.cpp` -> `RunLifecycleChecks`, `RunReinitCycleChecks`, `RunBackendSelectionChecks`, `RunFacadeScaffoldChecks`; `./agent/scripts/lint-projects.sh` -> `OK`.

## Open Questions
- None for CLN-S5. Residual follow-up, if any, should be managed by cleanup parent sequencing as a new lane.

## Handoff Checklist
- [x] `S5-1`: `RunEcsSyncSystemPolicyChecks` domain family migrated from `common.cpp` into `ecs_sync_checks.cpp` with local helpers.
- [x] `S5-2`: engine-sync domain family migrated into `engine_sync_checks.cpp` with local helper ownership.
- [x] `S5-2` validation run green in `build-cln-s5`.
- [x] `S5-3`: collider/runtime domain family migrated into `collider_runtime_checks.cpp` with local helper ownership.
- [x] `S5-3` validation run green in `build-cln-s5`.
- [x] `S5-4`: migrated `RunRuntimeCommandTraceClassificationChecks` and `RunScenePhysicsComponentContractChecks` from `common.cpp` into `ecs_sync_checks.cpp`.
- [x] `S5-4` validation run green in `build-cln-s5`.
- [x] `common.cpp` no longer contains the majority of suite logic.
- [x] Domain files own migrated domain checks and local helpers.
- [x] Parity binary behavior and backend CLI semantics unchanged.
- [x] `S5` closeout: remaining `common.cpp` ownership finalized as intentional shared core and CLN-S5 marked complete.
