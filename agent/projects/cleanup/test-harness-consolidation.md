# Cleanup S9 (`CLN-S9`): Test Harness Consolidation

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued`
- Immediate next task: identify repeated test helper patterns (`Expect`, `Fail`, `WaitUntil`, fixture bootstrap) and propose shared support utility modules.
- Validation gate: `cd m-karma && ./abuild.py -c -d <karma-build-dir>` and targeted `ctest` for touched suites.

## Mission
Reduce duplicated test harness code by consolidating common helpers while keeping contract tests readable and deterministic.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/network/tests/*`
- `m-karma/src/physics/tests/*`
- `m-bz3/src/tests/*`

## Why This Is Separate
This track targets test architecture quality and can run in parallel with runtime and renderer cleanup slices.

## Owned Paths
- `m-karma/src/tests/support/*`
- `m-karma/src/network/tests/support/*`
- `m-bz3/src/tests/*` (as needed)
- `m-overseer/agent/projects/cleanup/test-harness-consolidation.md`

## Interface Boundaries
- Inputs consumed:
  - existing test suite helper behavior and fixture expectations.
- Outputs exposed:
  - shared test support utilities with migration guidance.
- Coordinate before changing:
  - `projects/cleanup/physics-parity-suite.md`

## Non-Goals
- Do not rewrite test assertions semantically.
- Do not couple unrelated test domains into one giant framework.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
ctest --test-dir <karma-build-dir> -R ".*transport_contract_test|physics_backend_parity_.*|directional_shadow_contract_test" --output-on-failure
```

## Trace Channels
- `cleanup.s9`
- `tests.harness`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
```

## Current Status
- `2026-02-21`: repeated test helper patterns identified as `P2` lane.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- Should shared helpers live in one central support library or per-domain support modules with a tiny common core?
- What migration order minimizes noisy diffs across active test tracks?

## Handoff Checklist
- [ ] Duplicate helper inventory complete.
- [ ] Shared helper modules landed.
- [ ] Representative suites migrated and validated.
