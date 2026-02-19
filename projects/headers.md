# KARMA SDK Header Allowlist

## Project Snapshot
- Current owner: `overseer`
- Status: `not started`
- Immediate next task: run H0 baseline packet to remove header-shadowing from one `m-bz3` target and validate SDK-only include resolution.
- Validation gate:
  - `m-overseer`: `./scripts/lint-config-projects.sh`
  - `m-karma`: `./abuild.py -c -d build-sdk --install-sdk out/karma-sdk`
  - `m-bz3`: `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock`

## Mission
Narrow `m-karma` SDK-installed headers from broad `include/karma/*` copy to an explicit allowlist while ensuring `m-bz3` actually consumes SDK headers (not local shadow copies) through the locked `find_package(KarmaEngine CONFIG REQUIRED)` integration contract.

## Foundation References
- `config/projects/diligent.md`
- `m-karma/CMakeLists.txt`
- `m-karma/src/engine/CMakeLists.txt`
- `m-bz3/CMakeLists.txt`
- `m-bz3/src/game/CMakeLists.txt`

## Why This Is Separate
This is a cross-repo packaging and boundary-hardening effort with direct impact on long-term SDK stability. It should be tracked independently from Diligent packaging and gameplay/renderer feature tracks.

## Baseline Findings (2026-02-19)
1. `m-karma` currently installs SDK headers with broad directory copy:
   - `install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/karma DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} ...)` in `m-karma/CMakeLists.txt`.
2. Current `m-karma/include/karma/*` header count: `82`.
3. Unique `karma/*` headers directly included by `m-bz3/src/*`: `58`.
4. `m-bz3/src/*` currently references 3 headers that are not present in `m-karma/include/karma`:
   - `karma/app/ui_context.h`
   - `karma/input/bindings_text.hpp`
   - `karma/graphics/texture_handle.hpp`
5. `m-bz3` still adds local include roots to targets (`${PROJECT_SOURCE_DIR}/include`, `${PROJECT_SOURCE_DIR}/src`) in `m-bz3/src/game/CMakeLists.txt`, so header shadowing can mask SDK drift.

## Issue Matrix
1. Header shadowing in consumer repo (`m-bz3`) prevents reliable SDK-boundary validation.
2. Missing/legacy include paths in `m-bz3` need either migration to existing public headers or explicit promotion into SDK surface.
3. SDK install list in `m-karma` is over-broad and does not encode intended public surface.
4. No automated guard exists to prevent accidental public-header surface growth.
5. Allowlist changes must preserve locked KARMA -> BZ3 package contract.

## Owned Paths
- `m-overseer/config/projects/headers.md`
- `m-overseer/config/projects/ASSIGNMENTS.md`
- `m-karma/CMakeLists.txt`
- `m-karma/include/karma/*` (public-header declarations only)
- `m-karma/cmake/*` (if helper manifest/list file is introduced)
- `m-karma/scripts/*` (if allowlist guard script is introduced)
- `m-bz3/CMakeLists.txt`
- `m-bz3/src/game/CMakeLists.txt`
- `m-bz3/src/**` include statements that require migration

## Interface Boundaries
- Inputs consumed:
  - current packaging/export behavior in `m-karma`
  - current consumer include/link behavior in `m-bz3`
- Outputs exposed:
  - explicit, reviewable SDK header allowlist
  - consumer build path that resolves SDK headers without local shadowing
  - automated drift checks for SDK header surface
- Coordinate before changing:
  - `m-karma/CMakeLists.txt`
  - `m-bz3/CMakeLists.txt`
  - `m-bz3/src/game/CMakeLists.txt`
  - `projects/diligent.md` (only for cross-impact notes)

## Non-Goals
- No renderer/audio/physics behavior changes.
- No gameplay feature implementation.
- No change to official BZ3 consume contract (`find_package(KarmaEngine)` stays required).
- No Diligent packaging refactor in this track (owned by `diligent.md`).

## Execution Plan
1. H0: Consumer shadowing baseline
- Identify and document exact `m-bz3` targets currently relying on local `include/karma/*` shadowing.
- Run one bounded target slice proving SDK-only include resolution in practice.

2. H1: Resolve missing header references
- For each missing include (`ui_context.h`, `bindings_text.hpp`, `texture_handle.hpp`):
  - map to existing public SDK header if equivalent exists, or
  - explicitly decide to promote a new header into `m-karma/include/karma/*`.
- Land migrations before tightening install allowlist.

3. H2: Define explicit allowlist
- Build allowlist from:
  - required public entry headers for exported targets,
  - transitive `karma/*` includes required by those headers,
  - additional explicitly approved consumer headers.
- Store allowlist in a maintained manifest (CMake list or dedicated file).

4. H3: Replace broad install with allowlist install
- Remove broad `install(DIRECTORY ... include/karma ...)`.
- Install only allowlisted headers in stable paths under `${CMAKE_INSTALL_INCLUDEDIR}`.
- Keep package config/targets unchanged unless required by header-surface update.

5. H4: Add drift guard
- Add a guard script/check that compares installed SDK header surface against allowlist.
- Fail on unapproved header growth.

6. H5: End-to-end validation
- Re-export SDK from `m-karma`.
- Build `m-bz3` against SDK path with no local header fallback.
- Confirm locked contract remains clean.

## Validation
```bash
# Overseer tracking
cd m-overseer
./scripts/lint-config-projects.sh

# Karma SDK export
cd ../m-karma
./abuild.py -c -d build-sdk --install-sdk out/karma-sdk

# BZ3 SDK consume
cd ../m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock
./scripts/test-server-net.sh build-sdk
```

## Build/Run Commands
```bash
# Audit current consumer includes
rg -o "karma/[A-Za-z0-9_./-]+\\.(hpp|h)" m-bz3/src -g"*.hpp" -g"*.h" -g"*.cpp" | sed 's#^.*/src/[^:]*:##' | sort -u

# Re-export SDK after header changes
cd m-karma
./abuild.py -c -d build-sdk --install-sdk out/karma-sdk

# Consume from BZ3
cd ../m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock
```

## First Session Checklist
1. Confirm and record all `m-bz3` local-header shadowing points.
2. Resolve the 3 currently missing include references or approve header promotions.
3. Draft and review initial SDK header allowlist manifest.
4. Switch installation from broad copy to explicit allowlist.
5. Add allowlist drift guard and run end-to-end SDK consume validation.

## Current Status
- `2026-02-19`: Project initialized; baseline issue set captured from current `m-karma`/`m-bz3` state.

## Open Questions
- Should `m-bz3/include/karma/*` be fully removed, or kept temporarily behind a controlled migration flag?
- Where should the allowlist manifest live (`m-karma/CMakeLists.txt` variable vs dedicated file under `m-karma/cmake/`)?
- Should SDK header-surface CI/guard run in `m-karma` only, or also be validated from `m-bz3` integration CI?

## Handoff Checklist
- [ ] Shadowing audit completed and documented
- [ ] Missing include mappings resolved (migrate or promote)
- [ ] Explicit SDK header allowlist implemented
- [ ] Broad directory install removed/replaced
- [ ] Drift guard added and passing
- [ ] End-to-end KARMA SDK export + BZ3 consume validation passing
