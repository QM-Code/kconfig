# Multi-Stage Handshake Integration (`m-bz3` + `m-karma`)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (plan drafted; implementation not started)`
- Immediate next task: lock Stage-0 contract (state names, transition rules, timeout policy) and land non-behavioral enums/structs in client/server code.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`

## Project Overview
This project is about turning a set of existing join/bootstrap parts into one coherent handshake pipeline.

We already have the hard pieces:
- connect,
- auth/pre-auth,
- join admit/reject,
- init + world package transfer,
- snapshot and gameplay events.

The gap is orchestration. The current behavior is spread across multiple flags and local assumptions. The goal here is to make the handshake explicit, staged, and auditable, without reintroducing `m-dev`'s old two-step app-level join choreography.

## Foundation References
- `m-bz3/src/server/net/transport_event_source/*`
- `m-bz3/src/server/runtime/event_loop.cpp`
- `m-bz3/src/server/net/event_source.hpp`
- `m-bz3/src/client/net/connection/*`
- `m-bz3/src/net/protocol_codec/*`
- `m-karma/src/network/server/session/*`
- `m-dev/src/game/server/game.cpp` (behavior reference only; not architecture blueprint)
- `m-dev/src/game/client/server/server_connector.cpp` (behavior reference only)

## Why This Is Separate
Handshake integration cuts across protocol, server runtime, client runtime, and content transfer. That boundary crossing makes it high-risk if done opportunistically while working on unrelated gameplay features.

Keeping this as a dedicated project creates one place to:
- define stage semantics once,
- keep rollout slices bounded,
- prove no regressions with contract/integration tests.

## North Star (One Logical Flow)
Target server/client handshake stages:
1. `Connected`
2. `JoinRequested`
3. `JoinAdmitted`
4. `BootstrapStreaming`
5. `AwaitClientReady`
6. `Active`
7. `Terminated`

Design rule: gameplay-bearing events (`request_spawn`, `player_location`, `create_shot`) are legal only in `Active`.

## Stage Contract (authoritative behavior)
| Stage | Entry trigger | Exit trigger | Timeout/failure result |
|---|---|---|---|
| `Connected` | transport connect event | first valid join request packet | disconnect on malformed/invalid join payload |
| `JoinRequested` | decoded join request accepted for processing | join decision resolved | reject + disconnect on auth/admission failure |
| `JoinAdmitted` | join accepted | begin bootstrap emission | reject + disconnect if bootstrap emission cannot begin |
| `BootstrapStreaming` | join result accepted sent | init + transfer + snapshot finished server-side | disconnect on transfer/send/integrity failure |
| `AwaitClientReady` | bootstrap stream finished | explicit client ready received | disconnect on ready timeout |
| `Active` | ready received and validated | leave/disconnect/terminal error | normal leave path or explicit disconnect |
| `Terminated` | any terminal path | n/a | terminal |

Notes:
- `m-dev` parity strings remain valid reference for operator-facing trace and reject reasons, but stage contract is the source of truth.
- No reintroduction of app-level `JoinRequest` then `PlayerJoin` gating from `m-dev`.

## Plan of Execution
### P0: Freeze the contract before code churn
Ship a small internal contract doc + enums/structs naming the stages and legal transitions. No behavior change yet.

Outputs:
- shared stage enum names in server/client handshake code,
- transition table documented in code comments and project notes.

### P1: Server phase model
Replace ad-hoc per-peer `joined` booleans with explicit per-peer handshake phase tracking.

Key work:
- move peer state to `HandshakePhase`,
- guard illegal packets by current phase,
- keep existing join decision path (`ResolveServerSessionJoinDecision`) intact.

### P2: Client bootstrap phase model
Introduce matching client-side handshake phase tracking around:
- join response handling,
- init validation,
- world transfer begin/chunk/end handling,
- snapshot completion.

Key work:
- make bootstrap completion deterministic in one place,
- remove implicit "ready enough" assumptions spread across handlers.

### P3: Ready signal
Add explicit client-ready message once local bootstrap is complete.

Key work:
- protocol message addition (`ClientReady`),
- server transition from `AwaitClientReady` -> `Active`,
- strict replay/drop policy for duplicate/early ready packets.

### P4: Deadlines and failure semantics
Add stage-specific timeout policy with clear disconnect reasons.

Baseline deadlines:
- join request deadline after connect,
- bootstrap completion deadline after admit,
- client-ready deadline after snapshot/transfer completion.

### P5: Trace-first observability
Add stage transition traces and timeout/reject traces so failures are diagnosable in one run.

Required trace evidence:
- `peer=<id> phase old->new cause=...`,
- reject/timeout reason and stage name,
- bootstrap timing summary.

### P6: Contract and integration tests
Expand automated coverage to include the stage machine, not just packet-level behavior.

Must-cover cases:
- happy path to `Active`,
- reject before admit,
- transfer failure,
- ready timeout,
- illegal gameplay message before `Active`,
- reconnect and stale peer cleanup.

### P7: Rollout and cleanup
Remove legacy handshake assumptions that became redundant after stage model adoption.

Guardrail: cleanup only after tests and traces prove parity/stability.

## Owned Paths
- `m-bz3/src/server/net/transport_event_source/*`
- `m-bz3/src/server/runtime/event_loop.cpp`
- `m-bz3/src/server/net/event_source.hpp`
- `m-bz3/src/client/net/connection/*`
- `m-bz3/src/net/protocol_codec/*`
- `m-bz3/src/tests/*handshake*` (new or renamed tests)
- `m-karma/src/network/server/session/*` (only where handshake contracts are shared)

## Interface Boundaries
- Inputs consumed:
  - client transport events and decoded client packets,
  - pre-auth decision surface from `m-karma`,
  - world content sync decisions.
- Outputs exposed:
  - deterministic `JoinResult/Init/Transfer/Snapshot` sequencing,
  - explicit client-ready gate to gameplay-active state,
  - stage-aware trace and disconnect reasons.
- Coordination-sensitive files:
  - server/client protocol codec headers,
  - server event source interfaces,
  - client connection lifecycle handlers.

## Non-Goals
- Rebuilding handshake to mirror `m-dev` two-step application-level join.
- Shipping config/i18n expansion as part of handshake integration.
- Broad gameplay refactors unrelated to handshake staging.
- Rewriting content transfer subsystem internals.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

```bash
cd m-bz3
./abuild.py -a hs-overseer -d build-handshake --claim-lock
./abuild.py -a hs-overseer -d build-handshake --configure --karma-sdk ../m-karma/out/karma-sdk
cmake --build build-handshake --target server_net_contract_test transport_loopback_integration_test transport_disconnect_lifecycle_integration_test -- -j4
ctest --test-dir build-handshake -R "server_net_contract_test|transport_loopback_integration_test|transport_disconnect_lifecycle_integration_test" --output-on-failure
./abuild.py -a hs-overseer -d build-handshake --release-lock
```

## Trace Channels
- `net`
- `engine`
- `net.server`
- `net.client`
- `engine.server`
- `engine.client`

## Build/Run Commands
```bash
cd m-bz3
./abuild.py -a hs-overseer -d build-handshake --configure --karma-sdk ../m-karma/out/karma-sdk
./build-handshake/bz3-server --trace net,engine,net.server
```

```bash
cd m-bz3
timeout 20s ./build-handshake/bz3 --user-config demo/users/default/config.json --trace net,engine,net.client
```

## First Session Checklist
1. Read the listed handshake-related files before editing.
2. Land Stage-0 types/constants and transition table with no behavior change.
3. Implement one vertical slice: server phase gating up to `BootstrapStreaming`.
4. Run required tests and collect trace evidence.
5. Update this project doc with what was actually landed and what remains.

## Current Status
- `2026-02-25`: Planning baseline drafted. Architecture direction settled: one logical multi-stage flow, stage-gated gameplay entry, no `m-dev` two-step reintroduction.

## Open Questions
- Should client-ready be a dedicated protocol message or a repurposed existing message type?
- Do we want strict disconnect on any pre-`Active` gameplay packet, or soft-ignore with trace (for transition period)?
- Which stage timeout values should be configurable versus hardcoded operational defaults?

## Handoff Checklist
- [ ] Stage contract constants/types landed.
- [ ] Server per-peer phase model landed.
- [ ] Client bootstrap phase model landed.
- [ ] Explicit client-ready gate landed.
- [ ] Timeout and reject semantics landed.
- [ ] Contract/integration tests updated and passing.
- [ ] Trace evidence captured for happy and failure paths.
