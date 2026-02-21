# Lone CMake Aggregator Cleanup (`m-bz3/src/game`) - Retired

## Retirement Snapshot
- Status: `retired`
- Retired on: `2026-02-21`
- Superseded by: `m-overseer/projects/cmake.md`
- Reason: scope expanded from single-repo investigation to coordinated dual-repo CMake decomposition (`m-bz3` + `m-karma`) with overseer-managed parallel specialist execution.

## Useful Carry-Over Notes
1. `m-bz3/CMakeLists.txt` delegates into `add_subdirectory(src/game)`.
2. `m-bz3/src/game/CMakeLists.txt` is a broad target aggregator touching multiple `src/*` trees.
3. The cleanup target is structural maintainability with no target/runtime behavior drift.

## Historical Context
- This track started as a single-repo investigation into the lone aggregator perception in `m-bz3`.
- After confirming structural symmetry with `m-karma/src/engine/CMakeLists.txt`, the effort was replaced with a shared cross-repo project so both repos can be refactored under one consistency contract.
