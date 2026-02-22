# Cleanup S8 (`CLN-S8`): Factory + Stub Standardization

## Project Snapshot
- Current owner: `codex`
- Status: `complete (retired/archived after S8-1..S8-6 closeout)`
- Immediate next task: none (archived).
- Validation gate: `cd m-karma && ./abuild.py -c -d <karma-build-dir>` plus backend smoke/contract tests.

## Mission
Unify backend factory behavior (`parse/name/compiled/create`) and repetitive stub patterns across audio/physics/renderer/window/ui while preserving intentional subsystem-specific behavior.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/audio/backend_factory.cpp`
- `m-karma/src/physics/backend_factory.cpp`
- `m-karma/src/renderer/backend_factory.cpp`
- `m-karma/src/window/backend_factory.cpp`
- `m-karma/src/ui/backend_factory.cpp`

## Why This Is Separate
Factory standardization is high-leverage infrastructure work that can run independently from server/runtime and renderer-core decomposition.

## Owned Paths
- `m-karma/src/*/backend_factory.cpp`
- `m-karma/src/*/backends/factory_internal.hpp`
- `m-overseer/agent/projects/ARCHIVE/cleanup-factory-stub-standardization-retired-2026-02-22.md`

## Interface Boundaries
- Inputs consumed:
  - subsystem backend-kind enums and backend creation thunks.
- Outputs exposed:
  - shared selector contract and consistent fallback semantics.
- Coordinate before changing:
  - `projects/cleanup/config-path-resolver-dedupe.md`
  - `projects/cleanup/naming-directory-rationalization.md`

## Non-Goals
- Do not remove intentional behavior differences (`renderer::isValid`, UI software fallback, window wrapper).
- Do not change user-facing backend selection semantics without explicit alias policy.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
./scripts/test-engine-backends.sh <karma-build-dir>
ctest --test-dir <karma-build-dir> -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure
```

## Trace Channels
- `cleanup.s8`
- `engine.backend.factory`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
```

## Current Status
- `2026-02-21`: behavior contract and rollout direction documented.
- `2026-02-22`: moved under cleanup superproject child structure.
- `2026-02-22`: completed `S8-1` by introducing shared selector utility `src/common/backend/selector.hpp` (lowercase + alias parse + preferred/auto selection) and migrating `src/audio/backend_factory.cpp` as reference integration with no intended behavior changes; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: completed `S8-2` by migrating `src/physics/backend_factory.cpp` to shared `common/backend/selector.hpp` helpers (`ParseKindName`, `CreatePreferredBackend`) with no intended behavior changes; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: completed `S8-3` by migrating `src/window/backend_factory.cpp` to shared `common/backend/selector.hpp` helpers (`ParseKindName`, `CreatePreferredBackend`) with no intended behavior changes; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: completed `S8-4` by migrating `src/ui/backend_factory.cpp` to shared `common/backend/selector.hpp` helpers (`ParseKindName`, `CreatePreferredBackend`) while preserving `software-overlay` alias parsing; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: completed `S8-5` by migrating `src/renderer/backend_factory.cpp` to shared `common/backend/selector.hpp` helpers (`ParseKindName`, `CreatePreferredBackend`) while preserving `backend->isValid()` acceptance semantics; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: completed `S8-6` by adding `src/common/tests/backend_selector_contract_test.cpp` and wiring `backend_selector_contract_test` in `cmake/sdk/tests.cmake`, covering parse/alias behavior across audio/physics/renderer/window/ui plus out-selected/fallback behavior for audio/physics/ui; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "backend_selector_contract_test|physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: CLN-S8 closeout completed; subproject archived under `projects/ARCHIVE/cleanup-factory-stub-standardization-retired-2026-02-22.md`.

## Decision (`S8-1`)
- Decision: implement selector utility as header-only templates under `src/common/backend/selector.hpp`.
- Rationale: it must remain generic across backend enum types and backend pointer return types without adding new link-time wiring; header-only templates preserve type-specific call sites while removing duplicated logic.

## Open Questions
- None.

## Handoff Checklist
- [x] Shared selector utility introduced.
- [x] `S8-1` reference migration (`audio/backend_factory.cpp`) landed and validated.
- [x] `S8-2` migration (`physics/backend_factory.cpp`) landed and validated.
- [x] `S8-3` migration (`window/backend_factory.cpp`) landed and validated.
- [x] `S8-4` migration (`ui/backend_factory.cpp`) landed and validated.
- [x] All subsystem factories migrated.
- [x] Contract tests verify parse/fallback/out-selected semantics across subsystems.
