# Diligent SDK Packaging

## Project Snapshot
- Current owner: `overseer`
- Status: `not started`
- Immediate next task: produce a bounded implementation packet for replacing `FetchContent` Diligent with package-managed Diligent in `m-karma`.
- Validation gate:
  - `m-overseer`: `./scripts/lint-config-projects.sh`
  - `m-karma`: `./docs/scripts/lint-project-docs.sh`
  - `m-bz3`: `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock`

## Mission
Make Diligent export-safe for the KARMA SDK path so `m-bz3` can consume `karma::engine_client` through `find_package(KarmaEngine CONFIG REQUIRED)` even when Diligent renderer support is enabled in `m-karma`.

## Foundation References
- `m-karma/CMakeLists.txt`
- `m-karma/src/engine/CMakeLists.txt`
- `m-karma/cmake/KarmaEngineConfig.cmake.in`
- `m-karma/cmake/diligent_wayland_apply.py`

## Why This Is Separate
This is a cross-repo integration contract task with packaging implications and should be tracked independently from script ownership cleanup and gameplay tracks.

## Owned Paths
- `m-karma/CMakeLists.txt`
- `m-karma/src/engine/CMakeLists.txt`
- `m-karma/cmake/KarmaEngineConfig.cmake.in`
- `m-karma/vcpkg.json`
- `m-karma/vcpkg-overlays/*` (if needed)
- `m-overseer/config/projects/diligent.md`
- `m-overseer/config/projects/ASSIGNMENTS.md`

## Interface Boundaries
- Inputs consumed:
  - current in-tree Diligent integration in `m-karma`
- Outputs exposed:
  - Diligent-enabled SDK export path that remains relocatable and package-consumable.
- Coordinate before changing:
  - `m-karma/CMakeLists.txt`
  - `m-karma/src/engine/CMakeLists.txt`
  - `m-karma/cmake/KarmaEngineConfig.cmake.in`
  - `m-bz3/CMakeLists.txt` (only if consumer-side contract wiring needs adjustment)

## Non-Goals
- No gameplay feature work.
- No renderer behavior changes unrelated to packaging/export.
- No contract changes away from `find_package(KarmaEngine CONFIG REQUIRED)` for BZ3.

## Validation
```bash
# Overseer tracking docs
cd m-overseer
./scripts/lint-config-projects.sh

# Karma docs and SDK export
cd ../m-karma
./docs/scripts/lint-project-docs.sh
./abuild.py -c -d build-sdk --install-sdk out/karma-sdk

# BZ3 SDK consume check
cd ../m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock
```

## Trace Channels
- `render.system`
- `render.diligent`
- `engine.app`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d build-sdk -b diligent
./abuild.py -c -d build-sdk --install-sdk out/karma-sdk

cd ../m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock
```

## First Session Checklist
1. Confirm current Diligent guard that exports only `karma_engine_core` when Diligent is enabled.
2. Design package-managed Diligent path (overlay or fork package) with Wayland patch preserved.
3. Replace in-tree Diligent linking with package targets in KARMA client link graph.
4. Re-enable `karma_engine_client` export for Diligent profile once relocatable dependencies are in place.
5. Validate SDK install + BZ3 consume path end-to-end.

## Current Status
- `2026-02-19`: Project initialized from overseer planning after confirming Diligent is currently non-export-safe in the SDK path.

## Open Questions
- Should Wayland support patch live in a vcpkg overlay port or a maintained Diligent fork package?
- Is dual-backend bgfx+diligent packaging required for first acceptance, or is diligent-only export-safe path sufficient initially?

## Handoff Checklist
- [ ] Package-managed Diligent strategy selected and documented
- [ ] KARMA CMake switched off in-tree FetchContent Diligent for SDK export path
- [ ] Diligent-enabled SDK export includes `karma::engine_client` relocatably
- [ ] BZ3 consumes Diligent-capable KARMA SDK via `find_package(KarmaEngine)` without manual wiring
- [ ] Docs and tracker rows updated
