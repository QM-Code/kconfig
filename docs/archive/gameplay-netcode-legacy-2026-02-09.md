# Gameplay Netcode

## Project Snapshot
- Current owner: `unassigned`
- Status: `in progress`
- Immediate next task: implement Phase 2 predicted-shot path (`local_shot_id` mapping + reconciliation) after existing immediate-local-feedback behavior.
- Validation gate: `./scripts/test-server-net.sh` plus local multiplayer smoke with trace channels.

This is the canonical project file for gameplay responsiveness and netcode behavior.

## Newcomer Read Order
1. Read this top section for assignment context.
2. Read `## Product Goals`.
3. Read `## Current Status (2026-02-09)`.
4. Read `## Canonical Fire Flow`.
5. Execute from `## Implementation Plan`.

## Consolidation
This file supersedes:
- `docs/projects/netcode-responsiveness-playbook.md`
- `docs/projects/gameplay_netcode_track.md`

## Quick Start
1. Read `AGENTS.md`.
2. Read `docs/AGENTS.md`.
3. Use this file as the only project-level source for gameplay netcode work.

## Note
The source-material sections below are preserved verbatim for no-loss migration context and may mention retired `*_track`/`*playbook` filenames.

---

## Source Material: netcode-responsiveness-playbook.md
# Netcode Responsiveness Playbook (m-rewrite)

## Project Track Linkage
- Delegation track: `docs/projects/gameplay-netcode.md`

## Purpose
Define and enforce the client/server interaction model that keeps controls responsive under latency while preserving server authority.

This document is rewrite-phase guidance for `m-rewrite` while `m-dev` remains available as behavior reference.

## Product Goals
1. Local player actions feel immediate on input.
2. Server remains authoritative for world truth (shots, hits, score, death, rules).
3. Client prediction and reconciliation hide normal network delay without desync drift.
4. Audio and VFX feedback for local actions do not wait for server round trips.

## Boundaries
- Engine remains lifecycle owner.
- Game netcode lives under `m-rewrite/src/game/`.
- Backend internals remain hidden behind engine contracts.
- Do not move gameplay authority into engine-core subsystems.

## Current Status (2026-02-09)

## Implemented
1. Client sends fire/spawn intents to server (`request_player_spawn`, `create_shot`).
2. Server validates and broadcasts authoritative events.
3. Fire audio now plays immediately on local fire input in `src/game/game.cpp`.
4. Server `create_shot` now includes `source_client_id` in `src/game/protos/messages.proto`.
5. Client filters self-echo shot audio in `src/game/client/net/client_connection.cpp` to avoid double playback.

## Not Implemented Yet
1. Client-predicted local shot simulation/render path.
2. Reconciliation of predicted local shots to authoritative shot IDs/transforms.
3. Lag-compensated hit validation on server.
4. Jitter-buffer/interpolation for remote shot motion.
5. Automated latency/packet-loss scenario tests for gameplay feel thresholds.

## Canonical Fire Flow
1. Local input edge detected.
2. Client immediately plays local shot fire audio/VFX.
3. Client sends `create_shot` intent to server.
4. Server validates, allocates authoritative shot ID, and broadcasts `create_shot(source_client_id, global_shot_id, ...)`.
5. Shooter client suppresses self-echo SFX and reconciles predicted shot to authoritative state.
6. Remote clients render/play remote shot feedback from authoritative broadcast.

## Offline/Disconnected Behavior
1. Fire input should still play local fire sound (feedback path is local).
2. Network send is best-effort and skipped when not connected.
3. No authoritative world mutation is implied when disconnected.

## Implementation Plan

## Phase 1: Immediate Local Feedback (Done)
1. Local shot SFX on fire input edge.
2. Server echo de-dup via `source_client_id`.

## Phase 2: Predicted Shot Path (Next)
1. Add local predicted-shot container keyed by `local_shot_id`.
2. Spawn predicted shot immediately on fire.
3. On server `create_shot`, bind `local_shot_id -> global_shot_id` and reconcile transform/velocity.
4. Expire orphan predictions with bounded timeout if ack never arrives.

## Phase 3: Reconciliation + Smoothing
1. Add correction window and snap thresholds.
2. Interpolate remote shot transforms from server snapshots/events.
3. Keep correction telemetry in trace-only channels.

## Phase 4: Hit Validation and Lag Compensation
1. Define rewind or timestamp-based validation contract.
2. Keep final hit/death authority on server.
3. Add anti-cheat sanity checks around impossible shot data.

## Phase 5: Test and Instrumentation
1. Add deterministic net-sim tests (latency/jitter/loss).
2. Add trace counters for prediction error and correction frequency.
3. Gate regressions with pass/fail thresholds in CI.

## Trace Guidance
- Keep base channels concise (`net.client`, `net.server`).
- Put frame-level/high-rate diagnostics under leaf channels (for example `net.client.prediction`).
- Use `KARMA_TRACE` and `KARMA_TRACE_CHANGED` patterns.

## Immediate Next Tasks
1. Add `local_shot_id` echo to server `create_shot` payload for deterministic prediction matching.
2. Implement predicted local shot lifetime + reconciliation path in client game runtime.
3. Add a smoke test plan with local loopback plus injected latency/loss.

## Files to Start In
- `m-rewrite/src/game/game.cpp`
- `m-rewrite/src/game/client/net/client_connection.cpp`
- `m-rewrite/src/game/net/protocol_codec.hpp`
- `m-rewrite/src/game/net/protocol_codec.cpp`
- `m-rewrite/src/game/protos/messages.proto`
- `m-rewrite/src/game/server/net/enet_event_source.cpp`


---

## Source Material: gameplay_netcode_track.md

# Gameplay + Netcode Responsiveness Track

## Mission
Own gameplay-facing client/server interaction feel (responsiveness, prediction policy, reconciliation direction) while preserving engine boundaries.

## Primary Specs
- `docs/projects/gameplay-netcode.md`
- `docs/projects/server-network.md` (test guardrails for protocol/runtime changes)

## Why This Is Separate
This track is gameplay-semantics heavy and should be owned by one agent at a time to avoid conflicts.

## Owned Paths
- `m-rewrite/src/game/game.*`
- `m-rewrite/src/game/client/net/*`
- selected server runtime behavior under `m-rewrite/src/game/server/runtime*` (when assigned)

## Interface Boundaries
- Inputs: engine services (input/audio/physics/render), server protocol messages.
- Outputs: player-visible responsiveness and action/event timing.
- Coordinate before changing:
  - protocol files (`messages.proto`, `protocol*`)
  - server runtime event rules
  - `docs/projects/gameplay-netcode.md`

## Non-Goals
- Engine backend implementation internals (physics/audio/render backends).
- Content mount/archive internals.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-server-net.sh
```

And interactive client/server behavior validation for responsiveness scenarios.

## Trace Channels
- `input.events`
- `net.client`
- `net.server`

## Build/Run Commands
```bash
./bzbuild.py -a
./build-dev/bz3-server -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -w common
./build-dev/bz3 -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -n tester -t input.events,net.client
```

## First Session Checklist
1. Read `docs/projects/gameplay-netcode.md`.
2. Define exact responsiveness behavior being targeted.
3. Implement minimal client/server behavior slice.
4. Run server net tests + interactive validation.
5. Update this project file status and handoff notes.

## Current Status
- See `Project Snapshot` at top of file for active owner and status.

## Open Questions
- See `Project Snapshot` and `Newcomer Read Order` at top of file.

## Handoff Checklist
- [ ] Responsiveness behavior explicitly documented.
- [ ] Server/net tests still pass.
- [ ] Interactive validation notes captured.
