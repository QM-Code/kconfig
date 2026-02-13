# Gameplay Netcode

## Project Snapshot
- Current owner: `unassigned`
- Status: `in progress`
- Immediate next task: implement Phase 2 predicted-shot path (`local_shot_id` mapping + reconciliation) after existing immediate-local-feedback behavior.
- Validation gate: `./scripts/test-server-net.sh <build-dir>` plus local multiplayer smoke with trace channels.

## Mission
Preserve responsive local gameplay feel under latency while keeping server authority for world truth.

## Product Goals
1. Local player actions feel immediate.
2. Server remains authoritative for shots/hits/rules.
3. Prediction + reconciliation hide normal network delay.
4. Local SFX/VFX for local actions do not wait for round trip.

## Owned Paths
- `m-rewrite/src/game/game.cpp`
- `m-rewrite/src/game/client/net/*`
- `m-rewrite/src/game/server/net/*`
- `m-rewrite/src/game/net/*`
- `m-rewrite/src/game/protos/messages.proto`

## Canonical Fire Flow
1. Local input edge detected.
2. Client immediately plays local shot feedback.
3. Client sends `create_shot` intent to server.
4. Server validates and broadcasts authoritative shot event.
5. Shooter suppresses self-echo SFX and reconciles predicted shot to authoritative state.
6. Remote clients render authoritative shot feedback.

## Current State
Implemented:
- immediate local fire SFX path,
- server `source_client_id` tagging for echo suppression,
- client self-echo suppression.

Not implemented:
- full predicted local shot simulation path,
- robust reconciliation and smoothing,
- lag-compensated hit validation,
- deterministic net-sim regression suite for gameplay feel.

## Execution Plan
1. Add `local_shot_id` handshake path for deterministic mapping.
2. Add predicted local shot lifecycle and reconciliation.
3. Add correction/smoothing telemetry in leaf trace channels.
4. Add deterministic latency/jitter/loss tests with thresholds.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-server-net.sh <build-dir>
```

Interactive smoke:

```bash
./<build-dir>/bz3-server -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -w common
./<build-dir>/bz3 -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -n tester -t input.events,net.client
```

## Trace Channels
- `net.client`
- `net.server`
- `input.events`

## Handoff Checklist
- [ ] Local responsiveness preserved.
- [ ] Server authority unchanged.
- [ ] Prediction/reconciliation behavior documented.
- [ ] Server/net validation and smoke results recorded.
