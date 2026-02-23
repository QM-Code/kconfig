# Cleanup S10 (`CLN-S10`): Naming + Directory Rationalization

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (major source/build ownership cleanup landed; follow-on naming still pending)`
- Immediate next task: continue cleanup of remaining legacy naming tokens and file naming drift now that `src/game`/`src/engine` build wiring ownership has been removed.
- Validation gate: `cd m-bz3 && ./abuild.py -c -d <bz3-build-dir>` and `cd m-karma && ./abuild.py -c -d <karma-build-dir>`.

## Mission
Align naming and directory ownership with actual runtime/build responsibilities, reducing ambiguity and navigation cost.

## Foundation References
- `projects/cleanup.md`
- `m-bz3/cmake/targets/*`
- `m-bz3/src/client/game/game.hpp`
- `m-karma/cmake/40_sdk_subdir.cmake`
- `m-karma/cmake/sdk/*`

## Why This Is Separate
Naming/layout normalization is cross-cutting and often mechanical; isolating it prevents noise in behavior-changing tracks.

## Owned Paths
- `m-bz3/cmake/*`
- `m-bz3/src/client/*` (naming/layout only)
- `m-karma/cmake/*`
- `m-overseer/agent/projects/cleanup/naming-directory-rationalization.md`

## Interface Boundaries
- Inputs consumed:
  - structure decisions from active cleanup tracks.
- Outputs exposed:
  - consistent naming conventions and path ownership.
- Coordinate before changing:
  - `projects/cleanup.md`
  - `projects/ui.md`

## Non-Goals
- Do not combine behavior refactors with broad rename churn in one slice.
- Do not rename public SDK contracts without explicit migration decision.

## Validation
```bash
cd m-bz3
./abuild.py -c -d <bz3-build-dir>

cd ../m-karma
./abuild.py -c -d <karma-build-dir>
```

## Trace Channels
- `cleanup.s10`
- `build.layout`

## Build/Run Commands
```bash
cd m-bz3
./abuild.py -c -d <bz3-build-dir>
```

## Current Status
- `2026-02-21`: major ownership normalization landed (`src/game` and `src/engine` CMake ownership removed).
- `2026-02-22`: follow-on naming cleanup tracked as dedicated child lane under superproject.

## Open Questions
- With `CLN-S8` archived, should any remaining backend naming canonicalization (`sdl3audio` style) be tracked as `CLN-S10` naming debt?
- Which remaining path tokens should be treated as archival-only and left unchanged?

## Handoff Checklist
- [ ] Remaining high-friction naming drift identified and prioritized.
- [ ] Renames applied in behavior-neutral slices.
- [ ] Build wiring and scripts updated for each rename wave.
