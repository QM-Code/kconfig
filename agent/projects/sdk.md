# SDK Naming + Contract Consolidation

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (queued behind multiplatform MP0-SDK1 completion)`
- Immediate next task: confirm `multiplatform.md` `MP0-SDK1` is merged, then dispatch one bounded rename slice with no compatibility aliases.
- Validation gate:
  - `m-overseer`: `./scripts/lint-projects.sh`
  - `m-karma`: `./abuild.py -c -d build-sdk -b bgfx,diligent --install-sdk out/karma-sdk`
  - `m-bz3`: `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk`

## Mission
Rename SDK package and target naming away from `Engine/engine` terminology in one intentional migration so naming is consistent, explicit, and SDK-first.

Primary direction:
- package name: `KarmaEngine` -> `KarmaSDK`
- imported targets: `karma::engine_core` / `karma::engine_client` -> `karma::core` / `karma::client`
- internal CMake variable namespace: prefer `KARMA_SDK_*` over `KARMA_ENGINE_*`

## Foundation References
- `m-overseer/agent/projects/multiplatform.md`
- `m-overseer/agent/docs/building.md`
- `m-karma/cmake/10_backend_options.cmake`
- `m-karma/cmake/50_sdk_install_export.cmake`
- `m-karma/cmake/60_package_config.cmake`
- `m-karma/cmake/KarmaEngineConfig.cmake.in`
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
  - SDK-only producer contract from `multiplatform.md` (`MP0-SDK1`).
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

## Validation
- Run these validations after each bounded rename slice and once at final consolidation:

```bash
# overseer docs
cd m-overseer
./scripts/lint-projects.sh

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
1. Confirm `multiplatform.md` `MP0-SDK1` is complete in shared branch history.
2. List exact rename set for package names, targets, and variable namespaces.
3. Apply rename slice in `m-karma` producer/package export.
4. Apply consumer updates in `m-bz3`.
5. Run producer+consumer validation and update handoff notes.

## Current Status
- `2026-02-21`: project created to track post-`MP0-SDK1` naming migration.
- `2026-02-21`: policy decision captured: execute one clean rename pass without compatibility aliases.

## Open Questions
- Should installed package directory be `lib/cmake/KarmaSDK` only, or do we keep a legacy mirror path for one release despite no alias policy?
- Should local command-line flags (`--karma-sdk`) be renamed now, or deferred to a follow-up UX cleanup slice?

## Handoff Checklist
- [ ] Code updated
- [ ] Tests run and summarized
- [ ] Docs updated
- [ ] Risks/open questions listed
- [ ] Zero-hit grep gate passed for `karma_engine` and `KarmaEngine` in required paths
