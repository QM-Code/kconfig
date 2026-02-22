# Karma Demo Runtime (Client/Server)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (design lock + planning baseline)`
- Immediate next task: execute `DEMO-S1` by adding `m-karma` build targets that emit `client` and `server` binaries in the build directory while keeping all demo/runtime code outside exported SDK libraries.
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-karma`: `./abuild.py -c -d <build-dir>` and demo smoke command packet defined in this doc

## Mission
Create first-class `m-karma` demo runtime binaries, `client` and `server`, that:
- use only `m-karma` internals,
- demonstrate real client/server interaction end-to-end (bind, connect, pre-auth/join handshake, minimal command exchange, disconnect),
- read and layer config/data via the existing bootstrap path (`data/client/config.json`, `data/server/config.json`, `--data-dir`, `--user-config`, `KARMA_DATA_DIR`),
- become the primary SDK-user confidence path while preserving necessary low-level deterministic tests.

## Foundation References
- `m-karma/src/app/client/bootstrap.cpp`
- `m-karma/src/app/server/bootstrap.cpp`
- `m-karma/src/app/shared/bootstrap.cpp`
- `m-karma/src/common/data/directory_override.cpp`
- `m-karma/src/cli/client/app_options.cpp`
- `m-karma/src/cli/server/app_options.cpp`
- `m-karma/include/karma/network/transport/client.hpp`
- `m-karma/include/karma/network/transport/server.hpp`
- `m-karma/include/karma/network/server/auth/preauth.hpp`
- `m-karma/include/karma/network/server/session/hooks.hpp`
- `m-karma/include/karma/network/server/session/join_runtime.hpp`
- `m-karma/include/karma/network/server/session/leave_runtime.hpp`
- `m-karma/src/network/tests/contracts/client_transport_contract_test.cpp`
- `m-karma/src/network/tests/contracts/server_transport_contract_test.cpp`
- `m-bz3/src/client/main.cpp`
- `m-bz3/src/server/main.cpp`

## Why This Is Separate
This work is cross-cutting and intentionally product-facing:
- it defines how SDK users validate real runtime integration quickly,
- it affects build target policy, runtime packaging confidence, and test strategy,
- it creates a reference runtime surface likely to grow into a dedicated `src/demo/*` tree,
- it should not be mixed into unrelated gameplay/UI/cleanup slices.

## Direction Lock
1. `m-karma` must emit `client` and `server` binaries in normal build outputs, analogous to `m-bz3` emitting `bz3` and `bz3-server`.
2. Demo runtime code remains private implementation code and must not bloat exported SDK library APIs.
3. Demo binaries must use existing bootstrap/config/data layering behavior instead of custom loaders.
4. CLI behavior should track `m-karma` parser behavior by default; demo-only options must be additive and explicit.
5. End-to-end runtime traces become the primary integration confidence tool; low-level deterministic tests remain where they protect tight contracts.

## Comprehensive Requirements

### R1: Runtime Binary Outputs
- Add build targets that produce:
  - `<build-dir>/client`
  - `<build-dir>/server`
- These are `m-karma` binaries, not `m-bz3` wrappers.

### R2: Internal-Only Implementation
- Use only `m-karma` internals for runtime logic (transport/auth/session/config/bootstrap).
- No dependency on `m-bz3` runtime/domain/protocol code.

### R3: Data/Config Integration
- `server` uses bootstrap path requiring `server/config.json` and layered from `data/server/config.json`.
- `client` uses bootstrap path requiring `client/config.json` and layered from `data/client/config.json`.
- Respect `--data-dir`, `--user-config` (where applicable), and `KARMA_DATA_DIR`.
- Demonstrate canonical fixture usage under `demo/` for deterministic local runs.

### R4: End-to-End Interaction Coverage
- Required minimum interaction path:
  - server bind/listen,
  - client connect,
  - pre-auth + join decision,
  - minimal command exchange (for example ping/pong or equivalent authoritative roundtrip),
  - orderly disconnect/leave.

### R5: SDK Consumer Validation
- Add an in-tree consumer path that compiles/runs demo client/server using installed SDK package artifacts (`find_package(KarmaSDK CONFIG REQUIRED)`), so SDK contract health is testable in-tree.

### R6: Test Strategy Realignment
- Add process-level demo smoke gates as primary user confidence checks.
- Keep deterministic contract tests where they protect parser/codec/auth/config semantics.
- Retire overlapping tests only after equivalence is proven and documented.

## Proposed Source Layout
Target evolution path (locked as preferred structure):
- `m-karma/src/demo/client/*`
- `m-karma/src/demo/server/*`
- `m-karma/src/demo/net/*` (demo wire protocol and message adapters)
- `m-karma/src/demo/shared/*`

Initial pragmatic bootstrap may land as thin files before full decomposition, but end-state ownership should converge here.

## CMake/Build Plan

### Build Graph
- Add `m-karma/cmake/sdk/apps.cmake`.
- Include it from `m-karma/cmake/40_sdk_subdir.cmake`.
- Create executable targets (internal names allowed) with output names set to:
  - `client`
  - `server`
- Link against existing SDK libs (`karma_sdk_core`, `karma_sdk_client`) and existing internal dependencies.

### Anti-Bloat Constraints
- Do not add demo runtime sources into `karma_sdk_core` or `karma_sdk_client` source lists.
- Do not add new public headers unless a genuinely reusable engine contract is discovered.
- Keep demo protocol/runtime headers private to demo target include paths.

## Runtime/CLI Behavior Plan

### Compatibility Goal
- Match `m-karma` client/server parser behavior for shared options by default:
  - common options (`--trace`, `--data-dir`, `--language`, `--strict-config`, `--timestamp-logging`, `--user-config` where supported),
  - server options (`--server-config`, `--listen-port`, `--community`, backend overrides),
  - client options (`--server`, `--username`, `--password`, backend overrides).

### Caveat
- Runtime semantics can only match what `m-karma` implements.
- `bz3`-specific gameplay semantics are explicitly out of scope for demo binaries.

### Demo-Only Options
- If needed, add additive demo options with explicit namespacing (for example `--demo-script`, `--net-smoke`) so base parser compatibility remains clear.

## Data/Demo Integration Plan
- Keep `data/` as default runtime contract root for required config files.
- Use `demo/` as canonical reproducible fixture space:
  - `demo/communities/*`
  - `demo/users/*`
  - `demo/worlds/*`
- Ensure docs and command examples show deterministic runs with either:
  - `--data-dir <repo>/data` + `HOME=<repo>/demo/users/<fixture>`,
  - or fixture overlays as needed for scenario tests.

## SDK Consumer Build/Test Plan
- Add an in-tree SDK consumer example folder (for example `m-karma/examples/demo-sdk-consumer/`).
- Consumer must use package-based integration:
  - `find_package(KarmaSDK CONFIG REQUIRED)`
  - link to `karma::core` and/or `karma::client`.
- Add script wrappers that:
  1. build/install SDK from `m-karma`,
  2. configure/build consumer demo binaries against installed SDK,
  3. run consumer server/client smoke interaction.

## Validation Strategy

### Required Gates (project)
```bash
# project-doc hygiene
cd m-overseer
./agent/scripts/lint-projects.sh

# build m-karma runtime targets + existing transport contracts
cd ../m-karma
export ABUILD_AGENT_NAME=demo-runtime
./abuild.py --claim-lock -d <build-dir>
./abuild.py -c -d <build-dir>
ctest --test-dir <build-dir> -R "client_transport_contract_test|server_transport_contract_test" --output-on-failure

# demo process smoke (new gate to add)
./scripts/test-demo-client-server-smoke.sh <build-dir>

# sdk consumer smoke (new gate to add)
./scripts/test-sdk-demo-consumer.sh <build-dir> <sdk-prefix>

./abuild.py --release-lock -d <build-dir>
```

### Test Rationalization Policy
- Keep:
  - low-level parser/config/auth/transport contract tests that are deterministic and narrow.
- Shift primary confidence to:
  - process-level client/server smoke scenarios with trace evidence.
- Retire overlap only after:
  - equivalent behavior coverage is proven by scenario scripts and documented in this project.

## Milestones

### DEMO-S0: Project Baseline (this slice)
- Create this project doc and assignment row.
- Lock direction and constraints.
- Acceptance:
  - docs committed; ownership and next task recorded.

### DEMO-S1: Runtime Target Bring-Up
- Add `client` and `server` build targets in `m-karma`.
- Use existing app runner/bootstrap paths.
- Acceptance:
  - both binaries build and print help.

### DEMO-S2: Minimal Interaction Loop
- Implement bind/connect/pre-auth+join/minimal command/disconnect flow.
- Acceptance:
  - deterministic local smoke run succeeds and is scriptable.

### DEMO-S3: Data/Demo Fixture Integration
- Align runtime defaults and docs with `data/` + `demo/` fixture policy.
- Acceptance:
  - one reproducible fixture-backed command packet documented and validated.

### DEMO-S4: SDK Consumer In-Tree Gate
- Add package-based consumer build/test for demo runtime path.
- Acceptance:
  - scripted consumer smoke passes against installed SDK payload.

### DEMO-S5: Test Strategy Convergence
- Add trace-first scenario harness and map overlap with existing integration tests.
- Acceptance:
  - documented keep/retire decision list with no contract coverage regression.

### DEMO-S6: Structural Consolidation
- Complete migration to `src/demo/{client,server,net,shared}` shape.
- Acceptance:
  - directory structure, ownership boundaries, and docs stabilized.

## Owned Paths
- `m-overseer/agent/projects/demo.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`
- `m-karma/cmake/sdk/apps.cmake` (new)
- `m-karma/src/demo/*` (new)
- `m-karma/scripts/test-demo-client-server-smoke.sh` (new)
- `m-karma/examples/demo-sdk-consumer/*` (new)
- `m-karma/scripts/test-sdk-demo-consumer.sh` (new)

## Interface Boundaries
- Inputs consumed:
  - existing `m-karma` CLI/app/bootstrap/data/config/network internals.
- Outputs exposed:
  - two runtime binaries (`client`, `server`) and reproducible smoke gates.
- Coordination required before changing:
  - `m-karma/cmake/40_sdk_subdir.cmake`
  - `m-karma/cmake/sdk/targets.cmake`
  - `m-karma/cmake/50_sdk_install_export.cmake`
  - `m-karma/scripts/test-server-net.sh`
  - `m-karma/.github/workflows/*` (as gates are integrated)

## Non-Goals
- Do not import `m-bz3` gameplay/runtime code into `m-karma`.
- Do not expand exported SDK API surface for demo convenience.
- Do not remove deterministic low-level tests without explicit replacement evidence.
- Do not treat this as a full game implementation track.

## Trace Channels
- `config`
- `engine.server`
- `engine.app`
- `net.server`
- `net.client`
- `engine.sim`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d build-demo

# server
./build-demo/server --data-dir ./data --listen-port 11899 --trace net.server,engine.server,config

# client
./build-demo/client --data-dir ./data --server 127.0.0.1:11899 --trace net.client,engine.app,config
```

## First Session Checklist
1. Add runtime CMake target wiring without touching exported SDK library source sets.
2. Add thin `main` entry points that use existing `karma::app::{client,server}::Run`.
3. Validate help output and basic startup config resolution for both binaries.
4. Land first process smoke script and make it deterministic.
5. Update docs/assignments with outcomes and blockers.

## Current Status
- `2026-02-22`: project created from operator direction to make `m-karma` ship first-class demo `client`/`server` runtime binaries.
- `2026-02-22`: direction locked to use existing bootstrap/data-layering pipeline (`data/client/config.json`, `data/server/config.json`) instead of ad-hoc loaders.
- `2026-02-22`: long-term structure preference recorded: `src/demo/{client,server,net,shared}` with in-tree SDK consumer validation.
- `2026-02-22`: test strategy intent recorded: process-level trace-rich runtime scenarios become primary confidence, with deterministic contract tests retained for narrow guarantees.

## Open Questions
- Should `client` default to full graphics runtime, `--net-smoke`, or auto-fallback based on environment?
- Should demo runtime binaries be installed with SDK artifacts or remain build-tree-only reference apps?
- Which current integration tests are first candidates for subsumption once demo scenario gates are stable?
- What minimum cross-platform smoke matrix is required before marking this project green?

## Handoff Checklist
- [ ] Runtime targets (`client`, `server`) build in `m-karma`.
- [ ] Minimal end-to-end interaction scenario passes locally.
- [ ] Data/config layering behavior verified through bootstrap path.
- [ ] SDK consumer in-tree smoke path implemented.
- [ ] Test overlap keep/retire decisions documented.
- [ ] CI gate plan updated with clear required-vs-optional classification.
