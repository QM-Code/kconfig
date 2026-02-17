# KARMA Lighting + Shadow Parity

## Project Snapshot
- Current owner: `codex`
- Status: `priority/in progress (new canonical lighting/shadow intake track)`
- Upstream snapshot: `KARMA-REPO@905b63b`
- Rewrite snapshot: `m-rewrite@7ee717f8d`
- Immediate next task: execute Slice `P0-S1` (directional CSM intake design lock + first vertical slice in sandbox).
- Validation gate: both renderer builds, sandbox proof recipes, runtime smoke, and docs lint must pass before slice acceptance.

## Mission
Implement every lighting/shadow technique that is actively used in KARMA demo paths and still missing (or only partially implemented) in `m-rewrite`, prove each in sandbox first, then wire the proven stack into `bz3` runtime.

## Foundation References
- `docs/foundation/policy/execution-policy.md`
- `docs/foundation/policy/rewrite-invariants.md`
- `docs/foundation/governance/overseer-playbook.md`
- `docs/archive/renderer-shadow-hardening-superseded-2026-02-17.md` (source of carry-over unfinished items)
- `docs/projects/renderer-parity.md` (cross-track status only)

## Why This Is Separate
- Shadow hardening moved from isolated bug-fix mode into a full capability-parity program.
- We now need one canonical agent start point that covers: KARMA intake gaps, sandbox proof policy, and final bz3 integration sequencing.
- This document supersedes `renderer-shadow-hardening.md` as the active intake/implementation entry point.

## Owned Paths
- `docs/projects/karma-lighting-shadow-parity.md`
- `src/engine/renderer/backends/directional_shadow_internal.hpp`
- `src/engine/renderer/backends/bgfx/backend_bgfx.cpp`
- `src/engine/renderer/backends/diligent/backend_diligent.cpp`
- `data/bgfx/shaders/mesh/fs_mesh.sc`
- `src/engine/renderer/tests/renderer_shadow_sandbox.cpp`
- `src/engine/renderer/render_system.cpp`
- `include/karma/renderer/backend.hpp`
- `include/karma/renderer/device.hpp`
- `src/engine/app/engine_app.cpp`
- `src/game/client/runtime.cpp`
- `docs/projects/ASSIGNMENTS.md`

Read-only comparison root:
- `../KARMA-REPO`

## Interface Boundaries
- Inputs consumed:
  - KARMA behavioral reference from `KARMA-REPO` renderer/app/demo paths.
  - Existing rewrite renderer contracts and runtime config surface.
- Outputs exposed:
  - backend-parity lighting/shadow behavior in rewrite sandbox.
  - runtime-wired controls needed for deterministic bz3 verification.
- Coordinate before changing:
  - `docs/projects/renderer-parity.md`
  - `docs/foundation/architecture/core-engine-contracts.md`

## Non-Goals
- Do not clone KARMA file layout or architecture verbatim.
- Do not expand into unrelated gameplay/network/UI migration work.
- Do not land unproven shadow/lighting changes directly into bz3 runtime without sandbox proof.

## Sweep Result: KARMA Techniques Missing In m-rewrite

Legend:
- `Missing`: not implemented in rewrite.
- `Partial`: some plumbing exists, but behavior is not at KARMA parity.

| Area | KARMA-REPO (active demo path) | m-rewrite state | Gap |
|---|---|---|---|
| Directional shadow topology | 4-cascade CSM array with split logic + transition blending (`include/karma/renderer/backends/diligent/backend.hpp`, `src/renderer/backends/diligent/backend_render.cpp`, `src/renderer/backends/diligent/backend_init.cpp`) | Single-map directional shadow projection only; no cascade metadata/tables in shader path (`src/engine/renderer/backends/directional_shadow_internal.hpp`, `data/bgfx/shaders/mesh/fs_mesh.sc`) | `Missing` |
| CSM stability policy | Texel-snapped cascade fit + cached matrices/splits + camera/light threshold invalidation (`src/renderer/backends/diligent/backend_render.cpp`) | Single-map fit/snap exists; no cascade-stability policy because no CSM | `Missing` (CSM-specific) |
| Directional shadow sampling | Hardware compare sampling (`SamplerComparisonState`, `SampleCmpLevelZero`) + optional PCF loop (`src/renderer/backends/diligent/backend_init.cpp`) | Manual depth sample + `step(...)` PCF kernels in BGFX and Diligent shaders (`data/bgfx/shaders/mesh/fs_mesh.sc`, `src/engine/renderer/backends/diligent/backend_diligent.cpp`) | `Missing` |
| Point shadow sampling | Hardware compare sampling for point shadows (`SampleCmpLevelZero` on point map) | Manual depth sample + `step(...)` for point shadows | `Missing` |
| Rasterizer depth/slope bias usage | Shadow raster state consumes `shadow_raster_depth_bias` + `shadow_raster_slope_bias` (`src/renderer/backends/diligent/backend_init.cpp`) | Bias values are plumbed into semantics/uniforms, but not applied as rasterizer state in rewrite backends | `Partial` |
| Point-shadow generation path | GPU depth rendering per face with per-face DSVs + dirty-face scheduling (`include/karma/renderer/backends/diligent/backend.hpp`, `src/renderer/backends/diligent/backend_render.cpp`) | CPU rasterized atlas build (`BuildPointShadowMap`) then uploaded each update cycle (`src/engine/renderer/backends/directional_shadow_internal.hpp`, backend `BuildPointShadowMap` call sites) | `Missing` |
| Local light scalability | Forward+ local light clustering (compute path + CPU fallback), runtime tile/max controls (`src/renderer/backends/diligent/backend_init.cpp`, `src/renderer/backends/diligent/backend_render.cpp`) | Fixed-size local light array (`kMaxLocalLights = 4`) in both backends; no Forward+ path or controls | `Missing` |
| Environment lighting source | HDR environment-map pipeline: equirect -> cubemap, irradiance, prefilter, BRDF LUT, skybox render (`src/renderer/backends/diligent/backend_render.cpp`, `backend_mesh.cpp`) | Hemispherical sky/ground ambient approximation only; no env map ingestion/prefilter/BRDF LUT pipeline (`src/engine/renderer/backends/environment_lighting_internal.hpp`) | `Missing` |
| PBR + IBL shading | Shader path uses full material + IBL textures and tone mapping (`src/renderer/backends/diligent/backend_init.cpp`) | Simplified shading path; no KARMA-equivalent IBL integration | `Missing` |
| Exposure control | Runtime `setExposure` API wired from EngineConfig + debug overlay (`include/karma/renderer/backend.hpp`, `src/app/engine_app.cpp`, `src/debug/debug_overlay.cpp`) | No exposure API in rewrite backend/device interface; no exposure control plumbing | `Missing` |
| Renderer runtime control plane | `setGenerateMips`, `setEnvironmentMap`, `setAnisotropy`, `setForwardPlusSettings`, `setShadowSettings`, `setPointShadowSettings`, `setLocalLightingSettings`, `setExposure` are backend/device contracts | Rewrite interface only exposes camera, directional light, local lights, environment-lighting struct (`include/karma/renderer/backend.hpp`, `include/karma/renderer/device.hpp`) | `Missing` |
| Engine config plumbing for texture filtering | KARMA app applies anisotropy/mip flags into renderer backend on startup (`src/app/engine_app.cpp`) | Rewrite `EngineConfig` has anisotropy/mip fields, but runtime wiring into renderer APIs is absent | `Missing` |
| Runtime tuning/debug loop | KARMA debug overlay includes live shadow/local-light/forward+/exposure controls (`src/debug/debug_overlay.cpp`) | No rewrite runtime debug panel for these controls (CLI/sandbox only) | `Missing` |

## Already Landed In m-rewrite (Keep Green)
- GPU directional shadow pass (`gpu_default`) in BGFX and Diligent.
- Shared shadow bias semantics and config keys (constant/receiver/normal/raster fields).
- Multi-point shadow selection with dirty-face scheduling and face-budget control.
- Local light shadow-lift parameters and AO-local-light modulation knobs.
- Sandbox support for multi-point-light motion, pause/resume (`space`), and diagnostics.

## Normalized Carry-Over Items (from renderer-shadow-hardening.md)
1. Contact-edge visual closeout is still required in roaming/runtime checkpoints.
2. Low-frequency blockiness/aliasing remains a quality risk and must be measured after compare-sampler/CSM intake.
3. Diligent non-interactive screenshot capture remains environment-blocked (`VK_ERROR_INITIALIZATION_FAILED` on X11 in headless capture); operator desktop evidence remains required.
4. Regression watch stays active for prior distance-dropout behavior; every slice must include an explicit distance-persistence check.
5. Runtime debug UI timing question is now explicit: defer full panel until core parity slices land, then add bounded panel for maintainability.
6. Cascade-count policy decision is now attached to CSM slice acceptance (`fixed 4 first`, optional configurability only after parity proof).

## Execution Plan

### P0-S0: Baseline Lock + Regression Harness
- Lock current known-good sandbox recipe and traces as baseline.
- Keep current face-budget behavior documented:
  - dynamic scenes require budget proportional to active shadowed point lights (`faces = 6 * active_lights`).
- Acceptance:
  - baseline screenshots/traces for BGFX + Diligent captured and linked.
  - no regressions against current “working” sandbox command family.

### P0-S1: Directional CSM Intake (KARMA parity)
- Add 4-cascade directional shadow topology and per-cascade metadata.
- Add split policy (lambda), cascade transition blending, and texel-snapped cascade fit.
- Keep single-map fallback path behind an explicit mode while stabilizing.
- Acceptance:
  - moving-camera sandbox shows no cascade seam pops.
  - near/far shadow stability better than current single-map baseline.

### P0-S2: Compare-Sampler Shadow Sampling
- Replace manual `step`-based directional/point shadow compare with hardware compare sampling path (per backend capability).
- Keep bounded PCF radius controls.
- Acceptance:
  - visible reduction in blocky penumbra/seam artifacts in both backends.
  - no detached-shadow regressions in moving-point-light sandbox.

### P0-S3: Point Shadow GPU Generation Path
- Replace CPU `BuildPointShadowMap` raster path with GPU face rendering/update scheduling.
- Keep dirty-face scheduler + budget policy.
- Acceptance:
  - measured frame-time improvement at equivalent quality over CPU atlas path.
  - shadow placement remains stable under motion (lights + casters moving).

### P0-S4: Renderer Control Plane Parity
- Introduce rewrite-owned equivalents for missing runtime renderer controls:
  - environment map, anisotropy, mip generation, shadow settings, point shadow settings, local-light settings, forward+ settings, exposure.
- Wire from runtime config and engine app startup.
- Acceptance:
  - no dead knobs: every exposed config field is proven active via trace or functional change.

### P1-S1: Environment/IBL Intake
- Add HDR env-map ingestion + irradiance/prefilter/BRDF LUT + skybox path.
- Bind into material lighting path in both backends (or document bounded backend staging if unavoidable).
- Acceptance:
  - sandbox/world checkpoints show expected image-based ambient/specular response.

### P1-S2: Forward+ Local-Light Scalability
- Add clustered/Forward+ local-light path (with bounded fallback).
- Remove `kMaxLocalLights=4` practical ceiling in primary path.
- Acceptance:
  - local-light stress scenes retain stable frame-time and visual parity.

### P1-S3: Runtime Debug Control Surface
- Add bounded rewrite debug panel for shadow/lighting parity controls and perf diagnostics.
- Acceptance:
  - operator can tune and verify key knobs in-runtime without CLI restarts.

### P2-S1: bz3 Runtime Wiring
- After sandbox parity proof, wire full lighting/shadow stack into bz3 runtime scene flow.
- Acceptance:
  - roaming bz3 checkpoints match sandbox-proven behavior for directional + point shadows and lighting quality/perf envelopes.

## Validation
From `m-rewrite/`:

```bash
./abuild.py -c build-sdl3-bgfx-physx-imgui-sdl3audio
./abuild.py -c build-sdl3-diligent-physx-imgui-sdl3audio

# Canonical sandbox parity recipes (backend-specific)
./build-sdl3-bgfx-physx-imgui-sdl3audio/src/engine/renderer_shadow_sandbox \
  --backend-render bgfx --duration-sec 30 --ground-tiles 1 --ground-extent 20 \
  --shadow-map-size 2048 --shadow-pcf 2 --shadow-strength 0.85 --shadow-execution-mode gpu_default \
  --point-shadow-lights 2 --point-shadow-map-size 256 --point-shadow-max-lights 2 \
  --point-shadow-light-range 14 --point-shadow-light-intensity 2 \
  --point-shadow-scene-motion --point-shadow-motion-speed 0.9

./build-sdl3-diligent-physx-imgui-sdl3audio/src/engine/renderer_shadow_sandbox \
  --backend-render diligent --duration-sec 30 --ground-tiles 1 --ground-extent 20 \
  --shadow-map-size 2048 --shadow-pcf 2 --shadow-strength 0.85 --shadow-execution-mode gpu_default \
  --point-shadow-lights 2 --point-shadow-map-size 256 --point-shadow-max-lights 2 \
  --point-shadow-light-range 14 --point-shadow-light-intensity 2 \
  --point-shadow-scene-motion --point-shadow-motion-speed 0.9

# Runtime smoke
timeout -k 2s 20s ./build-sdl3-bgfx-physx-imgui-sdl3audio/bz3 -d ./data --strict-config=true --config data/client/config.json -v -t engine.sim,render.system,render.bgfx
timeout -k 2s 20s ./build-sdl3-diligent-physx-imgui-sdl3audio/bz3 -d ./data --strict-config=true --config data/client/config.json -v -t engine.sim,render.system,render.diligent

./docs/scripts/lint-project-docs.sh
```

## Trace Channels
- `render.system`
- `render.bgfx`
- `render.diligent`
- `engine.sim`
- `render.mesh`

## First Session Checklist
1. Read this file + `docs/archive/renderer-shadow-hardening-superseded-2026-02-17.md` carry-over section only.
2. Re-run canonical sandbox command for both backends and capture baseline evidence.
3. Pick exactly one active slice from this plan and scope it narrowly.
4. Land code + validation + doc updates in same handoff.
5. Update `docs/projects/ASSIGNMENTS.md` status/next task.

## Open Questions
- Should compare-sampler rollout be strictly backend-synchronized, or can Diligent lead with BGFX parity gate in next slice?
- For CSM, should rewrite lock to 4 cascades first (KARMA parity) or expose cascade count immediately?
- At what slice do we require world-asset parity captures in addition to synthetic sandbox captures?

## Handoff Checklist
- [ ] Active slice completed
- [ ] Builds/tests run and summarized
- [ ] Sandbox evidence captured for both backends
- [ ] Runtime smoke completed (or blocker documented)
- [ ] This file updated
- [ ] `docs/projects/ASSIGNMENTS.md` updated
- [ ] Remaining risks/open questions listed
