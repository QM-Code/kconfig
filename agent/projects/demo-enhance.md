# Karma Demo Runtime Enhancements

## Project Snapshot
- Current owner: `specialist-demo-s1`
- Status: `in progress (ENH-S0 baseline extraction complete; enhancement slices pending)`
- Immediate next task: execute `ENH-S1` by defining/landing first additive `--test-*` runtime mode contract without changing default client behavior yet.
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-karma`: `./abuild.py -c -d <build-dir>` + demo matrix/telemetry packet in this doc

## Mission
Carry forward post-`DEMO-S8B` improvements for demo runtime usability and maintainability:
- preserve current stable demo/runtime confidence gates,
- converge toward `client` defaulting to full graphics runtime,
- add explicit additive `--test-*` runtime modes for deterministic subsystem checks,
- reduce redundant ad-hoc script orchestration where equivalent in-runtime test modes are available,
- keep demo runtime binaries internal (not SDK-installed artifacts).

## Foundation References
- `m-karma/src/demo/client/main.cpp`
- `m-karma/src/demo/server/main.cpp`
- `m-karma/scripts/test-demo-trace-matrix.sh`
- `m-karma/scripts/test-demo-telemetry-summary.sh`
- `m-karma/scripts/test-demo-telemetry-trend.sh`
- `m-karma/.github/workflows/core-test-suite.yml`

## Why This Is Separate
The baseline demo-runtime track is functionally complete through telemetry trend tooling (`DEMO-S8B`).
This follow-on track isolates enhancement/productization decisions from the stabilized baseline so we can:
- keep regression risk bounded,
- treat script-to-runtime-mode convergence as incremental,
- keep baseline context consolidated in this active track.

## Extracted Outstanding Work (from prior baseline planning)
1. Continue Linux telemetry monitoring and open `DEMO-S8` only when trend policy escalates.
2. Convert remaining policy-level open questions into explicit implementation decisions.
3. Define first test-subsumption candidates and execute only after equivalent behavior coverage is proven.

## Policy Decisions (Operator-Provided)
1. Default direction: eventually make `client` run full graphics runtime by default (similar to `bz3`).
2. Runtime packaging: do **not** install demo runtime binaries with SDK artifacts.
3. Subsumption ownership: this track decides which existing tests/scripts are first migration candidates.
4. Green criteria: Linux-host passing path is sufficient proof-of-concept to mark this project green; cross-platform is future follow-on work.

## Clarification: `--net-smoke`
- Current state: there is no implemented `--net-smoke` flag in `m-karma` runtime code today.
- It previously appeared only as an example placeholder in prior planning notes.
- Enhancement direction: if we introduce it, define it as an additive explicit test mode under a broader `--test-*` family.

## Initial Subsumption Plan (Decision for #3)
First candidates (lowest risk, high overlap):
1. Retire duplicate manual command packets that invoke smoke/SDK gates outside the primary matrix wrapper.
2. Keep underlying deterministic tests (`client_transport_contract_test`, `server_transport_contract_test`, `demo_protocol_contract_test`) active.
3. Only migrate script logic into `client/server --test-*` modes after equivalence is proven by trace and exit-code behavior.

## Milestones

### ENH-S0: Baseline Extraction
- Create this project doc.
- Consolidate prior baseline planning notes into this track.
- Acceptance:
  - `demo-enhance.md` active with assignment row,
  - baseline context captured here for follow-on slices.

### ENH-S1: Test-Mode CLI Contract
- Define additive runtime test-mode contract (`--test-*` family) and land first mode behind explicit opt-in.
- Acceptance:
  - first mode implemented and documented,
  - default runtime behavior unchanged.

### ENH-S2: Script/Mode Convergence Slice
- Migrate one bounded existing smoke scenario from ad-hoc script behavior into explicit in-runtime test mode.
- Acceptance:
  - equivalence evidence captured,
  - no coverage regression.

### ENH-S3: Default Runtime Direction
- Transition toward full graphics runtime default for `client` while preserving explicit test-mode options.
- Acceptance:
  - default startup behavior matches direction lock,
  - deterministic test mode remains opt-in.

### ENH-S4: Closeout Gate
- Demonstrate Linux proof-of-concept green with matrix + telemetry trend policy operating cleanly.
- Acceptance:
  - telemetry trend says `monitor` over agreed observation window,
  - project marked green.

## Owned Paths
- `m-overseer/agent/projects/demo-enhance.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`
- `m-karma/src/demo/client/*` (as enhancement slices require)
- `m-karma/src/demo/server/*` (as enhancement slices require)
- `m-karma/scripts/test-demo-*.sh` (as enhancement slices require)
- `m-karma/.github/workflows/core-test-suite.yml` (only if gate wiring changes)

## Interface Boundaries
- Inputs consumed:
  - existing `m-karma` demo runtime, telemetry scripts, and CI workflow.
- Outputs/contracts exposed:
  - runtime test-mode contract and reproducible validation packet.
- Coordinate before changing:
  - `m-karma/cmake/sdk/apps.cmake`
  - `m-karma/cmake/50_sdk_install_export.cmake`
  - `m-karma/.github/workflows/core-test-suite.yml`

## Non-Goals
- Do not import `m-bz3` runtime/domain/protocol code.
- Do not install demo runtime binaries with SDK artifacts.
- Do not weaken deterministic transport/protocol coverage.
- Do not require non-Linux validation for proof-of-concept green in this track.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh

cd ../m-karma
export ABUILD_AGENT_NAME=demo-enhance
./abuild.py --claim-lock -d <build-dir>
./abuild.py -c -d <build-dir>
./scripts/test-demo-trace-matrix.sh <build-dir> <sdk-prefix>
./scripts/test-demo-telemetry-summary.sh ci-logs <build-dir>/demo-trace-matrix-artifacts
./scripts/test-demo-telemetry-trend.sh ci-logs 10
./abuild.py --release-lock -d <build-dir>
```

## Trace Channels
- `net.client`
- `net.server`
- `engine.app`
- `engine.server`
- `config`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d build-demo

./build-demo/server --data ./data --listen-port 11899 --trace net.server,engine.server,config
./build-demo/client --data ./data --server 127.0.0.1:11899 --trace net.client,engine.app,config
```

## First Session Checklist
1. Read this enhancement doc and current demo runtime entrypoints/scripts listed in Foundation References.
2. Confirm test-mode naming/scope for `ENH-S1`.
3. Implement one bounded additive mode.
4. Run validation packet.
5. Update status + risks.

## Current Status
- `2026-02-22`: enhancement track created from closed baseline; outstanding work extracted into this active plan.
- `2026-02-22`: policy decisions recorded: full-runtime default direction, no SDK install for runtime binaries, Linux PoC green criteria.
- `2026-02-22`: initial subsumption candidate plan recorded (duplicate packet retirement first; deterministic leaf tests retained).

## Open Questions
- What should the first concrete mode name be: `--test-net-smoke`, `--test-runtime-smoke`, or `--test-scenario=<name>`?
- Should test modes be client-only first or client/server symmetric from the start?
- What is the minimum observation window length for ENH-S4 closeout (for example 10 vs 20 Linux required runs)?

## Handoff Checklist
- [x] Useful/unfinished baseline work extracted into this active project.
- [x] Operator policy decisions captured.
- [x] Initial subsumption candidates selected.
- [ ] First additive `--test-*` mode implemented.
- [ ] Linux telemetry observation window completed and closeout decision recorded.
