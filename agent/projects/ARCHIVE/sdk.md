# SDK Naming + Contract Consolidation

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (SDK-S1 landed and validated; CLI naming intentionally retained)`
- Immediate next task: retire/archive this project track unless new SDK contract deltas are requested.
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-karma`: `./abuild.py -c -d build-sdk -b bgfx,diligent --install-sdk out/karma-sdk`
  - `m-bz3`: `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk`

## Mission
Rename SDK package and target naming away from `Engine/engine` terminology in one intentional migration so naming is consistent, explicit, and SDK-first.

Primary direction:
- package name: `KarmaEngine` -> `KarmaSDK`
- imported targets: `karma::engine_core` / `karma::engine_client` -> `karma::core` / `karma::client`
- internal CMake variable namespace: prefer `KARMA_SDK_*` over `KARMA_ENGINE_*`

## Foundation References
- `m-overseer/agent/projects/ARCHIVE/multiplatform.md`
- `m-overseer/agent/docs/building.md`
- `m-karma/cmake/10_backend_options.cmake`
- `m-karma/cmake/50_sdk_install_export.cmake`
- `m-karma/cmake/60_package_config.cmake`
- `m-karma/cmake/KarmaSDKConfig.cmake.in`
- `m-bz3/cmake/40_karma_sdk.cmake`
- `m-bz3/abuild.py`

## Why This Is Separate
- This is a naming/API contract migration that can ripple across both producer (`m-karma`) and consumer (`m-bz3`).
- It should be staged after SDK-only mode work, not mixed into multiplatform packaging slices.
- Keeping this separate avoids conflating packaging defects with naming-contract migration regressions.

## Owned Paths
- `m-overseer/agent/projects/sdk.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`
- `m-overseer/agent/docs/building.md`
- `m-karma/cmake/*`
- `m-karma/src/engine/cmake/*`
- `m-bz3/cmake/*`
- `m-bz3/abuild.py`

## Interface Boundaries
- Inputs consumed:
  - SDK-only producer contract from archived `multiplatform.md` (`MP0-SDK1`).
  - existing package-based consumer contract (`find_package(...)` + imported targets).
- Outputs/contracts exposed:
  - renamed package config and target names.
  - updated local build instructions for producer and consumer.
- Coordinate before changing:
  - exported package config filenames/install layout.
  - imported target names consumed in `m-bz3`.
  - any scripts/docs using `KarmaEngine` naming.

## Non-Goals
- No renderer/physics/audio behavior changes.
- No backend feature work.
- No compatibility alias transition pass (explicitly out of scope by decision).
- No dependency-version refresh campaign.

## Strategic Track Labels
- Primary track: `shared unblocker`
- Secondary track: `sdk contract hardening`

## Execution Plan (Bounded Slices)

### `SDK-S1` (single implementation slice)
- Track: `shared unblocker`
- Goal: complete package/target/namespace rename to SDK-first naming in one pass without compatibility aliases.
- Scope:
  - producer package contract:
    - `find_package(KarmaSDK CONFIG REQUIRED)` replaces `KarmaEngine`.
    - package install directory becomes `lib/cmake/KarmaSDK` only.
    - config/targets install names move from `KarmaEngine*` to `KarmaSDK*`.
  - producer exported targets:
    - `karma::engine_core` -> `karma::core`
    - `karma::engine_client` -> `karma::client`
  - consumer integration updates:
    - `m-bz3` package lookup, target links, and `abuild.py` SDK-prefix validation align to `KarmaSDK`.
  - eradicate legacy naming strings from project-owned code/docs called out in this file.
- Intended files:
  - `m-karma/cmake/10_backend_options.cmake`
  - `m-karma/cmake/50_sdk_install_export.cmake`
  - `m-karma/cmake/60_package_config.cmake`
  - `m-karma/cmake/KarmaSDKConfig.cmake.in`
  - `m-karma/abuild.py`
  - `m-karma/scripts/test-sdk-mobile-static.sh`
  - `m-bz3/cmake/10_backend_options.cmake`
  - `m-bz3/cmake/40_karma_sdk.cmake`
  - `m-bz3/abuild.py`
  - `m-overseer/agent/docs/building.md` (if legacy contract strings remain)
  - `m-overseer/agent/projects/sdk.md`
  - `m-overseer/agent/projects/ASSIGNMENTS.md`
- Acceptance:
  - producer/consumer validation commands in this doc pass.
  - strict zero-hit gate for `karma_engine` and `KarmaEngine` passes in required paths.
  - no compatibility alias targets or mirrored legacy package dirs introduced.

## Validation
- Run these validations after each bounded rename slice and once at final consolidation:

```bash
# overseer docs
cd m-overseer
./agent/scripts/lint-projects.sh

# producer sdk build/install
cd ../m-karma
export ABUILD_AGENT_NAME=<agent-name>
./abuild.py --claim-lock -d build-sdk
./abuild.py -c -d build-sdk -b bgfx,diligent --install-sdk out/karma-sdk
./abuild.py --release-lock -d build-sdk

# consumer build against installed sdk
cd ../m-bz3
export ABUILD_AGENT_NAME=<agent-name>
./abuild.py --claim-lock -d build-sdk
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk
./abuild.py --release-lock -d build-sdk
```

## Hard Acceptance Gate (String Eradication)
- Final migration is accepted only when both legacy strings have zero hits in:
  - `m-karma/abuild.py`
  - `m-karma/cmake/`
  - `m-karma/out/`
  - `m-karma/scripts/`
  - `m-karma/src/`
- Strings:
  - `karma_engine`
  - `KarmaEngine`
- Because `out/` is generated, enforce this sequence before grep validation:
  1. remove/regenerate stale generated package/install artifacts under `out/`.
  2. rerun producer build/install and consumer validation.
  3. run strict grep checks.

```bash
# strict zero-hit gate (must print no matches)
cd m-karma
grep -Rin "karma_engine\\|KarmaEngine" abuild.py cmake out scripts src
```

## Trace Channels
- `build.sdk`
- `build.cmake`

## Build/Run Commands
```bash
# quick producer/consumer smoke
cd m-karma
./abuild.py -c -d build-sdk --install-sdk out/karma-sdk

cd ../m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk
```

## First Session Checklist
1. Confirm archived `multiplatform.md` `MP0-SDK1` closeout is complete in shared branch history. (done)
2. List exact rename set for package names, targets, and variable namespaces.
3. Apply `SDK-S1` producer rename slice in `m-karma` package/export path.
4. Apply `SDK-S1` consumer updates in `m-bz3`.
5. Run producer+consumer validation and update handoff notes.

## Current Status
- `2026-02-21`: project created to track post-`MP0-SDK1` naming migration.
- `2026-02-21`: policy decision captured: execute one clean rename pass without compatibility aliases.
- `2026-02-21`: precondition verified: archived `multiplatform.md` records `MP0-SDK1` as implemented and Linux producer+consumer validations passing.
- `2026-02-21`: first bounded execution slice defined as `SDK-S1` (single-pass rename, no compatibility aliases).
- `2026-02-21`: `SDK-S1` implemented across producer+consumer contract surfaces:
  - producer package renamed to `KarmaSDK` with install path `lib/cmake/KarmaSDK`,
  - package/config/targets filenames renamed to `KarmaSDK*`,
  - exported/imported targets renamed to `karma::core` and `karma::client`,
  - internal CMake namespace migrated from `KARMA_ENGINE_*` policy variables to `KARMA_SDK_*` where touched by SDK packaging contract,
  - consumer package lookup and local CMake target aliases updated to the renamed contract.
- `2026-02-21`: validation evidence (`SDK-S1`):
  - `cd m-overseer && ./agent/scripts/lint-projects.sh` (pass)
  - `cd m-karma && export ABUILD_AGENT_NAME=overseer-sdk-s1 && ./abuild.py --claim-lock -d build-sdk && rm -rf out/karma-sdk out/karma-sdk-static && ./abuild.py -c -d build-sdk -b bgfx,diligent --install-sdk out/karma-sdk && ./abuild.py --release-lock -d build-sdk` (pass)
  - `cd m-bz3 && export ABUILD_AGENT_NAME=overseer-sdk-s1 && ./abuild.py --claim-lock -d build-sdk && ./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk && ./abuild.py --release-lock -d build-sdk` (pass)
  - `cd m-karma && export ABUILD_AGENT_NAME=overseer-sdk-s1 && ./abuild.py --claim-lock -d build-sdk-static && ./abuild.py -c -d build-sdk-static --sdk-linkage static --install-sdk out/karma-sdk-static && ./scripts/test-sdk-mobile-static.sh out/karma-sdk-static && ./abuild.py --release-lock -d build-sdk-static` (pass)
  - `cd m-karma && ./scripts/check-sdk-header-allowlist.sh` (pass)
  - `cd m-karma && grep -Rin "karma_engine\\|KarmaEngine" abuild.py cmake out scripts src` (no matches)

## Open Questions
- Decision (`2026-02-21`): keep `--karma-sdk` and `KARMA_SDK_ROOT` as-is for explicit consumer intent in `m-bz3`.

## Handoff Checklist
- [x] Scope and precondition evidence documented
- [x] `SDK-S1` landed and validated
- [x] Code updated
- [x] Tests run and summarized
- [x] Docs updated
- [x] Risks/open questions listed
- [x] Zero-hit grep gate passed for `karma_engine` and `KarmaEngine` in required paths
