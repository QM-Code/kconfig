# Lone CMake Aggregator Cleanup (`m-bz3/src/game`)

## Project Snapshot
- Current owner: `overseer`
- Status: `not started`
- Immediate next task: run `M0` baseline mapping of current `m-bz3/src/game/CMakeLists.txt` responsibilities and prepare a no-behavior-change relocation plan.
- Validation gate:
  - `m-overseer`: `./scripts/lint-config-projects.sh` (currently expected to fail until lint path drift is corrected)
  - `m-bz3`: `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock`
  - `m-bz3`: `./scripts/test-server-net.sh build-sdk`

## Mission
Normalize `m-bz3` build structure by removing the current "lone CMake aggregator" pattern where `src/game/` contains only `CMakeLists.txt` plus `game.hpp`, while preserving behavior and build outputs.

## Foundation References
- `m-bz3/CMakeLists.txt`
- `m-bz3/src/game/CMakeLists.txt`
- `m-bz3/src/game/game.hpp`
- `m-overseer/projects/headers.md`

## Why This Is Separate
This is a structural maintainability cleanup. It should be tracked independently from active SDK packaging/hardening work (`headers.md`, `diligent.md`) so scope does not expand into unrelated integration changes.

## Baseline Findings (2026-02-19)
1. `m-bz3/CMakeLists.txt` delegates target definition through `add_subdirectory(src/game)`.
2. `m-bz3/src/game/CMakeLists.txt` defines broad project targets (`bz3`, `bz3-server`, proto lib, and tests) using files from multiple `src/*` trees.
3. `m-bz3/src/game/` currently contains only:
   - `CMakeLists.txt`
   - `game.hpp`
4. The file placement works but obscures ownership and increases confusion during cross-repo cleanup.

## Owned Paths
- `m-overseer/projects/lone-cmake.md`
- `m-overseer/projects/ASSIGNMENTS.md`
- `m-bz3/CMakeLists.txt`
- `m-bz3/src/game/CMakeLists.txt`
- `m-bz3/src/CMakeLists.txt` (new file, if introduced)
- `m-bz3/src/game/game.hpp` and affected include call sites (only if relocation is approved in-scope)

## Interface Boundaries
- Inputs consumed:
  - existing `m-bz3` build graph and target definitions
- Outputs exposed:
  - clearer `m-bz3` CMake layout with unchanged target behavior
- Coordinate before changing:
  - `m-bz3/CMakeLists.txt`
  - `m-bz3/src/game/CMakeLists.txt`
  - `m-overseer/projects/headers.md` (if include path changes overlap)

## Non-Goals
- No gameplay/network/render/physics behavior changes.
- No KARMA SDK contract changes.
- No Diligent packaging work.
- No broad source-tree reshuffle beyond the minimum needed to remove the lone-aggregator pattern.

## Execution Plan
1. `M0` baseline map
- Capture exact targets, sources, include dirs, and link dependencies currently declared in `m-bz3/src/game/CMakeLists.txt`.

2. `M1` no-op structural relocation
- Move aggregator CMake logic to `m-bz3/src/CMakeLists.txt`.
- Update top-level `add_subdirectory(src/game)` to `add_subdirectory(src)`.
- Keep generated targets and command outputs identical.

3. `M2` optional header placement cleanup
- Decide whether `m-bz3/src/game/game.hpp` remains in place or moves to a more specific location with a compatibility forwarding include.

4. `M3` validation and closeout
- Rebuild against SDK and run required server-net tests.
- Record evidence that behavior did not change.

## Validation
```bash
# Build against current KARMA SDK export
cd m-bz3
./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock

# Net/server regression coverage
./scripts/test-server-net.sh build-sdk
```

## Build/Run Commands
```bash
# Inspect current delegation
rg -n "add_subdirectory\\(src/game\\)" m-bz3/CMakeLists.txt

# Inspect current source aggregation
rg -n "add_executable\\(|add_library\\(|target_include_directories\\(|target_link_libraries\\(" m-bz3/src/game/CMakeLists.txt
```

## First Session Checklist
1. Confirm this track stays structural-only.
2. Complete `M0` target/source mapping.
3. Implement `M1` relocation as no-op behavior change.
4. Run required validation.
5. Update this doc and `ASSIGNMENTS.md` in the same handoff.

## Current Status
- `2026-02-19`: Project created to track deferred cleanup of lone `src/game/CMakeLists.txt` aggregator layout.

## Open Questions
- Should `m-bz3/src/game/game.hpp` remain for include stability, or move with a forwarding header?
- Should this cleanup run before or after `headers.md` H0/H1 include-boundary work to reduce merge/conflict risk?

## Handoff Checklist
- [ ] `M0` baseline mapping documented
- [ ] `M1` relocation landed with no target behavior drift
- [ ] Validation commands run and summarized
- [ ] Assignments row updated
- [ ] Risks/open questions recorded
