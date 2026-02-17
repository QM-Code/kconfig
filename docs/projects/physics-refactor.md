# Physics Refactor (KARMA Alignment)

## Project Snapshot
- Current owner: `codex`
- Status: `in progress` (scope reset from backend-only parity to full KARMA-aligned physics foundation)
- Supersedes: `docs/projects/physics-backend.md` (retired to `docs/archive/physics-backend-retired-2026-02-17.md`)
- Immediate next task: execute Phase 0/1 contract reset and scaffolding for KARMA-style world + controller + collider flow.
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
- `m-rewrite/include/karma/app/*` and `m-rewrite/src/engine/app/*` (physics-facing lifecycle hooks)
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

## Current Status
- `2026-02-17`: Project created as full replacement for backend-only parity track.
- `2026-02-17`: `physics-backend.md` retired and subsumed into this plan.
- Existing backend-core parity work is preserved as foundational input, not discarded.
- Next implementation slice starts at Phase 0/1 contract reset and scaffolding.

## Handoff Checklist
- [ ] `physics-refactor.md` remains the single active physics project doc in `docs/projects/`.
- [ ] `ASSIGNMENTS.md` row is updated in same handoff.
- [ ] Retired project docs are moved under `docs/archive/`.
- [ ] Validation commands/results are recorded for implementation slices.
- [ ] Contract decisions and behavior changes are reflected in docs as they land.
