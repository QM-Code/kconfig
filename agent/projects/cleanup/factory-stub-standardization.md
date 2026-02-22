# Cleanup S8 (`CLN-S8`): Factory + Stub Standardization

## Project Snapshot
- Current owner: `overseer`
- Status: `queued (design contract captured; implementation pending)`
- Immediate next task: implement shared backend selector utility and migrate one subsystem factory as reference (`audio` or `physics`) before full rollout.
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
- `m-overseer/agent/projects/cleanup/factory-stub-standardization.md`

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

## Open Questions
- Should selector utility be header-only template or `.cpp` helper with typed adapters?
- Do we preserve legacy backend CLI tokens only, or add canonical+alias mapping?

## Handoff Checklist
- [ ] Shared selector utility introduced.
- [ ] All subsystem factories migrated.
- [ ] Contract tests verify parse/fallback/out-selected semantics across subsystems.
