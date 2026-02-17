# Server Network

## Project Snapshot
- Current owner: `unassigned`
- Status: `in progress`
- Immediate next task: expand runtime-event harness coverage for protocol edge cases while preserving existing event-source/runtime contracts.
- Validation gate: `./scripts/test-server-net.sh`

This is the canonical project file for server network behavior and server test harness coverage.

## Newcomer Read Order
1. Read this top section for assignment context.
2. Read `## Mission` in source material.
3. Read `## Current Server Test Suite`.
4. Read `## Agent Workflow Guardrails`.
5. Add one test or one small runtime contract improvement, then rerun wrapper.

## Consolidation
This file supersedes:
- `docs/projects/server_network_track.md`
- `docs/projects/server-testing-playbook.md`

## Quick Start
1. Read `AGENTS.md`.
2. Read `docs/AGENTS.md`.
3. Use this file as the only project-level source for server network work.

## Note
The source-material sections below are preserved verbatim for no-loss migration context and may mention retired `*_track`/`*playbook` filenames.

---

## Source Material: server_network_track.md
# Server Network Track

## Mission
Own server-side networking/event-source/runtime event handling and protocol-consistent behavior for join/spawn/shot/leave flows.

## Primary Specs
- `docs/projects/server-network.md`
- `docs/projects/core-engine-infrastructure.md` (backend contract + fixed-step context)

## Why This Is Separate
Server network flow can evolve with minimal overlap with physics/audio/UI internals if protocol boundaries remain stable.

## Owned Paths
- `m-rewrite/src/game/server/*`
- `m-rewrite/src/game/server/net/*`
- `m-rewrite/src/game/net/*` (server-related paths)
- `m-rewrite/src/game/tests/enet_*`
- `m-rewrite/src/game/tests/server_*`

## Interface Boundaries
- Inputs: client wire protocol messages.
- Outputs: runtime events + server->client messages.
- Coordinate before changing:
  - `m-rewrite/src/game/protos/messages.proto`
  - `m-rewrite/src/game/net/protocol.hpp`
  - `m-rewrite/src/game/net/protocol_codec.cpp`
  - `m-rewrite/src/game/CMakeLists.txt`

## Non-Goals
- Renderer/UI implementation work.
- Physics backend internals.
- Audio backend internals.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-server-net.sh
```

## Trace Channels
- `engine.server`
- `net.server`
- `net.client`

## Build/Run Commands
```bash
./abuild.py -a
./scripts/test-server-net.sh
```

## First Session Checklist
1. Read `docs/projects/server-network.md`.
2. Confirm protocol/event invariants before edits.
3. Implement minimal change.
4. Run `./scripts/test-server-net.sh`.
5. Update this project file if semantics changed.

## Current Status
- See `Project Snapshot` at top of file for active owner and status.

## Open Questions
- See `Project Snapshot` and `Newcomer Read Order` at top of file.

## Handoff Checklist
- [ ] Server net tests green.
- [ ] Protocol changes coordinated/documented.
- [ ] Trace quality maintained.
- [ ] Runtime semantics explained in handoff notes.


---

## Source Material: server-testing-playbook.md

# Server Testing Playbook (m-rewrite)

## Project Track Linkage
- Delegation track: `docs/projects/server-network.md`
- Governance track: `docs/projects/testing-ci-docs.md`

## Purpose
This document is the handoff point for server/network test coverage in the rewrite phase.

Use this first when touching:
- `m-rewrite/src/game/server/*`
- `m-rewrite/src/game/server/net/*`
- `m-rewrite/src/game/net/*` (server protocol paths)

## Current Server Test Suite

1. `server_net_contract_test`
- File: `m-rewrite/src/game/tests/server_net_contract_test.cpp`
- Scope:
  - protocol codec roundtrips for join/init/create-shot server/client messages
  - scripted server event-source parsing and validation (`join`, `leave`, `request_spawn`, `create_shot`)
- Expected in normal environments: `PASS`

2. `server_runtime_event_rules_test`
- File: `m-rewrite/src/game/tests/server_runtime_event_rules_test.cpp`
- Scope:
  - runtime event rule invariants without ENet dependency
  - unknown `leave/spawn/create_shot` are ignored
  - known `leave` emits death once, and leave-failure emits no death
  - known `create_shot` increments global shot IDs monotonically
- Expected in normal environments: `PASS`

3. `enet_environment_probe_test`
- File: `m-rewrite/src/game/tests/enet_environment_probe_test.cpp`
- Scope:
  - raw ENet runtime availability (init + host creation + loopback connect)
- Expected:
  - `PASS` when local loopback sockets are allowed
  - `SKIP` in restricted/sandboxed environments
- CTest behavior:
  - skip return code is `77`
  - configured via `SKIP_RETURN_CODE 77`

4. `enet_loopback_integration_test`
- File: `m-rewrite/src/game/tests/enet_loopback_integration_test.cpp`
- Scope:
  - real ENet client/server loopback through `CreateEnetServerEventSource()`
  - join handshake acceptance path
  - join rejection path (protocol mismatch): accepts either explicit reject response or immediate disconnect, but never allows a join event
  - spawn/create-shot event propagation into server input events
- Expected:
  - `PASS` when ENet loopback is available
  - `SKIP` when environment probe conditions are unavailable
- CTest behavior:
  - skip return code is `77`
  - configured via `SKIP_RETURN_CODE 77`

5. `enet_multiclient_loopback_test`
- File: `m-rewrite/src/game/tests/enet_multiclient_loopback_test.cpp`
- Scope:
  - real ENet client/server loopback with two simultaneous clients
  - accepted join handshake for both clients
  - broadcast fan-out validation for `onPlayerSpawn`, `onCreateShot`, and `onPlayerDeath`
  - event payload checks for shot source client id and global shot id
- Expected:
  - `PASS` when ENet loopback is available
  - `SKIP` when environment probe conditions are unavailable
- CTest behavior:
  - skip return code is `77`
  - configured via `SKIP_RETURN_CODE 77`

6. `enet_disconnect_lifecycle_integration_test`
- File: `m-rewrite/src/game/tests/enet_disconnect_lifecycle_integration_test.cpp`
- Scope:
  - disconnect-after-join emits exactly one `ClientLeave`
  - explicit `PlayerLeave` followed by disconnect does not emit duplicate `ClientLeave`
  - reconnect path does not reuse stale client ids
  - post-reconnect `create_shot` is attributed to current connection id (not payload/stale id)
- Expected:
  - `PASS` when ENet loopback is available
  - `SKIP` when environment probe conditions are unavailable
- CTest behavior:
  - skip return code is `77`
  - configured via `SKIP_RETURN_CODE 77`

## How To Run

From `m-rewrite/`:

```bash
./scripts/test-server-net.sh

# Equivalent explicit commands:
cmake --build build-dev --target \
  server_net_contract_test \
  server_runtime_event_rules_test \
  enet_environment_probe_test \
  enet_loopback_integration_test \
  enet_multiclient_loopback_test \
  enet_disconnect_lifecycle_integration_test

ctest --test-dir build-dev -R "server_net_contract_test|server_runtime_event_rules_test|enet_environment_probe_test|enet_loopback_integration_test|enet_multiclient_loopback_test|enet_disconnect_lifecycle_integration_test" --output-on-failure
```

CI note:
- `.github/workflows/core-test-suite.yml` runs this same wrapper (`./scripts/test-server-net.sh`) on pull requests and main/master pushes.

## Result Interpretation Rules

1. `enet_environment_probe_test=PASS` and any ENet integration test fails (`enet_loopback_integration_test`, `enet_multiclient_loopback_test`, or `enet_disconnect_lifecycle_integration_test`)
- Treat as real regression in ENet integration behavior.

2. `enet_environment_probe_test=SKIP`
- Environment is network-restricted.
- ENet integration test results are non-actionable for regressions.

3. `server_net_contract_test=FAIL` or `server_runtime_event_rules_test=FAIL`
- Treat as actionable regression regardless of network environment.
- These tests do not rely on real socket availability.

## Agent Workflow Guardrails

1. If ENet implementation is in active flux by another agent:
- Prefer adding/changing tests in `m-rewrite/src/game/tests/*`.
- Avoid mixing behavior changes and test changes in one patch unless necessary.

2. For server runtime rule changes:
- Update/add assertions in `server_runtime_event_rules_test` first (and `server_net_contract_test` when protocol/event-source shape changes).
- Only then adjust runtime code.

3. Keep tests deterministic:
- Avoid hard-coded public ports where possible.
- Prefer short timeouts and skip semantics for environment constraints.

## Next Useful Test Additions

1. ENet join-policy integration:
- duplicate player-name handling policy (when enforced) and expected reject/disconnect behavior
- protocol-version mismatch policy across future protocol revisions

2. ENet transport edge-paths:
- delayed packet + disconnect ordering (ensure no phantom post-disconnect actions)
- duplicate/replayed reliable client messages and expected de-duplication policy
