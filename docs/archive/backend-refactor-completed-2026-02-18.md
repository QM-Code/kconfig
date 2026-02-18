# Backend Refactor (Factory + Layout Standardization)

## Project Snapshot
- Current owner: `overseer`
- Status: `archived/completed (cross-subsystem backend standardization landed and validated)`
- Immediate next task: `none (archived)`.
- Validation gate: docs slices run `./docs/scripts/lint-project-docs.sh`; backend code slices run `./abuild.py -c -d <build-dir>`, `./scripts/test-engine-backends.sh <build-dir>`, and `./scripts/test-server-net.sh <build-dir>`.

## Mission
Standardize backend architecture across `audio`, `renderer`, `physics`, `ui`, and `platform` so they share one coherent pattern for:
- filesystem layout,
- factory responsibilities and behavior,
- constructor wiring and compile-time capability gating,
- stub strategy,
- namespace and header organization.

## Locked Outcomes (Implemented)
1. Redundant `backend_` filename prefixes were removed from backend implementation/internal-factory files in audio and physics.
2. Missing factory seams were added:
   - `src/engine/ui/backend_factory.cpp`
   - `src/engine/window/backend_factory.cpp`
3. Factory behavior was normalized across backend subsystems around:
   - `BackendKindName`
   - `ParseBackendKind`
   - `CompiledBackends`
   - `CreateBackend`
4. Stub strategy was aligned to explicit per-backend `stub.cpp` translation units.
5. Renderer constructor declaration header was standardized to:
   - `src/engine/renderer/backends/factory_internal.hpp`
6. Underscore backend namespace islands and compatibility aliases/shims were removed from engine/public surfaces.

## As-Built Baseline (2026-02-18)

| Subsystem | Factory entrypoint | Constructor declarations | Concrete backend units | Notes |
|---|---|---|---|---|
| `audio` | `src/engine/audio/backend_factory.cpp` | `src/engine/audio/backends/factory_internal.hpp` | `sdl3audio.cpp`, `sdl3audio_stub.cpp`, `miniaudio.cpp`, `miniaudio_stub.cpp` | no `backend_` filename prefix remains |
| `renderer` | `src/engine/renderer/backend_factory.cpp` | `src/engine/renderer/backends/factory_internal.hpp` | backend dirs with explicit `stub.cpp` per backend | standardized internal factory header naming is in place |
| `ui` | `src/engine/ui/backend_factory.cpp` | `src/engine/ui/backends/factory_internal.hpp` | imgui/rmlui backend dirs + software backend | selection extracted from `ui/system.cpp` |
| `platform` | `src/engine/window/backend_factory.cpp` | `src/engine/window/backends/factory_internal.hpp` | `sdl3.cpp`, `sdl3_stub.cpp` | standardized backend factory seam is in place |
| `physics` | `src/engine/physics/backend_factory.cpp` | `src/engine/physics/backends/factory_internal.hpp` | `jolt.cpp`, `jolt_stub.cpp`, `physx.cpp`, `physx_stub.cpp` | no `backend_` filename prefix remains; real/stub split matches subsystem standard |

## Standardized Contract

### A. Namespace model
Backend namespaces align to filesystem layout:
- `karma::audio::backend`
- `karma::renderer::backend`
- `karma::ui::backend`
- `karma::window::backend`
- `karma::physics::backend`

No underscore-form backend namespace compatibility aliases are permitted.

### B. Factory behavior model
Each backend factory provides:
- `BackendKindName(kind)`
- `ParseBackendKind(name)`
- `CompiledBackends()`
- `CreateBackend(preferred, out_selected)`

Behavior invariants:
- `preferred == Auto` iterates compiled backends in deterministic order.
- Explicit backend requests do not silently switch to another backend.
- `out_selected` is initialized to `Auto` then set to concrete backend on success.
- Unavailable/uncompiled backends resolve deterministically via `nullptr` return.

### C. Stub policy
Stub behavior is implemented via explicit per-backend `stub.cpp` units. Real backend implementation files do not embed stub fallback classes.

## Execution Slice Status
- `BR0` Standards freeze + impact matrix: complete.
- `BR1` Mechanical rename pass: complete.
- `BR2` UI factory extraction: complete.
- `BR3` Platform factory standardization: complete.
- `BR4` Cross-subsystem factory behavior normalization: complete.
- `BR5` Stub policy harmonization: complete.
- `BR6` Header + namespace refactor: complete, including alias/shim removal.
- `BR7` Closeout + contract-doc sync: complete.

## Validation (Most Recent Refactor Pass)
From `m-rewrite/`:

```bash
./abuild.py -c -d build-dev
./scripts/test-engine-backends.sh build-dev
./scripts/test-server-net.sh build-dev
./abuild.py -c -d build-physcheck -b jolt,physx --test-physics
./docs/scripts/lint-project-docs.sh
```

## Remaining Work
1. No open work for locked project scope.
2. If future subsystem work introduces backend-factory or namespace drift, reopen this track with a bounded corrective slice.

## Current Status Log
- `2026-02-18`: project started with consistency decisions and phased plan.
- `2026-02-18`: non-physics backend standardization executed (factory seams/layout/header naming/stub policy/namespace cleanup).
- `2026-02-18`: physics backend naming/split standardization executed (`factory_internal.hpp`, `jolt.cpp`/`jolt_stub.cpp`, `physx.cpp`/`physx_stub.cpp`) and validated.
- `2026-02-18`: BR7 closeout complete; project locked as complete.
- `2026-02-18`: project archived to `docs/archive/backend-refactor-completed-2026-02-18.md` and active assignment row removed from `docs/projects/ASSIGNMENTS.md`.

## Handoff Checklist
- [x] Scope stayed in backend-refactor boundaries for this pass.
- [x] Cross-subsystem factory/header/namespace standardization state captured.
- [x] `docs/projects/ASSIGNMENTS.md` active row removed at archive closeout.
- [x] Physics-lane closeout merged and BR7 marked complete.
