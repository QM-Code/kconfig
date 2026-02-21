# Diligent SDK Packaging

## Project Snapshot
- Current owner: `codex`
- Status: `completed`
- Immediate next task: archive-ready; no active implementation work remains.
- Validation gate:
  - `m-overseer`: `./scripts/lint-projects.sh`
  - `m-karma`: `./docs/scripts/lint-project-docs.sh`
  - `m-karma`: `./abuild.py -c -d build-sdk-diligent-port -b diligent --install-sdk out/karma-sdk-diligent-port --ignore-lock`
  - `m-bz3`: `./abuild.py -c -d build-sdk-diligent-port --karma-sdk ../m-karma/out/karma-sdk-diligent-port --ignore-lock`

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
./scripts/lint-projects.sh

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
- `2026-02-20`: Completed package-managed Diligent integration for SDK export safety.
  - `m-karma` switched from in-tree `FetchContent` Diligent to `find_package(DiligentEngine CONFIG REQUIRED)`.
  - Added local vcpkg overlay port `diligentengine` (`m-karma/vcpkg-overlays/diligentengine`) with bundled Wayland/Vulkan patch application.
  - Added package config wiring so Diligent public build definitions (e.g., `PLATFORM_LINUX=1`, backend support flags) propagate to SDK consumers.
  - Preserved relocatable SDK export of `karma::engine_client` for Diligent-enabled installs.
  - Validation completed:
    - `cd m-karma && ABUILD_AGENT_NAME=specialist-diligent-d0 ./abuild.py -c -d build-sdk-diligent-port -b diligent --install-sdk out/karma-sdk-diligent-port --ignore-lock` (pass)
    - `cd m-bz3 && ABUILD_AGENT_NAME=specialist-diligent-d0 ./abuild.py -c -d build-sdk-diligent-port --karma-sdk ../m-karma/out/karma-sdk-diligent-port --ignore-lock` (pass)
    - `cd m-bz3 && ABUILD_AGENT_NAME=specialist-diligent-d0 ./abuild.py -c -d build-sdk-diligent-port-diligent -b diligent --karma-sdk ../m-karma/out/karma-sdk-diligent-port --ignore-lock` (pass)

## Open Questions
- Resolved (`2026-02-20`): Wayland patch lives in a vcpkg overlay port (`m-karma/vcpkg-overlays/diligentengine`) rather than a maintained fork.
- Resolved (`2026-02-20`): Diligent-only export-safe acceptance path is sufficient for this track.

## Handoff Checklist
- [x] Package-managed Diligent strategy selected and documented
- [x] KARMA CMake switched off in-tree FetchContent Diligent for SDK export path
- [x] Diligent-enabled SDK export includes `karma::engine_client` relocatably
- [x] BZ3 consumes Diligent-capable KARMA SDK via `find_package(KarmaEngine)` without manual wiring
- [x] Docs and tracker rows updated
