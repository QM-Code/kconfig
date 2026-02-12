# Content Mount

## Project Snapshot
- Current owner: `codex`
- Status: `in progress`
- Immediate next task: add stronger transfer-integrity controls for chunked world payloads (chunk hash chain or equivalent) with one targeted regression test.
- Validation gate: `./scripts/test-server-net.sh` plus manual client/server world-override smoke.

## Mission
Implement the engine-facing content mount abstraction so default shipped content is always available and world-specific packages can override it safely.

This project enables:
- default runtime data from configured data root,
- server-selected world override via `-w`,
- client application of received world package overlays.

## Locked Behavior Contract
1. No `-w`: default mode, use shipped/default content only.
2. `-w <world-dir>`: world override mode on top of defaults.
3. Server package behavior:
- server uses world files/config as overrides,
- server sends world package metadata/payload to client.
4. Client package behavior:
- client keeps local defaults,
- client applies received world package as higher-priority overlay.

## Owned Paths
- `m-rewrite/src/engine/common/data_path_resolver.*`
- `m-rewrite/include/karma/common/data_path_resolver.hpp`
- `m-rewrite/src/engine/common/world_archive.*`
- `m-rewrite/src/game/net/protocol_codec.*`
- `m-rewrite/src/game/server/net/enet_event_source.cpp`
- `m-rewrite/src/game/client/net/client_connection.*`

## Interface Boundaries
- Inputs: data-dir selection, server world selection, protocol package metadata.
- Outputs: deterministic content resolution order and safe package apply behavior.
- Coordinate before changing:
  - `m-rewrite/src/game/protos/messages.proto`
  - `m-rewrite/src/game/net/protocol.hpp`

## Current State (Implemented)
1. Data-root override pipeline exists (`--data-dir`, user config `DataDir`, env var).
2. World archive helpers exist (`BuildWorldArchive`, `ExtractWorldArchive`).
3. Server world config runtime layering from filesystem exists.
4. Mount precedence and mount-point matching behavior are implemented.
5. Network world package payload path is wired end-to-end.
6. Join/init path includes package hash metadata and client cache hinting.
7. World identity metadata (`world_id`, `world_revision`) exists with client-side identity validation.
8. Manifest summary and entry metadata are exchanged; cache-hit no-payload init and manifest reuse paths are implemented.
9. Runtime transfer supports `chunked_full` and manifest-driven `chunked_delta` with server-side mode selection and fallback to full payload.
10. Client world-package apply is staged, verified, and atomically promoted (full and delta), with rollback-safe behavior.
11. Client cache uses revisioned package paths plus retention pruning for stale revisions/packages.
12. Targeted failure-path coverage exists (`client_world_package_safety_integration_test`) and is included in `./scripts/test-server-net.sh`.
13. Chunk-transfer retry/resume semantics are implemented for world-package streaming:
- server retries and resumes from first unsent chunk with bounded attempts,
- client accepts compatible transfer restarts and idempotent duplicate chunks,
- interrupted transfer recovery is covered in integration test flow.

## Current Gaps
1. Transfer reliability
- stronger transfer-integrity controls beyond current sequencing/size checks are not yet implemented.
2. Compatibility contract
- client/server/world compatibility policy needs explicit hard-reject gating (current behavior is mostly trace/soft handling).
3. Delta policy hardening
- delta applicability heuristics are functional but need tuning/telemetry hardening.
4. Deferred boundary-hygiene candidate tracking
- Deferred extraction candidate from `engine-game-boundary-hygiene` (world/package transfer assembler path in `src/game/client/net/client_connection.cpp`) remains intentionally game-owned.
- Revisit only via a new explicitly scoped project doc with protocol-boundary review entry criteria and acceptance gates; do not silently fold this into generic engineization work.

## Execution Plan
1. Add stronger transfer-integrity controls for streamed world payloads.
2. Enforce explicit compatibility/version gating policy.
3. Harden delta-selection policy with trace-backed tuning and regression tests.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-server-net.sh
```

Manual smoke:

```bash
./build-dev/bz3-server -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -w common
./build-dev/bz3 -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -n tester
```

## Trace Channels
- `config`
- `engine.server`
- `net.server`
- `net.client`

## Risks
- Delta selection may be suboptimal under some content profiles without additional policy tuning.
- Compatibility drift can surface as late runtime failures until explicit gating is enforced.
- Transfer retries currently rely on chunk ordering/size invariants without per-chunk cryptographic integrity.

## Archive Reference
Full legacy material preserved at:
- `docs/archive/content-mount-legacy-2026-02-09.md`

## Handoff Checklist
- [x] Mount precedence contract preserved.
- [x] Package apply safety constraints enforced.
- [x] Cache/revision behavior validated with trace evidence.
- [x] Tests and docs updated for current protocol/semantic changes.
- [x] Resume/retry semantics implemented and validated.
- [ ] Explicit compatibility gating policy implemented.
- [ ] Delta policy tuning validated across representative world-update cases.
