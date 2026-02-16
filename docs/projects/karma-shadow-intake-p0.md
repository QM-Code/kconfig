# KARMA Shadow Intake P0 (Integration Track)

## Project Snapshot
- Current owner: `overseer`
- Status: `priority/queued (P0 integration track created)`
- Immediate next task: issue first specialist slice for point-shadow contract scaffolding plus moving-caster refresh policy.
- Validation gate: renderer build gates in both assigned build dirs, sandbox/runtime trace evidence, and docs lint.

## Mission
Intake high-value shadowing capability updates from `KARMA-REPO` (latest upstream: `20bdf28`, `905b63b`) into `m-rewrite` under rewrite-owned contracts, without mirroring KARMA file layout.

P0 objective:
- preserve current accepted directional `gpu_default` behavior,
- add bounded point-shadow capability + refresh behavior where it improves visible parity and stability,
- keep backend parity across BGFX + Diligent.

## Foundation References
- `docs/foundation/policy/rewrite-invariants.md`
- `docs/foundation/policy/execution-policy.md`
- `docs/foundation/policy/decisions-log.md`
- `docs/projects/renderer-parity.md`
- `docs/projects/renderer-shadow-hardening.md`

## Why This Is Separate
This intake cuts across renderer contracts, backend internals, config policy, and visual acceptance evidence. Keeping it in a dedicated integration track avoids destabilizing active closeout work while preserving explicit adopt/defer boundaries for KARMA deltas.

## Strategic Labeling
- Primary track label: `KARMA intake`
- Secondary track label: `shared unblocker` (renderer visual parity and stability)

## Scope Intake (From KARMA Commits)
Adopt now (P0 bounded scope):
1. Point-light shadow map generation/sampling for shadow-casting point lights (bounded light count and update budget).
2. Moving-caster-aware point-shadow refresh invalidation (dirty only impacted faces/slots).
3. Runtime policy/config plumbing for point-shadow bias controls and local-light interaction terms needed for deterministic tuning.
4. Trace visibility for update reasons (cache hit/miss, dirty face updates, fallback causes).

Defer (not P0 in this track):
1. Radar camera shader-override demo workflow.
2. Broad camera override shader system beyond what is needed for shadowing acceptance.
3. Full renderer-pipeline refactors not required for shadow quality/stability closeout.

## Owned Paths
- `m-rewrite/docs/projects/karma-shadow-intake-p0.md`
- `m-rewrite/docs/projects/ASSIGNMENTS.md`
- `m-rewrite/src/engine/renderer/backends/directional_shadow_internal.hpp`
- `m-rewrite/src/engine/renderer/backends/bgfx/backend_bgfx.cpp`
- `m-rewrite/src/engine/renderer/backends/diligent/backend_diligent.cpp`
- `m-rewrite/include/karma/renderer/types.hpp`
- `m-rewrite/src/engine/renderer/tests/*`
- `m-rewrite/data/client/config.json`

## Interface Boundaries
- Inputs consumed:
  - KARMA commit intent from `20bdf28` and `905b63b`.
  - Existing shadow contracts/acceptance criteria from `renderer-parity.md` and `renderer-shadow-hardening.md`.
- Outputs exposed:
  - rewrite-owned point-shadow contract behavior with backend parity,
  - deterministic update/fallback diagnostics and bounded config surface.
- Coordinate before changing:
  - `m-rewrite/docs/projects/renderer-parity.md`
  - `m-rewrite/docs/projects/renderer-shadow-hardening.md`
  - `m-rewrite/docs/foundation/architecture/core-engine-contracts.md`

## Non-Goals
- Do not mirror KARMA source layout/abstractions one-to-one.
- Do not expand into gameplay/netcode/UI behavior.
- Do not start platform-backend expansion.
- Do not open unbounded renderer-material feature work outside shadow correctness/stability.

## Execution Slices
1. `KS1` contract + config slice:
   - define bounded point-shadow settings and defaults under rewrite config policy.
   - add parser/contract tests for settings and clamps.
2. `KS2` backend implementation slice:
   - implement point-shadow map generation/sampling parity in BGFX + Diligent with bounded light budget.
3. `KS3` refresh invalidation slice:
   - implement moving-caster and light-motion dirtying policy with per-frame budgeted updates.
4. `KS4` tuning + closeout slice:
   - lock defaults from bounded sweep evidence and record operator visual checkpoints.

## Acceptance Criteria
1. Both renderer build dirs compile and run with point-shadow path enabled.
2. Point-shadow refresh occurs when relevant moving casters/lights invalidate cached faces.
3. Trace evidence includes clear update/fallback reason tokens.
4. Directional-shadow existing `gpu_default` behavior remains green (no regression).
5. Visual parity checkpoints are documented for BGFX + Diligent.

## Validation
From `m-rewrite/`:

```bash
./bzbuild.py -c build-sdl3-bgfx-physx-imgui-sdl3audio
./bzbuild.py -c build-sdl3-diligent-physx-imgui-sdl3audio
./scripts/run-renderer-shadow-sandbox.sh 20 16 20
timeout -k 2s 20s ./build-sdl3-bgfx-physx-imgui-sdl3audio/bz3 -d ./data --strict-config=true --config data/client/config.json -v -t engine.sim,render.system,render.bgfx,render.mesh
timeout -k 2s 20s ./build-sdl3-diligent-physx-imgui-sdl3audio/bz3 -d ./data --strict-config=true --config data/client/config.json -v -t engine.sim,render.system,render.diligent,render.mesh
./docs/scripts/lint-project-docs.sh
```

## Trace Channels
- `render.system`
- `render.bgfx`
- `render.diligent`
- `render.mesh`
- `engine.sim`

## Build Dirs (Assigned)
- `build-sdl3-bgfx-physx-imgui-sdl3audio`
- `build-sdl3-diligent-physx-imgui-sdl3audio`

## Current Status
- `2026-02-16`: P0 integration track created from latest KARMA shadowing deltas.
- `2026-02-16`: Upstream intake candidates classified:
  - adopt-now: point-shadow map path, dirty refresh policy for moving casters, bounded runtime tuning controls.
  - deferred: radar/camera shader-override demo path and non-essential wide renderer refactors.

## Open Questions
- Should point-shadow rendering be default-on in rewrite or rollout behind explicit config until closeout evidence is complete?
- Keep hard cap at 2 shadow-casting point lights for P0, or expose bounded config now?
- Should local-light directional-shadow lift be in KS2 or deferred to KS4 tuning once baseline parity is confirmed?

## Handoff Checklist
- [ ] Slice boundary respected (`KS1`..`KS4`)
- [ ] Required build/runtime evidence recorded
- [ ] Regression checks for directional `gpu_default` path recorded
- [ ] `ASSIGNMENTS.md` updated
- [ ] Risks/open questions updated
