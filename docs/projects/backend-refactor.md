# Backend Refactor (Factory + Layout Standardization)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (non-physics backend standardization landed; closeout/doc sync remains)`
- Immediate next task: complete BR7 closeout bookkeeping after physics lane coordination.
- Validation gate: docs slices run `./docs/scripts/lint-project-docs.sh`; backend code slices run `./abuild.py -c -d <build-dir>`, `./scripts/test-engine-backends.sh <build-dir>`, and `./scripts/test-server-net.sh <build-dir>`.

## Mission
Standardize backend architecture across `audio`, `renderer`, `physics`, `ui`, and `platform` so they share one coherent pattern for:
- filesystem layout,
- factory responsibilities and behavior,
- constructor wiring and compile-time capability gating,
- stub strategy,
- namespace and header organization.

This document currently reflects the non-physics closeout state. Physics-specific progress/details stay in `docs/projects/physics-refactor.md`.

## Locked Outcomes (Now Implemented for Non-Physics Scope)
1. Redundant `backend_` filename prefixes were removed from audio backend implementation/internal-factory files.
2. Missing factory seams were added:
   - `src/engine/ui/backend_factory.cpp`
   - `src/engine/platform/backend_factory.cpp`
3. Factory behavior was normalized across non-physics subsystems around:
   - `BackendKindName`
   - `ParseBackendKind`
   - `CompiledBackends`
   - `CreateBackend`
4. Stub strategy was aligned to explicit per-backend `stub.cpp` translation units.
5. Renderer constructor declaration header was standardized to:
   - `src/engine/renderer/backends/factory_internal.hpp`
6. Underscore backend namespace islands and compatibility aliases/shims were removed from non-physics engine/public surfaces.

## As-Built Baseline (2026-02-18)

| Subsystem | Factory entrypoint | Constructor declarations | Concrete backend units | Notes |
|---|---|---|---|---|
| `audio` | `src/engine/audio/backend_factory.cpp` | `src/engine/audio/backends/factory_internal.hpp` | `sdl3audio.cpp`, `sdl3audio_stub.cpp`, `miniaudio.cpp`, `miniaudio_stub.cpp` | no `backend_` filename prefix remains |
| `renderer` | `src/engine/renderer/backend_factory.cpp` | `src/engine/renderer/backends/factory_internal.hpp` | backend dirs with explicit `stub.cpp` per backend | standardized internal factory header naming is in place |
| `ui` | `src/engine/ui/backend_factory.cpp` | `src/engine/ui/backends/factory_internal.hpp` | imgui/rmlui backend dirs + software backend | selection extracted from `ui/system.cpp` |
| `platform` | `src/engine/platform/backend_factory.cpp` | `src/engine/platform/backends/factory_internal.hpp` | `window_sdl3.cpp`, `window_sdl3_stub.cpp` | standardized backend factory seam is in place |
| `physics` | tracked separately | tracked separately | tracked separately | excluded from this doc update pass |

## Standardized Contract

### A. Namespace model
Backend namespaces align to filesystem layout:
- `karma::audio::backend`
- `karma::renderer::backend`
- `karma::ui::backend`
- `karma::platform::backend`
- `karma::physics::backend` (managed in physics track)

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
- `BR1` Mechanical rename pass: complete for non-physics scope.
- `BR2` UI factory extraction: complete.
- `BR3` Platform factory standardization: complete.
- `BR4` Cross-subsystem factory behavior normalization: complete for non-physics scope.
- `BR5` Stub policy harmonization: complete for non-physics scope.
- `BR6` Header + namespace refactor: complete for non-physics scope, including alias/shim removal.
- `BR7` Closeout + contract-doc sync: in progress (remaining dependency is physics-lane coordinated closeout).

## Validation (Most Recent Refactor Pass)
From `m-rewrite/`:

```bash
./abuild.py -c -d build-dev
./scripts/test-engine-backends.sh build-dev
./scripts/test-server-net.sh build-dev
./docs/scripts/lint-project-docs.sh
```

## Remaining Work
1. Complete BR7 final cross-track closeout after physics lane synchronization.
2. If physics lane changes factory/header contracts, mirror final policy wording in this file and `docs/foundation/architecture/core-engine-contracts.md`.
3. Keep `docs/projects/ASSIGNMENTS.md` backend-refactor row aligned with actual execution state.

## Current Status Log
- `2026-02-18`: project started with consistency decisions and phased plan.
- `2026-02-18`: non-physics backend standardization executed (factory seams/layout/header naming/stub policy/namespace cleanup).
- `2026-02-18`: this document refreshed to reflect as-built non-physics state; physics details intentionally tracked in `docs/projects/physics-refactor.md`.

## Handoff Checklist
- [x] Scope stayed in backend-refactor boundaries for this pass.
- [x] Non-physics factory/header/namespace standardization state captured.
- [x] `docs/projects/ASSIGNMENTS.md` row updated in the same handoff.
- [ ] Physics-lane closeout merged and BR7 marked complete.
