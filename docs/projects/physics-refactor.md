# Physics Refactor (KARMA Alignment)

## Project Snapshot
- Current owner: `specialist-physics-refactor`
- Status: `in progress` (Phase 4h validated in `build-a3`; Jolt lock-assert blocker closed with deterministic invalid-intent contract for dynamic both-lock configuration)
- Supersedes: `docs/projects/physics-backend.md` (retired to `docs/archive/physics-backend-retired-2026-02-17.md`)
- Immediate next task: execute a bounded Phase 4 runtime-mutation follow-up slice for backend-neutral runtime material-property parity (`friction`/`restitution`) and deterministic ECS fallback behavior.
- Validation gate: `./scripts/test-engine-backends.sh <build-dir>`

## Mission
Align `m-rewrite` physics architecture with the direction proven in `KARMA-REPO`: world-level physics API, collider/component-driven sync, controller-based character collision, static mesh collision intake, and backend-equivalent behavior across compiled physics backends.

The goal is behavioral and architectural alignment, not literal file-for-file copying.

## Foundation References
- `docs/foundation/architecture/core-engine-contracts.md` (physics sections)
- `docs/foundation/governance/engine-backend-testing.md`
- `docs/foundation/policy/decisions-log.md` (BodyId core + optional facade direction)
- `KARMA-REPO/include/karma/physics/*`
- `KARMA-REPO/src/physics/*`
- `KARMA-REPO/examples/main.cpp`

## Why This Is Separate
- Existing rewrite physics work is mostly backend-core parity (`BodyId`, gravity flag, locks, raycast).
- KARMA-aligned intake requires wider architectural changes (public API shape, ECS component model, sync system, backend capability depth, engine loop wiring).
- This is a major foundation realignment and should be tracked as a dedicated project.

## Owned Paths
- `m-rewrite/include/karma/physics/*`
- `m-rewrite/src/engine/physics/*`
- `m-rewrite/src/engine/physics/tests/*`
- `m-rewrite/include/karma/app/{client,server,shared}/*` and `m-rewrite/src/engine/app/{client,server,shared}/*` (physics-facing lifecycle hooks)
- `m-rewrite/include/karma/scene/*` and `m-rewrite/src/engine/scene/*` (if physics components/sync hooks land here)
- `m-rewrite/src/engine/CMakeLists.txt`
- `m-rewrite/docs/projects/physics-refactor.md`
- `m-rewrite/docs/projects/ASSIGNMENTS.md`

## Direction Lock
- We accept intentional breakage in currently migrated gameplay code while refactoring this foundation.
- Priority is physics architecture alignment first; game repair follows afterward.

## KARMA Intake Checklist (Definition of Alignment)
- [ ] Physics-backed world API supports dynamic rigid bodies, static mesh collision, player controller creation, gravity control, and raycast.
- [ ] ECS physics sync system owns collider-to-physics object lifecycle and transform/velocity write-back.
- [ ] Collider component model includes box/sphere/capsule/mesh and trigger semantics.
- [ ] Player controller component + backend path supports collision-driven movement and grounded behavior.
- [ ] Player controller shape/size updates are detected and reconciled (including controlled recreation when required).
- [ ] Static world geometry collision can be created from mesh assets through physics backend path.
- [ ] Collision enablement/filter intent is represented at component level and honored by sync path.
- [ ] Cleanup of stale physics objects (destroyed/changed entities) is deterministic.
- [ ] Both compiled physics backends expose equivalent contract behavior for these capabilities.
- [ ] A demo-level scenario equivalent to `KARMA-REPO/examples/main.cpp` physics flow is possible in rewrite without backend leakage.

## Target Architecture in `m-rewrite`

### 1) Public Gameplay Physics Surface
- Introduce/restore a KARMA-like public surface under `karma::physics`:
  - `World`
  - `RigidBody`
  - `StaticBody`
  - `PlayerController`
- Maintain overlap with KARMA interface shape where practical:
  - `createBoxBody(...)`
  - `createStaticMesh(...)`
  - `createPlayer(...)`
  - `playerController()`
  - `raycast(...)`
- Keep backend specifics hidden from game code.

### 2) Core Backend-Neutral Layer
- Preserve a backend-neutral core (existing BodyId-backed capabilities are useful and should be retained as internal substrate).
- Normalize layering so higher-level world/component flows map onto this substrate.
- Keep deterministic invalid-input and stale-handle behavior from current parity work.

### 3) ECS/Scene Physics Component Layer
- Add/normalize physics data components equivalent to KARMA intent:
  - rigidbody
  - colliders (box/sphere/capsule/mesh)
  - player controller intent
  - collision visibility/layer mask intent
- Decide final namespace ownership (`scene` vs dedicated `components`) early and keep consistent.

### 4) Physics Sync System
- Implement KARMA-style sync responsibilities:
  - create/update/destroy runtime rigid bodies from ECS components
  - create static mesh collision from mesh components
  - maintain player controller runtime object from controller + collider components
  - apply dirty transform pushes and pull simulated state back to ECS
  - enforce collision-enabled gating and stale cleanup

### 5) Engine Loop Integration
- Ensure game-facing fixed-step hooks can drive physics-intent updates before physics simulation.
- Ensure physics simulation and ECS sync order is explicit and deterministic.
- Keep backend selection/runtime ownership policy consistent with existing engine architecture.

## Carried-Forward Assets From Retired `physics-backend.md`
- Existing backend selection/lifecycle scaffolding is already working.
- Jolt + PhysX compiled backend parity harness exists and should remain authoritative.
- Implemented parity slices to preserve:
  - closest-hit raycast query path
  - dynamic gravity enable flag API
  - creation-time rotation lock behavior
  - creation-time translation lock behavior
- Existing explicit TODOs to carry forward into new contracts:
  - standardized start-inside ray semantics
  - filter/layer query contract design
  - optimize native-hit to logical-id mapping where needed

These are not discarded; they become the lower-layer regression base for the refactor.

## Execution Plan

### Phase 0: Contract Reset and Migration Scaffolding
1. Freeze direction and publish breaking-change intent in docs/trace notes.
2. Define target public physics API (KARMA-overlap) and compatibility stance for existing `PhysicsSystem` usage.
3. Decide whether current BodyId API remains internal-only or dual-exposed during transition.
4. Add migration TODO matrix mapping old interfaces/tests to new layers.

Exit criteria:
- `physics-refactor.md` accepted as authoritative plan.
- No ambiguity on public API target and transition path.

### Phase 1: Public World/Body/Controller API Introduction
1. Add `World`, `RigidBody`, `StaticBody`, `PlayerController` wrappers in rewrite.
2. Expose world-level calls overlapping KARMA behavior where sensible.
3. Keep backend abstraction hidden and backend-neutral.
4. Add compile-time and runtime guards for invalid lifecycle usage.

Exit criteria:
- New public physics API compiles and can be instantiated.
- No backend objects leak through public headers.

### Phase 2: Component Model Expansion
1. Introduce physics component set (rigidbody/collider/controller/visibility mask intent).
2. Ensure component validation constraints are explicit (for example controller requires compatible collider).
3. Define authoritative ownership for transform fields and dirty-state semantics.

Exit criteria:
- Components exist, are serializable where required, and have documented invariants.

### Phase 3: ECS Sync System (KARMA-Style Behavior)
1. Implement rigid-body sync from components.
2. Implement static mesh collider creation path.
3. Implement player-controller sync path, including shape/size-change handling.
4. Implement simulated-state write-back and deterministic stale cleanup.

Exit criteria:
- Entity/component-driven collision behavior is active.
- Controller-driven movement can collide against static mesh collision.

### Phase 4: Backend Capability Depth (Jolt + PhysX)
1. Ensure backend adapters support required world/controller/static-mesh capabilities.
2. Normalize grounded, trigger, kinematic, gravity semantics at contract level.
3. Ensure shape handling and recreation semantics are equivalent across backends.
4. Keep deterministic failure mapping and trace quality.

Exit criteria:
- Both backends pass the same capability-level contract suite for new features.

### Phase 5: Engine and Game-Facing Integration
1. Wire fixed-step game hooks and physics intent timing as needed for controller flow.
2. Replace ad-hoc gameplay collision approximations with physics-backed paths.
3. Accept temporary game breakage while foundation lands; fix-up can follow as separate slices.

Exit criteria:
- Engine supports KARMA-style controller/collider update flow end-to-end.

### Phase 6: Validation, Parity, and Stabilization
1. Keep existing backend parity tests green for preserved low-level contracts.
2. Add/refine higher-level tests for:
  - ECS sync lifecycle
  - controller collision/grounding
  - static mesh collision intake
  - collider mutation/recreation behavior
  - collision mask/filter semantics
3. Add scenario smoke equivalent to KARMA demo collision behavior.

Exit criteria:
- Lower-layer parity + higher-layer KARMA-aligned behavior tests pass in both backends.

## Required Validation Commands
From `m-rewrite/`:

```bash
./abuild.py -c --test-physics -d <build-dir> -b jolt,physx
./scripts/test-engine-backends.sh <build-dir>
./docs/scripts/lint-project-docs.sh
```

## Trace Channels
- `engine.app`
- `engine.server`
- `engine.sim`
- `engine.sim.frames`
- `physics.system`
- `physics.jolt`
- `physics.physx`

## Open Decisions (Resolve Early)
- Keep existing `physics::PhysicsSystem` type name as backend substrate, or repurpose it as ECS sync system and rename substrate.
- Whether to preserve BodyId API as supported public advanced path or internal-only implementation detail.
- Component namespace location for rewrite physics components.
- Exact collision-layer/filter contract shape for first aligned slice.
- Runtime lock mutation/query policy in the new layered API.

## Phase 0/1 Contract Slice (2026-02-18)
Landed in this bounded slice:
- Public KARMA-aligned facade surface added under `include/karma/physics/*`:
  - `World`
  - `RigidBody`
  - `StaticBody`
  - `PlayerController`
  - `PhysicsMaterial` (`types.hpp`)
- Engine-side facade scaffolding added under `src/engine/physics/*`, mapping facade calls onto existing `PhysicsSystem` + `BodyId` substrate.
- Existing `PhysicsSystem` parity API remains intact and supported (additive compatibility model, no removal).
- Backend internals remain hidden from the new public facade headers.
- Parity harness now includes a bounded world-facade smoke path (`RunFacadeScaffoldChecks`) to prove compile/link/runtime surface viability per backend.

Intentionally deferred (explicit non-goals for this slice):
- Real static-mesh collision ingestion from mesh assets (`createStaticMesh` currently uses placeholder static-body scaffolding).
- Controller grounded/collision semantics and shape reconciliation policies.
- True half-extents/material propagation into backend shape creation.
- ECS component model and sync-system migration (Phase 2/3 work).

## Phase 2a Component Contract Slice (2026-02-18)
Landed in this bounded slice:
- Scene-owned physics component contracts introduced under `include/karma/scene/physics_components.hpp` and re-exported via `include/karma/scene/components.hpp`.
- Added backend-neutral component data contracts for:
  - rigidbody intent,
  - collider intent (shape + trigger + enabled + collision layer/filter intent),
  - player-controller intent,
  - collision mask intent.
- Added bounded validation helpers for component contracts (finite-value checks, positive-dimension checks, mesh-path non-empty checks, collision-mask checks).
- Added parity-test coverage for:
  - default validity,
  - invalid-input rejection,
  - ECS storage/view behavior for new scene physics components.
- Closed carry-over world-facade issues:
  - borrowed-system `World::shutdown()` no longer shuts down externally provided `PhysicsSystem`,
  - `World::raycast(...)` no longer fabricates normals; phase contract now returns trustworthy hit position and zeros the normal field.

Explicit deferrals for next phases:
- `Phase 2b`:
  - component-level transform ownership/dirty-state policy,
  - shape-specific mutation/reconciliation policy (controller+collider coupling rules),
  - collision filter/layer contract tightening beyond current mask baseline.
- `Phase 3`:
  - ECS physics sync system implementation (create/update/destroy runtime objects, write-back ordering, stale cleanup),
  - static mesh ingestion pipeline hookup,
  - controller grounded/collision-driven runtime behavior.

## Phase 2b Policy Hardening Slice (2026-02-18)
Landed in this bounded slice:
- Added explicit transform ownership contract utilities in `include/karma/scene/physics_components.hpp`:
  - authority model (`scene-authoritative` vs `physics-authoritative`),
  - coherent dirty/push/pull validation helper for ownership intent,
  - helper predicates for scene->physics push and physics->scene pull policy decisions.
- Added shape-specific collider reconciliation policy:
  - reconcile action classifier (`no-op`, `runtime property update`, `runtime shape rebuild`, `reject invalid`),
  - shape-parameter diff handling keyed by active collider shape contract.
- Added controller/collider compatibility classification policy:
  - compatibility states for missing/invalid/disabled/trigger/unsupported-shape cases,
  - helper to collapse classification to a bounded compatible/not-compatible decision.
- Added bounded parity coverage for all Phase 2b policies:
  - transform ownership validation + helper behavior,
  - collider reconcile-action classification,
  - controller/collider compatibility classification.

Explicit Phase 3 deferrals after Phase 2b closure:
- No ECS sync-system runtime wiring yet (no runtime create/update/destroy from ECS components).
- No runtime reconcile execution against backend objects yet (policy helpers are contract-level only).
- No gameplay/controller integration or grounded-runtime behavior changes yet.

## Phase 3 ECS Sync Slice 1 (2026-02-18)
Landed in this bounded slice:
- Added `src/engine/physics/ecs_sync_system.hpp/.cpp` with deterministic two-phase flow:
  - pre-sim: validate intents, reconcile runtime bodies (create/rebuild/update/teardown), apply scene->physics transform push by ownership policy.
  - post-sim: apply physics->scene transform pull by ownership policy.
- Consumes Phase 2 policy helpers from `include/karma/scene/physics_components.hpp`:
  - `ValidateRigidBodyIntent`, `ValidateColliderIntent`, `ValidateTransformOwnership`,
  - `ClassifyColliderReconcileAction`,
  - `ShouldPushSceneTransformToPhysics`, `ShouldPullPhysicsTransformToScene`,
  - `ClassifyControllerColliderCompatibility` / `IsControllerColliderCompatible`.
- Runtime support in this slice is explicitly Box-only for colliders; unsupported/invalid collider intent is deterministically rejected/teardown.
- Added deterministic stale cleanup for destroyed entities and entities that no longer satisfy required component sets.
- Added bounded sync-system parity checks in `physics_backend_parity_test.cpp` per backend for:
  - create on valid entity,
  - rebuild on shape-parameter mutation,
  - teardown on invalid intent,
  - scene-authoritative push behavior,
  - physics-authoritative pull behavior,
  - cleanup after entity destroy.
- Added minimal sync-system introspection helpers used by parity tests (binding existence/count, runtime body lookup, transform snapshot).

Explicit remaining deferrals after this Phase 3 slice:
- No static-mesh ingestion/runtime path yet.
- No controller runtime object lifecycle or grounded behavior implementation yet.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.
- No non-Box collider runtime ingestion (unsupported shapes remain deterministic reject/teardown in this slice).

## Phase 3 ECS Sync Slice 2 (2026-02-18)
Landed in this bounded slice:
- Extended `EcsSyncSystem` runtime lifecycle behavior to include static mesh placeholder support under policy constraints:
  - `ColliderShapeKind::Mesh` now participates in runtime lifecycle for static intent (`dynamic=false`),
  - runtime object creation remains placeholder/substrate-backed (no mesh-geometry ingestion yet),
  - invalid transitions (for example mesh + dynamic=true) deterministically teardown runtime state.
- Added bounded controller lifecycle metadata wiring in ECS sync:
  - compatibility transitions are classified each pre-sim tick using Phase 2 policy helpers,
  - compatible states create/update controller runtime metadata bound to the entity,
  - incompatible states deterministically teardown runtime body + controller metadata,
  - controller component removal clears controller metadata while preserving body lifecycle when otherwise valid.
- Closed the remaining `ColliderReconcileAction::UpdateRuntimeProperties` behavior gap for enabled-state transitions:
  - collider `enabled=false` now deterministically maps to teardown/no-runtime,
  - re-enable (`enabled=true`) deterministically recreates runtime body on the next valid pre-sim pass.
- Added sync introspection helpers for controller metadata state, used only by parity tests.
- Added bounded parity coverage per backend for:
  - mesh-placeholder lifecycle create/teardown/recreate behavior,
  - controller compatibility transition lifecycle effects,
  - collider enabled toggle teardown/recreate behavior,
  - stale cleanup invariants after component mutation and entity destroy.

Explicit remaining deferrals after this Phase 3 follow-up:
- No real static-mesh collision ingestion pipeline (placeholder mesh runtime path only).
- No backend-native player-controller runtime object or grounded/movement behavior.
- No backend mutation path yet for trigger/filter `UpdateRuntimeProperties` transitions beyond deterministic caching/deferral.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4a Substrate Hooks Slice (2026-02-18)
Landed in this bounded slice:
- Extended substrate contracts in `include/karma/physics/backend.hpp` and `include/karma/physics/physics_system.hpp`:
  - `BodyDesc` now carries backend-neutral collider shape descriptor (`Box`/`Sphere`/`Capsule`) and shape parameters.
  - Added backend-neutral runtime collider property APIs for trigger state and collision layer/filter mask (`set/get`).
- Implemented `PhysicsSystem` passthroughs in `src/engine/physics/physics_system.cpp` for the new runtime collider property APIs.
- Implemented backend behavior in `backend_jolt_stub.cpp` and `backend_physx_stub.cpp`:
  - `createBody` now consumes shape descriptor for `Box`/`Sphere`/`Capsule` creation.
  - runtime trigger/filter APIs now expose deterministic success/failure and coherent reported state.
  - bounded deterministic fallback behavior is explicit where runtime mutation is unsupported:
    - Jolt reports unsupported runtime collision-mask transition mutations (enabling caller-driven deterministic fallback).
- Updated ECS sync (`src/engine/physics/ecs_sync_system.cpp`) to:
  - pass collider shape parameters + trigger/filter intent into `BodyDesc` during runtime body creation,
  - execute `UpdateRuntimeProperties` via new substrate APIs,
  - preserve deterministic enabled teardown/recreate behavior,
  - apply deterministic rebuild fallback when runtime trigger/filter mutation reports unsupported.
- Added parity coverage in `physics_backend_parity_test.cpp` for:
  - Box/Sphere/Capsule creation path sanity,
  - runtime trigger/filter mutation roundtrip contracts,
  - ECS sync trigger/filter transitions with both success and deterministic fallback paths.

Explicit remaining deferrals after Phase 4a:
- No real static-mesh geometry ingestion pipeline (mesh runtime path remains placeholder-backed).
- No backend-native player-controller runtime object or grounded/movement behavior.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4b Controller Ingestion Depth Slice (2026-02-18)
Landed in this bounded slice:
- Extended substrate contracts in `include/karma/physics/backend.hpp` and `include/karma/physics/physics_system.hpp` with backend-neutral runtime velocity APIs:
  - `set/get` linear velocity,
  - `set/get` angular velocity,
  - deterministic `false` on invalid/unknown/non-dynamic bodies.
- Implemented `PhysicsSystem` passthrough methods in `src/engine/physics/physics_system.cpp` for the new runtime velocity APIs.
- Implemented backend support in `backend_jolt_stub.cpp` and `backend_physx_stub.cpp`:
  - valid dynamic bodies support linear/angular velocity set/get,
  - static/invalid/unknown/destroyed bodies deterministically return failure.
- Updated ECS sync (`src/engine/physics/ecs_sync_system.hpp/.cpp`) controller path:
  - compatible controller entities now push `desired_velocity` to runtime linear velocity each pre-sim pass,
  - `PlayerControllerIntentComponent.enabled == false` applies deterministic bounded behavior by pushing zero linear velocity effect while keeping controller metadata.
- Added parity coverage in `physics_backend_parity_test.cpp` for:
  - runtime velocity API roundtrip behavior,
  - invalid/unknown/destroyed/static rejection behavior,
  - ECS sync desired-velocity application for enabled vs disabled controller states.

Explicit remaining deferrals after Phase 4b:
- No backend-native player-controller runtime object.
- No grounded/jump/controller collision behavior implementation.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4c Controller-Runtime Parity Slice (2026-02-18)
Landed in this bounded slice:
- Extended scene component policy contracts in `include/karma/scene/physics_components.hpp`:
  - added explicit controller velocity ownership policy helper (`ControllerVelocityOwnership` + classifier helpers),
  - extended controller/collider compatibility classifier to reject enabled controller intent on non-dynamic rigidbodies (`EnabledControllerRequiresDynamicRigidBody`).
- Updated ECS sync runtime behavior in `src/engine/physics/ecs_sync_system.hpp/.cpp`:
  - unified create/update runtime velocity application through policy-driven ownership,
  - enabled + compatible controller now owns runtime linear velocity (`desired_velocity`) and enforces zero runtime angular velocity,
  - disabled or absent controller now defers linear+angular runtime velocity ownership to rigidbody intent,
  - deterministic teardown/rebuild fallback remains in place when runtime velocity mutation fails,
  - enabled-controller/non-dynamic-rigidbody policy-incompatible configurations no longer churn runtime bodies and do not retain controller runtime metadata bindings.
- Added bounded parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp` for:
  - enabled -> disabled -> enabled controller velocity ownership transitions,
  - angular-velocity ownership expectations under controller vs rigidbody ownership,
  - explicit incompatible classification for enabled controller + non-dynamic rigidbody,
  - repeated pre-sim stability for rejected controller/non-dynamic configurations (no controller runtime binding retention).

Explicit remaining deferrals after Phase 4c:
- No backend-native player-controller runtime object.
- No grounded/jump/controller collision behavior implementation.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4d Controller-Geometry Reconcile Slice (2026-02-18)
Landed in this bounded slice:
- Extended scene policy contracts in `include/karma/scene/physics_components.hpp`:
  - added controller-geometry reconcile contract helper (`NoOp` / `RebuildRuntimeShape` / `RejectInvalidIntent`).
- Updated ECS sync behavior in `src/engine/physics/ecs_sync_system.hpp/.cpp`:
  - controller geometry mutations (`half_extents`, `center`) now trigger deterministic runtime rebuild when required.
  - enabled + compatible controllers now derive runtime collider geometry from controller dimensions for Box/Capsule runtime creation (bounded mapping), rather than stale collider dimensions.
  - geometry-driven rebuilds preserve runtime transform and linear/angular velocity.
  - deterministic fallback behavior for rebuild/update failure paths remains intact.
- Added bounded parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp` for:
  - rebuild on controller geometry mutation,
  - no rebuild when geometry is unchanged,
  - runtime transform/velocity preservation across controller-geometry rebuild.

Explicit remaining deferrals after Phase 4d:
- No backend-native player-controller runtime object.
- No grounded/jump/controller collision behavior implementation.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4e Controller Motion Parity Slice (2026-02-18)
Landed in this bounded slice:
- Reused scene contract helpers in `include/karma/scene/physics_components.hpp` for controller-runtime velocity composition and upward jump applicability (`HasControllerJumpUpwardIntent`, `ComposeControllerRuntimeLinearVelocity`) and added bounded contract assertions in parity tests.
- Updated ECS sync controller velocity ownership behavior in `src/engine/physics/ecs_sync_system.cpp`:
  - enabled + compatible controller now preserves current runtime linear `y`, applies controller `desired_velocity.xz`, and enforces zero angular velocity,
  - one-shot jump is applied only when `jump_requested` is true with positive upward intent, and `jump_requested` is consumed only after successful runtime velocity writes,
  - disabled/absent/incompatible controller paths remain rigidbody-owned for linear+angular velocity and do not consume jump intent,
  - runtime linear-velocity read/write failure still maps to deterministic teardown behavior.
- Added bounded parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp` for:
  - enabled-controller vertical-velocity preservation (no per-frame `y` clobber),
  - one-shot jump application + consumption,
  - disabled/incompatible controller non-consumption of jump intent.

Explicit remaining deferrals after Phase 4e:
- No backend-native player-controller runtime object.
- No grounded/controller collision response behavior (jump handling is velocity-intent only in this slice).
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4e Boundary Correction Slice (2026-02-18)
Landed in this bounded slice:
- Removed jump-specific APIs from engine scene-physics contracts in `include/karma/scene/physics_components.hpp`:
  - removed `jump_requested` from `PlayerControllerIntentComponent`,
  - removed jump-specific helper APIs (`HasControllerJumpUpwardIntent`, `ComposeControllerRuntimeLinearVelocity`).
- Updated ECS sync in `src/engine/physics/ecs_sync_system.cpp`:
  - removed all jump read/consume/mutate behavior,
  - restored engine-generic controller velocity ownership behavior: enabled + compatible controller writes `desired_velocity` and zero angular velocity; disabled/absent/incompatible paths stay rigidbody-owned.
- Updated parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp`:
  - removed jump behavior assertions/helper checks,
  - retained engine-generic controller/rigidbody velocity ownership and compatibility lifecycle checks.

Explicit remaining deferrals after Phase 4e boundary correction:
- No backend-native player-controller runtime object.
- No grounded/controller movement gameplay semantics in engine physics contracts.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4f Collider Local-Offset Intake Slice (2026-02-18)
Landed in this bounded slice:
- Extended backend-neutral substrate shape descriptor in `include/karma/physics/backend.hpp` with `ColliderShapeDesc.local_center` (create-time collider local offset).
- Implemented offset-aware shape creation in both compiled backends:
  - `src/engine/physics/backends/backend_jolt_stub.cpp`: wraps Box/Sphere/Capsule shapes in a translated shape when `local_center` is non-zero.
  - `src/engine/physics/backends/backend_physx_stub.cpp`: applies `PxShape::setLocalPose` from `local_center` for Box/Sphere/Capsule.
- Kept deterministic invalid-input behavior for the new offset path by rejecting non-finite local-center input in both backends.
- Updated ECS sync runtime shape build path in `src/engine/physics/ecs_sync_system.cpp`:
  - removed controller-center placeholder extents expansion,
  - controller geometry now uses true dimensions (`half_extents`) plus `local_center = controller.center` in the substrate `BodyDesc`.
- Added parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp` for:
  - backend-level shape-offset collision-query behavior consistency (including invalid local-offset rejection),
  - ECS controller-center mutation behavior proving underside query shifts in the offset direction across rebuild (distinguishes true offset from extents-expansion workaround).

Explicit remaining deferrals after Phase 4f:
- No backend-native player-controller runtime object.
- No grounded/controller movement gameplay semantics in engine physics contracts.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4g Runtime Damping Mutation Parity Slice (2026-02-18)
Landed in this bounded slice:
- Extended scene rigidbody intent contract in `include/karma/scene/physics_components.hpp`:
  - added `linear_damping` + `angular_damping`,
  - validation now enforces finite and non-negative damping values.
- Extended backend-neutral substrate contracts in `include/karma/physics/backend.hpp` + `include/karma/physics/physics_system.hpp`:
  - `BodyDesc` now carries create-time linear/angular damping,
  - added runtime damping APIs (`set/get` linear damping, `set/get` angular damping).
- Implemented `PhysicsSystem` passthroughs in `src/engine/physics/physics_system.cpp` for damping APIs.
- Implemented backend runtime damping behavior in `backend_jolt_stub.cpp` + `backend_physx_stub.cpp`:
  - create-time damping is consumed for dynamic bodies,
  - runtime damping set/get works for valid dynamic bodies,
  - deterministic `false` is returned for invalid/unknown/non-dynamic bodies and invalid damping values.
- Updated ECS sync in `src/engine/physics/ecs_sync_system.cpp`:
  - rigidbody damping is applied in runtime body creation via `BodyDesc`,
  - valid damping intent mutations are applied through runtime substrate APIs without forcing body rebuild/churn,
  - runtime damping mutation failure follows deterministic teardown behavior.
- Added bounded parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp` for:
  - damping API roundtrip + invalid/static rejection behavior,
  - ECS damping mutation updates without body-id churn,
  - scene rigidbody damping validation contract checks.

Explicit remaining deferrals after Phase 4g:
- No backend-native player-controller runtime object.
- No grounded/controller movement gameplay semantics in engine physics contracts.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Phase 4h Runtime Motion-Lock Mutation Parity Slice (2026-02-18)
Landed in this bounded slice:
- Extended backend-neutral substrate contracts in `include/karma/physics/backend.hpp` + `include/karma/physics/physics_system.hpp`:
  - added runtime motion-lock APIs (`set/get` rotation lock, `set/get` translation lock).
- Implemented `PhysicsSystem` passthroughs in `src/engine/physics/physics_system.cpp` for motion-lock APIs.
- Implemented backend runtime behavior:
  - `src/engine/physics/backends/backend_physx_stub.cpp`: true runtime lock mutation for dynamic bodies using PhysX lock flags.
  - `src/engine/physics/backends/backend_jolt_stub.cpp`: deterministic unsupported runtime mutation for lock changes (`false`), with coherent lock-state query support for current runtime state.
- Updated ECS sync in `src/engine/physics/ecs_sync_system.cpp`:
  - lock transitions are no longer unconditional rebuild triggers,
  - runtime lock mutation is attempted first,
  - deterministic rebuild fallback is applied on unsupported/failure paths (no silent no-op).
- Added bounded parity coverage in `src/engine/physics/tests/physics_backend_parity_test.cpp`:
  - runtime motion-lock API behavior (roundtrip where supported, deterministic unsupported semantics where not),
  - invalid/unknown/non-dynamic rejection checks,
  - ECS lock-mutation behavior (runtime update on supported backend, deterministic rebuild fallback on unsupported backend).
- Phase 4h blocker closure landed:
  - dynamic bodies with both `rotation_locked` and `translation_locked` are now engine-contract invalid intent,
  - scene intent validation rejects this combination before runtime (`ConflictingMotionLocks`),
  - Jolt/PhysX backends reject create/mutation paths deterministically for this combination,
  - ECS sync parity checks now assert teardown/stable rejection/recovery behavior for invalid both-lock intent.

Explicit remaining deferrals after Phase 4h:
- No backend-native player-controller runtime object.
- No grounded/controller movement gameplay semantics in engine physics contracts.
- No real static-mesh geometry ingestion pipeline.
- No engine loop integration in `src/engine/app/*`.
- No gameplay migration wiring.

## Current Status
- `2026-02-17`: Project created as full replacement for backend-only parity track.
- `2026-02-17`: `physics-backend.md` retired and subsumed into this plan.
- Existing backend-core parity work is preserved as foundational input, not discarded.
- `2026-02-18`: Phase 0/1 bounded scaffolding slice landed (`World`/`RigidBody`/`StaticBody`/`PlayerController` facade + minimal engine implementation over `PhysicsSystem`).
- `2026-02-18`: Required validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 2a landed:
  - scene physics component contracts + validation helpers,
  - parity coverage for component validity/invalid-input/ECS-view behavior,
  - world facade shutdown ownership and raycast-normal contract fixes.
- `2026-02-18`: Phase 2a validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 2b landed:
  - transform authority ownership contract + validation helpers,
  - shape-specific collider reconcile-action classification policy,
  - controller/collider compatibility classification policy,
  - bounded parity coverage for all Phase 2b policy helpers.
- `2026-02-18`: Phase 2b validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 3 ECS sync slice 1 landed:
  - new `EcsSyncSystem` runtime added with deterministic pre/post simulation ordering,
  - policy-driven runtime lifecycle + transform push/pull behavior implemented for Box colliders,
  - parity coverage added for create/rebuild/teardown/push/pull/cleanup behavior across Jolt/PhysX.
- `2026-02-18`: Phase 3 slice 1 validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 3 ECS sync slice 2 landed:
  - static mesh placeholder lifecycle added for `dynamic=false` mesh collider intent,
  - controller compatibility-driven runtime metadata lifecycle added,
  - deterministic `enabled` teardown/recreate behavior landed for `UpdateRuntimeProperties`.
- `2026-02-18`: Phase 3 slice 2 validation completed in `build-a3`:
  - `./abuild.py --lock-status -d build-a3` (owner verified)
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 4a substrate hooks slice landed:
  - backend-neutral collider-shape descriptor + runtime trigger/filter APIs added to substrate contract,
  - Jolt/PhysX substrate implementations updated for shape creation and deterministic runtime property mutation behavior,
  - ECS sync now applies trigger/filter runtime transitions through substrate APIs with deterministic rebuild fallback path.
- `2026-02-18`: Phase 4a validation completed in `build-a3`:
  - `./abuild.py --lock-status -d build-a3` (owner verified)
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 4b controller-ingestion depth slice landed:
  - backend-neutral runtime linear/angular velocity APIs added to substrate and `PhysicsSystem`,
  - Jolt/PhysX velocity runtime support landed with deterministic invalid/unknown/static rejection semantics,
  - ECS sync now applies controller desired velocity to runtime bodies for compatible enabled controllers and pushes zero effect when disabled.
- `2026-02-18`: Phase 4b validation completed in `build-a3`:
  - `./abuild.py --lock-status -d build-a3` (owner verified)
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
- `2026-02-18`: Phase 4c controller-runtime parity slice landed:
  - controller velocity ownership policy helpers added to scene contracts,
  - ECS sync now deterministically applies controller-vs-rigidbody velocity ownership (linear + angular) in both create/update paths,
  - enabled-controller/non-dynamic-rigidbody combinations are now policy-incompatible without runtime churn.
- `2026-02-18`: Phase 4c validation completed in `build-a3`:
  - `if [ -x ./vcpkg/vcpkg ] || [ -x ./vcpkg/bootstrap-vcpkg.sh ] || [ -f ./vcpkg/.bootstrap-complete ]; then echo VCPKG_BOOTSTRAPPED; else echo VCPKG_MISSING; fi` (`VCPKG_BOOTSTRAPPED`)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4d controller-geometry reconcile slice landed:
  - controller-geometry reconcile policy helper added (`NoOp`/`RebuildRuntimeShape`/`RejectInvalidIntent`),
  - ECS sync now rebuilds deterministically on controller geometry mutation and derives runtime Box/Capsule geometry from controller dimensions for enabled compatible controllers,
  - runtime transform and runtime linear/angular velocity are preserved across controller-geometry-triggered rebuilds.
- `2026-02-18`: Phase 4d validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4e controller-motion parity slice landed:
  - ECS sync controller-owned velocity updates now preserve runtime `y`, apply `desired_velocity.xz`, and keep angular velocity zero,
  - one-shot upward jump intent is applied and consumed deterministically only on successful runtime writes,
  - disabled/incompatible controller states keep rigidbody-owned runtime velocity and do not consume jump intent.
- `2026-02-18`: Phase 4e validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4e boundary correction landed:
  - removed `jump_requested` and jump-specific helper APIs from engine scene-physics contracts,
  - removed jump consume/mutate behavior from ECS sync,
  - parity checks now cover engine-generic controller/rigidbody velocity ownership without jump semantics.
- `2026-02-18`: Phase 4e boundary-correction validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4f collider local-offset intake slice landed:
  - substrate shape descriptor now carries backend-neutral collider local-center offset,
  - Jolt/PhysX runtime shape creation now consumes local-center for Box/Sphere/Capsule,
  - ECS controller-center runtime mapping now uses shape local offset instead of placeholder extents expansion,
  - parity checks now cover backend offset query behavior and ECS controller-center offset rebuild behavior.
- `2026-02-18`: Phase 4f validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4g runtime damping mutation parity slice landed:
  - scene rigidbody intent now carries validated linear/angular damping,
  - substrate/backends now expose runtime damping set/get APIs with dynamic-body deterministic rejection semantics,
  - ECS sync now applies damping on create and valid mutation updates without runtime body-id churn.
- `2026-02-18`: Phase 4g validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- `2026-02-18`: Phase 4h runtime motion-lock mutation parity slice implemented:
  - substrate/backends/`PhysicsSystem` runtime lock APIs added,
  - ECS sync lock-mutation runtime-first + deterministic rebuild fallback path landed,
  - parity tests extended for motion-lock API and ECS fallback behavior.
- `2026-02-18`: Phase 4h blocker-fix follow-up landed:
  - dynamic both-lock motion intent is now explicitly invalid (`ConflictingMotionLocks`) for dynamic rigidbodies,
  - Jolt/PhysX now reject dynamic both-lock create/mutation paths deterministically (prevents Jolt SIGTRAP assert),
  - ECS parity coverage now asserts deterministic invalid-intent teardown and stable reject/recovery behavior.
- `2026-02-18`: Phase 4h validation completed in `build-a3`:
  - `./abuild.py -c --test-physics -d build-a3 -b jolt,physx` (pass)
  - `./scripts/test-engine-backends.sh build-a3` (pass)
  - `./docs/scripts/lint-project-docs.sh` (pass)
  - `./abuild.py --lock-status -d build-a3` (owner verified)
- Next implementation slice: execute a bounded Phase 4 runtime material-property mutation parity follow-up (`friction`/`restitution`) with deterministic substrate/ECS fallback semantics across Jolt/PhysX.

## Handoff Checklist
- [x] `physics-refactor.md` remains the single active physics project doc in `docs/projects/`.
- [x] `ASSIGNMENTS.md` row is updated in same handoff.
- [x] Retired project docs are moved under `docs/archive/`.
- [x] Validation commands/results are recorded for implementation slices.
- [x] Contract decisions and behavior changes are reflected in docs as they land.
