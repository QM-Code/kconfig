# CMake Structure Harmonization (`m-bz3` + `m-karma`)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress` (project created; dual-agent dispatch pending)
- Immediate next task: dispatch parallel specialist packets for `BZ3-S1` and `KARMA-S1` with a shared symmetry contract and no-behavior-change constraints.
- Validation gate:
  - `m-overseer`: `./scripts/lint-projects.sh`
  - `m-bz3`: `./abuild.py -c -d <bz3-build-dir>` and `./scripts/test-server-net.sh <bz3-build-dir>`
  - `m-karma`: `./abuild.py -c -d <karma-build-dir>`, `./scripts/test-engine-backends.sh <karma-build-dir>`, and `./scripts/test-server-net.sh <karma-build-dir>`

## Mission
Coordinate two separate specialists in parallel to split monolithic source-root CMake coordinators into clearer, smaller include fragments while preserving target names, outputs, and runtime behavior:
- `m-bz3/src/game/CMakeLists.txt`
- `m-karma/src/engine/CMakeLists.txt`

## Foundation References
- `m-bz3/CMakeLists.txt`
- `m-bz3/src/game/CMakeLists.txt`
- `m-karma/CMakeLists.txt`
- `m-karma/src/engine/CMakeLists.txt`
- `m-overseer/projects/archive/lone-cmake-retired-2026-02-21.md`
- `m-overseer/templates/SPECIALIST_PACKET.md`

## Why This Is Separate
This is shared build-system maintenance across two repos and should not compete with gameplay/physics integration throughput. It needs overseer-led symmetry control and parallel execution.

## Baseline Findings
1. `m-bz3/CMakeLists.txt` delegates source target graph definition with `add_subdirectory(src/game)`.
2. `m-karma/CMakeLists.txt` delegates source target graph definition with `add_subdirectory(src/engine)`.
3. Both delegated files act as broad target aggregators. The cleanup goal is parity-friendly decomposition, not behavior change.

## Owned Paths
- `m-overseer/projects/cmake.md`
- `m-overseer/projects/ASSIGNMENTS.md`
- `m-bz3/src/game/CMakeLists.txt`
- `m-bz3/src/game/cmake/*` (new)
- `m-karma/src/engine/CMakeLists.txt`
- `m-karma/src/engine/cmake/*` (new)

## Interface Boundaries
- Inputs consumed:
  - existing target/source/include/link/install definitions in both repos
- Outputs exposed:
  - unchanged target names, output binaries/libraries, install/export contracts, and test registration behavior
- Coordinate before changing:
  - `m-bz3/CMakeLists.txt`
  - `m-karma/CMakeLists.txt`
  - any SDK export/install sections in `m-karma/src/engine/CMakeLists.txt`

## Non-Goals
- No gameplay, physics, networking, or rendering behavior changes.
- No target rename/retype/output-path changes.
- No dependency-version/toolchain policy changes.
- No SDK packaging contract churn beyond mechanical refactor needs.

## Execution Plan
1. `C0` overseer symmetry contract (planning packet)
- Define fragment naming/mirroring rules both specialists must follow.
- Define forbidden changes (target names, exports, runtime flags).

2. `BZ3-S1` (specialist A; in parallel)
- Refactor `m-bz3/src/game/CMakeLists.txt` into included fragment files under `m-bz3/src/game/cmake/`.
- Keep `m-bz3/src/game/CMakeLists.txt` as thin coordinator.
- Preserve all current targets/tests and build outputs.

3. `KARMA-S1` (specialist B; in parallel)
- Refactor `m-karma/src/engine/CMakeLists.txt` into included fragment files under `m-karma/src/engine/cmake/`.
- Keep `m-karma/src/engine/CMakeLists.txt` as thin coordinator.
- Preserve current engine targets, tests, and SDK/export/install behavior.

4. `C1` overseer convergence and symmetry check
- Compare both lanes for structure consistency.
- Approve only if both are no-behavior-change and validation-clean.

## Validation
```bash
# m-bz3 lane
cd m-bz3
export ABUILD_AGENT_NAME=specialist-cmake-bz3
./abuild.py --claim-lock -d <bz3-build-dir>
./abuild.py -c -d <bz3-build-dir>
./scripts/test-server-net.sh <bz3-build-dir>
./abuild.py --release-lock -d <bz3-build-dir>

# m-karma lane
cd ../m-karma
export ABUILD_AGENT_NAME=specialist-cmake-karma
./abuild.py --claim-lock -d <karma-build-dir>
./abuild.py -c -d <karma-build-dir>
./scripts/test-engine-backends.sh <karma-build-dir>
./scripts/test-server-net.sh <karma-build-dir>
./abuild.py --release-lock -d <karma-build-dir>

# overseer docs
cd ../m-overseer
./scripts/lint-projects.sh
```

## Build/Run Commands
```bash
# symmetry baseline checks
rg -n "add_subdirectory\\(src/game\\)" m-bz3/CMakeLists.txt
rg -n "add_subdirectory\\(src/engine\\)" m-karma/CMakeLists.txt
rg -n "add_executable\\(|add_library\\(|target_link_libraries\\(" m-bz3/src/game/CMakeLists.txt
rg -n "add_executable\\(|add_library\\(|target_link_libraries\\(" m-karma/src/engine/CMakeLists.txt
```

## First Session Checklist
1. Publish `C0` symmetry contract and specialist packet boundaries.
2. Launch `BZ3-S1` and `KARMA-S1` in parallel (distinct build directories).
3. Require each specialist handoff to include exact commands/results and unchanged-target assurances.
4. Perform overseer convergence diff review.
5. Update this doc and `ASSIGNMENTS.md`.

## Current Status
- `2026-02-21`: Project created for coordinated dual-agent CMake decomposition across `m-bz3` and `m-karma`, superseding standalone `lone-cmake` planning.

## Open Questions
- Should fragment file names be fully mirrored across repos (`client.cmake`, `server.cmake`, `tests.cmake`) or repo-specific to existing domain naming?
- Should both lanes include a one-shot script check that verifies target-count parity before/after split?

## Handoff Checklist
- [ ] `C0` symmetry contract published
- [ ] `BZ3-S1` landed and validated
- [ ] `KARMA-S1` landed and validated
- [ ] Cross-repo symmetry review completed
- [ ] `ASSIGNMENTS.md` updated
