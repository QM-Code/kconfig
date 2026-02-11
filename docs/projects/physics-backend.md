# Physics Backend

## Project Snapshot
- Current owner: `codex`
- Status: `in progress` (collision-query + dynamic gravity body-flag + rotation-lock + translation-lock parity slices complete)
- Immediate next task: decide and implement runtime lock mutation/query contract (or explicitly keep constraints creation-time only for this phase) with backend-matching assertions.
- Validation gate: `./scripts/test-engine-backends.sh`

## Mission
Own engine physics backend behavior and contract parity (`auto|jolt|physx`) without leaking backend specifics into game code.

## Primary Specs
- `docs/projects/core-engine-infrastructure.md` (physics sections)
- `docs/projects/engine-backend-testing.md`

## Why This Is Separate
Physics backend implementation and parity checks are mostly independent from UI, renderer, and server protocol details.

## Owned Paths
- `m-rewrite/src/engine/physics/*`
- `m-rewrite/include/karma/physics/*`
- `m-rewrite/src/engine/physics/tests/*`

## Interface Boundaries
- Consumed by: `EngineApp`, `EngineServerApp`.
- Exposed contract: `PhysicsSystem` and backend factory behavior.
- Coordinate before changing:
  - `m-rewrite/src/engine/CMakeLists.txt`
  - `docs/projects/core-engine-infrastructure.md`

## Layering Contract Alignment (Implementation-Ready)

## Core Contract Boundaries
- `PhysicsSystem` is the only game-facing/system-facing physics boundary.
- `BodyId` remains opaque and engine-owned; backend handles/pointers must not cross engine/game contracts.
- Core contract scope is lifecycle (`init/step/shutdown`), body lifecycle (`create/destroy/isValid`), state operations, and base queries (`raycastClosest` + minimal overlap primitives).
- Fixed-step ownership stays engine-owned; callers do not drive backend step timing directly.

## Optional 95%-Path Facade Boundaries
- Facade helpers (spawn/query/preset helpers) are allowed for common workflows only.
- Facade APIs must map to core `PhysicsSystem` calls and may not bypass the core contract.
- Facade defaults must remain deterministic and backend-neutral.

## Override/Extension Limits (5% Path)
- Advanced behavior must be exposed through explicit advanced contracts (for example `PhysicsAdvanced`), not mixed into default-path facade APIs.
- Advanced options may be added via explicit option structs, while preserving core lifecycle/safety invariants.
- Disallowed:
  - backend SDK exposure in game code,
  - backend-specific gameplay branching,
  - alternate physics-step ownership outside engine fixed-step loop.

## Validation/Failure Expectations
- Startup failures (invalid backend selection/config) must fail fast before first simulation tick with actionable diagnostics.
- Invalid/stale `BodyId` operations must be deterministic failure/no-op and never crash.
- Invalid query/descriptor inputs must fail deterministically and must not partially mutate world/backend state.
- Failure diagnostics must include operation name, reason category, and `BodyId` when applicable.

## Required Tests and Wrapper-Gate Expectations
- Required first-slice contract tests (from `core-engine-infrastructure.md`):
  - `physics_body_id_lifecycle_contract_test`
  - `physics_core_state_query_contract_test`
  - `physics_invalid_input_failure_contract_test`
  - `physics_facade_boundary_contract_test`
  - `physics_override_boundary_contract_test`
  - `engine_fixed_step_physics_contract_smoke_test`
- Required parity expectation:
  - backend parity coverage must explicitly assert `BodyId` lifecycle + failure semantics across all compiled physics backends.
- Required project-level backend validation commands:
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-jolt-rmlui-sdl3audio`
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-physx-rmlui-sdl3audio`
- Required wrapper-gate expectation for handoff:
  - `./scripts/test-engine-backends.sh <build-dir>` must pass for touched assigned physics build dirs.

## Non-Goals
- Gameplay rules (shots, scoring, respawn).
- Network protocol changes.
- UI/render implementation details.

## Validation
From `m-rewrite/`:

```bash
./bzbuild.py -c --test-physics build-sdl3-bgfx-jolt-rmlui-sdl3audio
./bzbuild.py -c --test-physics build-sdl3-bgfx-physx-rmlui-sdl3audio
```

## Trace Channels
- `engine.app`
- `engine.server`
- `engine.sim`
- `engine.sim.frames`

## Build/Run Commands
```bash
./bzbuild.py -c --test-physics build-sdl3-bgfx-jolt-rmlui-sdl3audio
./bzbuild.py -c --test-physics build-sdl3-bgfx-physx-rmlui-sdl3audio
```

## First Session Checklist
1. Read `docs/projects/core-engine-infrastructure.md` physics sections.
2. Confirm backend selection + lifecycle expectations.
3. Implement smallest contract-safe change.
4. Run both assigned isolated physics builds with `./bzbuild.py`.
5. Update this file's status notes.

## Current Status
- `2026-02-10`: Synced this project doc with implementation-ready physics layering contract boundaries and required test/wrapper expectations from `docs/projects/core-engine-infrastructure.md`.
- Contract/lifecycle scaffolding and backend selection are in place.
- Physics backend parity tests are present and should remain the authoritative regression gate.
- Real backend path is active for both compiled backends (`jolt`, `physx`) behind the same `PhysicsSystem` contract.
- Immediate parity task verification passed in dedicated backend build dirs:
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-jolt-rmlui-sdl3audio`
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-physx-rmlui-sdl3audio`
- Collision query parity slice is now implemented behind `PhysicsSystem`:
  - backend-agnostic closest-hit raycast API is exposed through engine physics contracts,
  - Jolt and PhysX implement matching closest-ray behavior under the same API,
  - parity tests now validate deterministic closest-hit/miss semantics for both backends.
- Dynamic-body gravity flag parity slice is now implemented behind `PhysicsSystem`:
  - `BodyDesc` now includes `gravity_enabled` for creation-time body flag intent,
  - backend-agnostic `setBodyGravityEnabled` / `getBodyGravityEnabled` APIs are exposed via `PhysicsSystem`,
  - Jolt and PhysX implement matching dynamic-body gravity flag behavior under the same API,
  - parity tests now assert both backends match: static-body rejection, invalid/uninitialized rejection, no-gravity drop suppression, and runtime toggle application.
- Rotation-lock constraints parity slice is now implemented behind `PhysicsSystem` creation contract:
  - `BodyDesc` now includes `rotation_locked` for creation-time angular constraint intent,
  - Jolt maps the flag to allowed-DOF constraints and PhysX maps the flag to rigid-dynamic angular lock flags,
  - parity tests now assert backend-matching behavior: unlocked bodies rotate under angular velocity while rotation-locked bodies remain orientation-stable.
- Translation-lock constraints parity slice is now implemented behind `PhysicsSystem` creation contract:
  - `BodyDesc` now includes `translation_locked` for creation-time linear constraint intent,
  - Jolt maps lock combinations (`rotation_locked`, `translation_locked`) to matching allowed-DOF constraints and PhysX maps linear lock intent to rigid-dynamic linear lock flags,
  - parity tests now assert backend-matching behavior: unlocked bodies translate under linear velocity while translation-locked bodies stay position-stable.
- Raycast contract follow-up TODOs are now explicit:
  - start-inside-shape behavior must be standardized across backends,
  - query filtering/layer semantics are intentionally not exposed yet and need contract design before gameplay use,
  - current backend body-handle -> `BodyId` hit mapping uses linear scan and should be optimized if query load increases.
- Remaining parity work is depth expansion under current boundaries (constraints + additional query depth), not stub replacement for current body lifecycle path.
- Latest delegated validation (assigned build dirs):
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-jolt-rmlui-sdl3audio` ✅ pass
  - `./bzbuild.py -c --test-physics build-sdl3-bgfx-physx-rmlui-sdl3audio` ✅ pass
- Wrapper validation passed:
  - `./scripts/test-engine-backends.sh build-sdl3-bgfx-jolt-rmlui-sdl3audio` ✅ pass (2/2)

## Open Questions
- Should constraints exposure remain creation-time only for now, or add runtime lock mutation/getter APIs in the next parity slice?
- Do any current call sites assume behavior that is not contractually documented?

## Raycast Contract Notes (TBD)
- Start-inside semantics (TBD): contract is not yet standardized; until finalized, callers must not depend on whether `raycastClosest` returns an immediate hit (`fraction ~= 0`) when origin starts inside a shape.
- Filtering/layer semantics (TBD): current contract performs an unfiltered closest ray query across backend-visible physics bodies; no layer mask/query filter is exposed yet, and gameplay callers must treat filtering behavior as undefined until that API is added.

## Handoff Checklist
- [x] Physics contract unchanged or intentionally versioned.
- [x] Parity tests green.
- [x] Required assigned build-dir commands executed and recorded.
- [x] Trace signals remain high-signal.
- [x] Relevant docs updated.
