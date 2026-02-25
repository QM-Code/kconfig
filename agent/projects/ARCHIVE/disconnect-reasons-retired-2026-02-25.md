# Disconnect Reasons Migration (`m-dev` -> `m-karma`/`m-bz3`)

## Project Snapshot
- Current owner: `overseer`
- Status: `retired/archived (closeout captured)`
- Immediate next task: none (archived record only).
- Validation gate: project-doc lint + relevant runtime tests if this archive is ever reactivated.

## Mission (Primary Focus)
Bring over disconnect/reject/kick reason behavior from `m-dev` while keeping developer-facing reason text hardcoded in runtime code.

Primary outcomes:
- parity with `m-dev` reason semantics where still relevant,
- intentionally no new config/i18n surface for this track,
- `m-karma` and `m-bz3` kept in sync for core semantics.

## Scope
1. `m-dev` source inventory (authoritative reference behavior)
2. `m-karma` server/client reason paths
3. `m-bz3` server/client reason paths
4. bounded cleanup + validation/tests for behavior parity

## Source Baseline (`m-dev`)
Known reason behavior to port or reconcile:
- server join rejection reasons (`Protocol version mismatch.`, `Name already in use.`, `Join request required.`, `Join request mismatch.`)
- admin/plugin kick/disconnect reason flow (`disconnect_player` / `kick_player` with reason)
- client-side mapping from reason text to user-facing status/dialog messages
- explicit transport disconnect actions following reason emission

Reference files:
- `m-dev/src/game/server/game.cpp`
- `m-dev/src/game/net/backends/enet/server_backend.cpp`
- `m-dev/src/game/server/plugin.cpp`
- `m-dev/src/game/client/main.cpp`
- `m-dev/src/game/client/server/server_connector.cpp`
- `m-dev/src/game/client/server/community_browser_controller.cpp`
- `m-dev/src/game/client/world_session.cpp`
- `m-dev/src/game/net/backends/enet/client_backend.cpp`

## Inventory Pass (`m-dev`, 2026-02-25)
Decision note: as of `2026-02-25`, this project explicitly keeps developer-facing reasons hardcoded and defers config/i18n migration.

### Server-side reason semantics
| Source literal/behavior | Source callsite(s) | Keep/rename/drop | Hardcoded strategy |
|---|---|---|---|
| `"Protocol version mismatch."` emitted on join reject and on post-join protocol guard | `m-dev/src/game/server/game.cpp` (`JoinRequest` and `PlayerJoin` branches) | keep | keep literal hardcoded |
| `"Name already in use."` emitted on duplicate name at join-request stage and join-finalization stage | `m-dev/src/game/server/game.cpp` | keep | keep literal hardcoded |
| `"Join request required."` emitted when `PlayerJoin` arrives before approved `JoinRequest` | `m-dev/src/game/server/game.cpp` | keep | keep literal hardcoded |
| `"Join request mismatch."` emitted when approved join state does not match incoming `PlayerJoin` | `m-dev/src/game/server/game.cpp` | keep | keep literal hardcoded |
| `disconnect_player` / `kick_player` pass-through reason argument to transport disconnect | `m-dev/src/game/server/plugin.cpp` | keep | keep dynamic pass-through; optional hardcoded default only if empty reason policy is desired |
| Non-empty disconnect reason is sent as a direct server chat notice before transport disconnect | `m-dev/src/game/net/backends/enet/server_backend.cpp` | keep behavior | (behavioral contract; not a text key by itself) |

### Client-side reason semantics
| Source literal/behavior | Source callsite(s) | Keep/rename/drop | Hardcoded strategy |
|---|---|---|---|
| Join response fallback for empty server reason: `"Join rejected by server."` | `m-dev/src/game/client/server/server_connector.cpp` | keep | keep literal hardcoded |
| Transport/manual disconnect default: `"Disconnected from server."` | `m-dev/src/game/net/backends/enet/client_backend.cpp`, `m-dev/src/game/client/main.cpp` | keep | keep literal hardcoded |
| Transport timeout reason: `"Connection lost (timeout)."` | `m-dev/src/game/net/backends/enet/client_backend.cpp` | keep | keep literal hardcoded |
| UI message mapping for `Name already in use` | `m-dev/src/game/client/main.cpp` | keep | keep mapping literals hardcoded |
| UI message mapping for `Protocol version mismatch` | `m-dev/src/game/client/main.cpp`, `m-dev/src/game/client/world_session.cpp` | keep | keep mapping literals hardcoded |
| UI message mapping for `Join request required` + `Join request mismatch` to retry prompt | `m-dev/src/game/client/main.cpp` | keep | keep shared retry literal hardcoded |
| UI message mapping for connection-loss substring | `m-dev/src/game/client/main.cpp` | keep | keep literal hardcoded |
| UI message mapping for timeout substring | `m-dev/src/game/client/main.cpp` | keep | keep literal hardcoded |
| UI message mapping for server-closed substring | `m-dev/src/game/client/main.cpp` | keep | keep literal hardcoded |
| Generic non-empty reason formatting (`"Disconnected: " + reason`) | `m-dev/src/game/client/main.cpp` | keep | keep format literal hardcoded |
| Browser status fallback (`"Disconnected from server. Select a server to reconnect."`) | `m-dev/src/game/client/server/community_browser_controller.cpp` | keep | keep literal hardcoded |

## Current Gap Check (`m-karma` / `m-bz3`, 2026-02-25)
- Hardcoded runtime reason literals are present in active paths and are now accepted for this project:
  - `m-bz3/src/server/net/transport_event_source/receive.cpp`
  - `m-bz3/src/server/runtime/server_game.cpp`
  - `m-bz3/src/client/net/connection/inbound/bootstrap.cpp`
  - `m-karma/src/demo/net/protocol.cpp`
- Numeric disconnect-code policy is already JSON-backed in both branches:
  - `server.network.disconnectCodes.*`
  - `client.network.disconnectCodes.*`

## Hardcoding Direction
- For this project, developer-facing reason strings remain hardcoded.
- No new locale/config reason key expansion is planned.
- Existing JSON-backed numeric disconnect codes remain in place and unchanged.

## Implementation Rules
- hardcoded developer-facing reason strings are allowed for this track,
- avoid proliferating duplicate phrasing for the same condition where simple reuse is possible,
- transport/protocol numeric disconnect codes stay stable and shared (JSON-backed in distro defaults, validated at startup),
- no i18n migration work in this project unless explicitly re-opened.

## Workflow
1. Audit one runtime path (server first, then client) and list literals + fallback behavior.
2. Identify only essential parity/clarity fixes to hardcoded literals.
3. Implement minimal edits in `m-karma` / `m-bz3`.
4. Update/extend tests only where behavior changes.

## Hardcoded Reason Matrix (Batch A)
| Runtime literal | Intended usage | Notes |
|---|---|---|
| `"Protocol version mismatch."` | protocol mismatch reject/disconnect | keep |
| `"Name already in use."` | duplicate name reject/disconnect | keep |
| `"Join request required."` | join-flow order violation | keep |
| `"Join request mismatch."` | join-flow identity mismatch | keep |
| `"Join rejected by server."` | generic join reject fallback | keep |
| `"Disconnected from server."` | generic disconnected fallback | keep |
| `"Connection lost (timeout)."` | timeout disconnect reason payload | keep |
| `"Connection timed out. Please try again."` | user-facing timeout guidance | keep |
| `"Connection closed by server."` | user-facing closed guidance | keep |
| `"Disconnected: {reason}"` (format) | generic non-empty reason formatting | keep |
| `"Disconnected by server administrator."` | optional default for empty admin/plugin kick reason | optional |

Rules:
- no index column,
- keep notes short.

## Immediate Backlog
1. Confirm whether to perform any wording harmonization at all, or freeze current hardcoded strings as-is.
2. If harmonization is desired, apply minimal edits only in:
   - `m-bz3/src/server/net/transport_event_source/receive.cpp`
   - `m-bz3/src/server/runtime/server_game.cpp`
   - `m-bz3/src/client/net/connection/inbound/bootstrap.cpp`
   - `m-karma/src/demo/net/protocol.cpp`
3. Add/update tests only for changed behavior (if any).
4. Keep existing numeric disconnect-code coverage intact:
   - join rejection reason payloads,
   - disconnect/kick reason propagation,
   - protocol mismatch handling.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

```bash
cd m-karma
cmake --build build-test --target karma_demo_server demo_protocol_contract_test -- -j4
ctest --test-dir build-test -R demo_protocol_contract_test --output-on-failure
```

```bash
cd m-bz3
cmake --build build-test --target server_net_contract_test -- -j4
ctest --test-dir build-test -R server_net_contract_test --output-on-failure
```

## Handoff Checklist
- [x] `m-dev` reason inventory captured and triaged.
- [x] Hardcoding direction confirmed by operator for this project.
- [x] Optional harmonization edits applied (only if requested).
- [ ] Contract tests updated for changed behavior (if any).
