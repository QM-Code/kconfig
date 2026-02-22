# Cleanup S3 (`CLN-S3`): Config + Path Resolver Deduplication

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued`
- Immediate next task: inventory duplicate canonicalization/merge helpers between config store and path resolver, then define one shared utility surface.
- Validation gate: `cd m-karma && ./abuild.py -c -d <karma-build-dir>`.

## Mission
Remove duplicated config/path resolution logic and keep one authoritative implementation for canonicalization, merge behavior, and asset lookup flattening.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/common/config/store.cpp`
- `m-karma/src/common/data/path_resolver.cpp`

## Why This Is Separate
This is an engine utility-layer refactor that can proceed independently from server runtime, renderer core, and UI work.

## Owned Paths
- `m-karma/src/common/config/*`
- `m-karma/src/common/data/*`
- `m-overseer/agent/projects/cleanup/config-path-resolver-dedupe.md`

## Interface Boundaries
- Inputs consumed:
  - existing config/path call-site behavior expectations.
- Outputs exposed:
  - stable shared utility contract for canonicalization/merge resolution.
- Coordinate before changing:
  - `projects/cleanup/factory-stub-standardization.md`
  - `projects/cleanup/naming-directory-rationalization.md`

## Non-Goals
- Do not change public config key semantics.
- Do not broaden into unrelated content pipeline redesign.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
./scripts/test-engine-backends.sh <karma-build-dir>
```

## Trace Channels
- `cleanup.s3`
- `config.store`
- `data.path`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
```

## Current Status
- `2026-02-21`: identified duplicate helper families and designated as `P1` lane.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- Should shared logic live under `common/data` or a new `common/pathing` utility module?
- Which behavior should be codified first with contract tests before dedupe?

## Handoff Checklist
- [ ] Duplicate helper inventory complete.
- [ ] Shared utility extracted and call sites migrated.
- [ ] Engine build/tests pass with no behavior regressions.
