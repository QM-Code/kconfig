# Content Mount

## Project Snapshot
- Current owner: `in progress by delegated agent` (coordinate before overlapping edits)
- Status: `in progress`
- Immediate next task: finish cache/revision hardening and archive safety constraints on top of existing mount/package flow.
- Validation gate: `./scripts/test-server-net.sh` plus manual client/server world-override smoke.

This is the canonical project file for content mount abstraction and world package behavior.

## Newcomer Read Order
1. Read this top section only for assignment context.
2. Read `## Product Intent (Locked)`.
3. Read `## Current State (Implemented)`.
4. Read `## Current Gaps (Not Implemented Yet)`.
5. Execute from `## Execution Plan`.

## Consolidation
This file supersedes:
- `docs/projects/content-mount-abstraction-playbook.md`
- `docs/projects/content_mount_track.md`

## Quick Start
1. Read `AGENTS.md`.
2. Read `docs/AGENTS.md`.
3. Use this file as the only project-level source for content mount work.

## Note
The source-material sections below are preserved verbatim for no-loss migration context and may mention retired `*_track`/`*playbook` filenames.

---

## Source Material: content-mount-abstraction-playbook.md
# Content Mount Abstraction Playbook (m-rewrite)

## Project Track Linkage
- Delegation track: `docs/projects/content-mount.md`

## Agent Handoff Context
This playbook is a direct handoff document for rewrite core-infra item `#3` and is intended to be sufficient for another agent to start implementation immediately.

### Where this fits in the rewrite
- `m-rewrite` is the production architecture target.
- This document covers only the content-mount/world-package track, not gameplay feature parity.
- The rewrite-level priority is to keep engine ownership and backend boundaries intact while restoring `m-dev`-proven world package behavior.

### Project state relative to this topic
- Core data root selection is implemented (`--data-dir`, user `DataDir`, `BZ3_DATA_DIR`).
- World archive helpers exist (`BuildWorldArchive`, `ExtractWorldArchive`).
- Server currently applies local world runtime config from filesystem.
- Mount abstraction semantics are implemented (package mount precedence + mount-point matching).
- Rewrite runtime now sends/applies world package payloads end-to-end.
- Join/init path now includes world package hash metadata and client cached-hash hint.
- Join/init path now carries world identity metadata (`world_id`, `world_revision`) with client-side cache identity validation.

### Core goals to preserve
- No `-w` means default mode.
- `-w <world-dir>` means world override mode on top of default shipped content.
- In world override mode, server packages world content and client applies it as an overlay to local defaults.
- Minimal world-builder package should work with `config.json` only; common package is `config.json` + `world.glb`.

### Hard guardrails (do not violate)
- Work in `m-rewrite/` only.
- Keep engine/game boundaries strict.
- Preserve current local filesystem behavior when no package mount is active.
- Keep trace categories disciplined and consistent with existing channels.

### Start Here (for the next agent)
1. Read this file fully, then `docs/projects/core-engine-infrastructure.md` section `3B`.
2. Implement Phase 1 in this file first: mount precedence and mount-point matching in resolver.
3. Validate no-regression by running server/client in default mode with no `-w`.
4. Implement Phase 2 after Phase 1 is stable: wire server init payload encode/decode to actually carry/apply world package bytes.
5. Add/update traces around mount table activation and world package apply lifecycle.

### Immediate first code targets
- `src/engine/common/data_path_resolver.cpp`
- `src/engine/common/data_path_resolver.hpp`
- `include/karma/common/data_path_resolver.hpp`
- `src/game/net/protocol_codec.cpp`
- `src/game/server/net/enet_event_source.cpp`
- `src/game/client/net/client_connection.cpp`

## Purpose
Define and execute rewrite item `#3 Content mount abstraction` so BZ3 can:
- keep default runtime data under `BZ3_DATA_DIR` (or overrides),
- let `bz3-server -w <world-dir>` provide world-specific config/assets as overrides,
- transfer world packages to clients and apply them as layered overrides over defaults.

This document captures:
- what is already implemented,
- what is missing,
- concrete behavior/CLI semantics to lock,
- implementation phases and acceptance criteria.

---

## Product Intent (Locked)

### Default vs custom world mode
- Default mode is implied when `-w/--world` is not provided.
- `-w/--world <world-dir>` means: use that world content as an override layer on top of defaults.

### Server and client behavior
- Server always has a default base data root from `BZ3_DATA_DIR`/config/CLI data-dir selection.
- When `-w` is provided, server uses world config/assets from `<world-dir>` to override base defaults.
- Server sends packaged world content to clients for that session/world.
- Client keeps local shipped defaults and applies received world content as higher-priority overrides.

### World builder workflow target
- A minimal world package may include only `config.json`.
- Common package includes `config.json` + `world.glb`.
- Optional package includes replacement assets (for example `audio/*.wav`, custom models, fonts, shaders).

---

## Current State (Implemented)

## A) Data root override pipeline (objective 1 foundation)
- Implemented in rewrite with precedence:
  - CLI `-d/--data-dir`
  - user config `"DataDir"`
  - env var (`BZ3_DATA_DIR`)
- Relevant code:
  - `src/engine/common/data_dir_override.cpp`
  - `src/game/client/bootstrap.cpp`
  - `src/game/server/bootstrap.cpp`

## B) World archive utilities exist
- Zip build/extract helpers already exist:
  - `src/engine/common/world_archive.cpp`
  - `include/karma/common/world_archive.hpp`

## C) Server world config runtime layer (local filesystem)
- Server resolves `-w`/`defaultWorld`, loads `<world>/config.json`, and adds runtime config layer.
- Relevant code:
  - `src/game/server/domain/world_session.cpp`

## D) Mount abstraction semantics are implemented
- New content mount contract added:
  - `ContentMountType`
  - `ContentMount`
  - `RegisterPackageMount`, `ClearPackageMounts`, `GetContentMounts`
- `Resolve()` now applies deterministic package overlay resolution before filesystem fallback:
  - mount-point path matching,
  - more-specific mount points win,
  - tie-breaker by latest registration order.
- Relevant code:
  - `src/engine/common/data_path_resolver.cpp`
  - `src/engine/common/data_path_resolver.hpp`
  - `include/karma/common/data_path_resolver.hpp`

## E) Rewrite network world package path is wired
- Server now builds/sends world package bytes in init payload when `-w` is active.
- Client now decodes/apply path:
  - receives `world_data` from init payload,
  - extracts under per-server user world cache,
  - clears prior package mounts/runtime layer,
  - registers package mount overlay,
  - applies runtime config layer from package `config.json` when present.
- Bundled/default mode (`no -w`) remains package-free transfer behavior.
- Relevant code:
  - `src/game/server/domain/world_session.cpp`
  - `src/game/server/net/enet_event_source.cpp`
  - `src/game/server/runtime.cpp`
  - `src/game/net/protocol_codec.cpp`
  - `src/game/client/net/client_connection.cpp`

## F) Package identity and cache hint wiring is implemented
- Server now computes and sends package hash/size metadata in `ServerMsg_Init`.
- Server now also sends `world_content_hash` metadata in `ServerMsg_Init` for stable content identity.
- Server now sends manifest summary metadata (`world_manifest_hash`, `world_manifest_file_count`) in `ServerMsg_Init` as delta-transfer groundwork.
- Server now sends a manifest entry list (`world_manifest[]`: path/size/hash) in `ServerMsg_Init` for future delta/chunk negotiation.
- Client now sends cached world identity hint in `ClientMsg_JoinRequest` (`cached_world_hash`, `cached_world_content_hash`, `cached_world_id`, `cached_world_revision`).
- Client now also sends cached manifest summary hint in `ClientMsg_JoinRequest` (`cached_world_manifest_hash`, `cached_world_manifest_file_count`).
- Client now sends cached manifest entry hints in `ClientMsg_JoinRequest` (`cached_world_manifest[]`: path/size/hash) when cached identity is present.
- Server skips world package bytes only when cached identity matches (`id + revision`) and one of (`hash`, `content_hash`, or manifest summary `manifest_hash + file_count`) matches.
- On cache-hit when manifest summary matches, server may omit `world_manifest[]` entry payload to reduce init size while still sending manifest summary metadata.
- Client applies cached package mount directly when init includes matching hash with no payload.
- Client enforces cache identity checks on no-payload init (`world_id` + `world_revision` + (`world_hash` or `world_content_hash` or manifest summary `manifest_hash + file_count`)) and invalidates stale cache hints on mismatch.
- Client reuses cached manifest sidecar when init omits `world_manifest[]` but manifest summary metadata matches.
- Client now persists cached manifest entries per server and logs trace-only manifest diff planning summaries (`unchanged/added/modified/removed` + potential transfer bytes).
- Server now computes/logs trace-only manifest diff planning summaries from client-cached manifest entries versus authoritative world manifest (`unchanged/added/modified/removed` + potential transfer bytes + reused bytes).
- Runtime delta transfer path is now active:
  - server compares client-cached manifest entries against authoritative manifest and may choose `chunked_delta` mode when overlap exists and delta payload is smaller than full package,
  - server streams a delta archive (changed files + removed-path list metadata) over `world_transfer_begin/chunk/end`,
  - client applies delta archive over cached base package (same `world_id`, prior revision), then mounts/persists as the new revision.

---

## Current Gaps (Not Implemented Yet)

## 1) Transfer efficiency and coherence
- No package hash manifest/state yet.
- Initial file-level delta transfer is implemented through manifest-driven delta archives over the existing chunk stream.
- No package revision handshake contract yet.
- Application-level chunked world-package transfer runtime path is implemented for both:
  - full package transfer on cache miss (`chunked_full`),
  - manifest-driven delta archive transfer when beneficial (`chunked_delta`).
- Remaining chunk-transfer work:
  - resume/retry semantics for interrupted transfers,
  - stronger transfer-integrity constraints beyond end-of-transfer size/hash checks,
  - evolution from delta-archive patching to direct file/object transfer protocol (no temporary archive packaging step).

## 2) Archive safety and package hardening
- Baseline extraction safety is implemented in `ExtractWorldArchive`:
  - path traversal/absolute-path rejection,
  - entry count cap,
  - per-file and total uncompressed size caps.
- UNIX archive entries with unsupported file types (symlink/device/fifo/etc.) are rejected during extraction planning.
- Client package cache now uses revisioned paths (`world-packages/by-world/<world-id>/<world-revision>/<world-hash>`).
- Client package activation now uses staged extraction + atomic rename with rollback of previous package root on failure.
- Client now verifies staged package contents against transfer metadata (`content_hash`, manifest summary, optional manifest entries) before activation for both full and delta transfer modes.
- Delta apply now clones cached base into staging, applies delta/removals in staging, verifies, then atomically promotes to the target revision path.
- Integration coverage now includes client cache-safety failure injection:
  - `src/game/tests/client_world_package_safety_integration_test.cpp` seeds revision A, injects a corrupted delta-content-hash update for revision B, and asserts active cached identity/package remain on A.
- Client cache retention pruning is implemented (LRU-by-timestamp) for:
  - max revisions per world,
  - max package dirs per revision,
  - with active world identity preserved from pruning.
- Remaining hardening:
  - optional policy tuning and telemetry for cache growth.

## 3) Compatibility metadata contract
- Implemented baseline checks:
  - protocol version check during handshake,
  - init payload size/hash consistency check before world archive extraction.
- Implemented identity baseline:
  - `world_id`/`world_revision` metadata transmitted in init,
  - client requires non-empty world identity metadata when applying world packages,
  - cache-hit (no payload) apply path validates cached identity (`id/revision` + `hash|content_hash`) before mount.
- Remaining:
  - richer content compatibility gating beyond hash/size integrity.

---

## Alignment With `m-dev` (Reference Behavior)

`m-dev` already proved the core concept:
- Server zips custom world and sends in init message.
- Client extracts under user world directory and merges world config runtime layer.
- World asset keys can override defaults (for example audio/model/world paths).

Important nuance from `m-dev`:
- Zip transfer is tied to custom-world path (`-w`); bundled default mode skips world zip transfer.

Rewrite should preserve this practical behavior while moving it behind cleaner engine-owned mount contracts.

---

## CLI and Behavior Contract (Proposed for Rewrite)

## Data root
- Keep `-d/--data-dir`, user `"DataDir"`, and `BZ3_DATA_DIR`.
- Optional compatibility alias to consider: accept `KARMA_DATA_DIR` as fallback env var with trace warning.

## World selection flags
- `-w/--world <dir>`:
  - selects custom world overlay source.
  - enables server world package transfer behavior.
- no `-w`:
  - implied default/bundled mode.

## Effective content stack
- Base (lowest priority): resolved default data root (`data/common`, `data/client`, `data/server`).
- World package overlay (higher priority): server-selected world package.
- Optional future patch overlay (highest priority): incremental file overrides.

---

## Design Risks and Mitigations

## Risk: full zip transfer per client is bandwidth-heavy
- Mitigation (implemented): client/server hash hint and package cache allow no-payload init for unchanged world.
- Mitigation (phase 2): file-level delta/patch transfer.
- Mitigation (phase 2/4): application-level chunked transfer path for large world packages to avoid single-payload size pressure.

## Risk: compatibility drift between client build and world package
- Mitigation: include protocol/content version metadata and enforce compatibility checks before mount.

## Risk: unsafe archive contents (path traversal, zip bombs)
- Mitigation: strict extraction validation (`..`, absolute paths, size caps, file count caps) and fail closed.

## Risk: stale mixed content after world change
- Mitigation: per-world revision directories and atomic mount switch.

---

## Execution Plan

## Phase 1: Mount semantics hardening
1. Implement resolver mount precedence and mount-point matching.
2. Keep filesystem behavior unchanged when no package mounts exist.
3. Add trace channel output for active mount table at world/session transitions.

Acceptance:
- Relative resolution follows deterministic mount order.
- Existing local-data workflows remain unchanged.

## Phase 2: Rewrite network world package path
1. Extend server init payload encode path to include world package bytes when `-w` active.
2. Extend client decode/apply path to persist package in user world cache and load world config layer.
3. Apply world config as runtime layer with package-root base path for asset resolution.

Acceptance:
- `-w` world on server changes client world/config/assets without client-side manual files.
- No `-w` remains default/bundled behavior.

## Phase 3: Package identity and cache (partially complete)
Done:
1. Added package metadata fields (`world_hash`, `world_size`, `world_content_hash`) on init payload.
2. Added manifest summary metadata fields (`world_manifest_hash`, `world_manifest_file_count`) on init payload.
3. Added manifest entry list field (`world_manifest[]`: path/size/hash) on init payload.
4. Added client cached identity hint fields (`cached_world_hash`, `cached_world_content_hash`) on join request.
5. Added client cached manifest summary hint fields (`cached_world_manifest_hash`, `cached_world_manifest_file_count`) on join request.
6. Added identity-based no-payload init behavior when cache matches (`id/revision` + `hash|content_hash|manifest_summary`).

Remaining:
1. Tune client cache retention/eviction defaults/telemetry for accumulated revisions.

## Phase 4: Incremental content updates
1. Manifest-based file hash index on init metadata (`world_manifest[]`) is implemented.
2. Transfer only changed files (patch package or object list).
3. Chunked transport envelope is implemented for full world package transfer on cache miss; delta/resume/integrity hardening is still pending.
4. Mount patch overlay above base package.

Acceptance:
- single-asset update (for example one texture or audio file) does not require full world package transfer.

---

## Test Matrix (Minimum)

## Startup/data root
- CLI data-dir override wins over user config and env.
- User config DataDir wins over env.
- Env-only startup works with valid marker file.

## World mode behavior
- No `-w`: bundled default mode.
- `-w`: world package mode with runtime overrides applied.

## Override correctness
- World `config.json` overrides default keys.
- World asset path overrides resolve from world package root.
- Missing world keys fall back to defaults.

## Robustness
- Invalid zip rejected safely.
- Path traversal entries rejected.
- Hash mismatch rejects package and preserves prior good state.

---

## Done vs Remaining Summary

Done now:
- Data root override pipeline.
- World archive helpers.
- Server local world config runtime layer.
- Mount precedence/mount-point matching implementation in resolver.
- Rewrite server->client world package send/receive/apply path.
- Hash/content-hash package metadata + cache-hint handshake (`world_hash`, `world_content_hash`, `world_size`, `cached_world_hash`, `cached_world_content_hash`).
- Manifest summary metadata on init (`world_manifest_hash`, `world_manifest_file_count`) to support future patch/delta negotiation.
- Manifest entry list on init (`world_manifest[]`) to enable file-level diff/chunk planning.
- Client-side manifest cache sidecar + diff-planning traces for incremental transfer observability.
- Server-side cache-hit decision now also recognizes matching cached manifest summary as a no-payload init fast path.
- Runtime chunked world-package transfer path (`world_transfer_begin/chunk/end`) for cache-miss full package delivery.
- Baseline archive extraction safety checks (path and size guards).

Remaining:
- Revision/compatibility metadata hardening and incremental transfer runtime completion (file-level delta + chunked resume/integrity hardening).
- Advanced package hardening (cache retention telemetry/policy refinement) and compatibility metadata checks.


---

## Source Material: content_mount_track.md

# Content Mount Track

## Mission
Own mount manager/data-path/world-package flow so default data and server-delivered world content resolve deterministically and safely.

## Primary Specs
- `docs/projects/content-mount.md`
- `docs/projects/core-engine-infrastructure.md` (content mount sections)

## Why This Is Separate
Content resolution and package application are mostly orthogonal to renderer/UI/physics/audio internals.

## Owned Paths
- `m-rewrite/src/engine/common/data_path_resolver.*`
- `m-rewrite/src/engine/common/world_archive.*`
- `m-rewrite/src/engine/common/world_content.*`
- client world-package apply/cache paths under `m-rewrite/src/game/client/net/*`

## Interface Boundaries
- Inputs: server world package metadata/payload, local data dir, user cache dirs.
- Outputs: mounted path precedence and active world data layer.
- Coordinate before changing:
  - protocol metadata fields in `messages.proto` and `protocol_codec.cpp`
  - `docs/projects/content-mount.md`

## Non-Goals
- Gameplay feature logic.
- Backend-specific renderer/audio/physics internals.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-server-net.sh
```

Plus targeted client/server runtime verification for package apply behavior.

## Trace Channels
- `config`
- `engine.server`
- `net.server`
- `net.client`

## Build/Run Commands
```bash
./abuild.py -a
./build-dev/bz3-server -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -w common
./build-dev/bz3 -d /home/karmak/dev/bz3-rewrite/m-rewrite/data -p 11911 -n tester
```

## First Session Checklist
1. Read `docs/projects/content-mount.md` fully.
2. Confirm current mount precedence and package cache assumptions.
3. Implement narrow changes.
4. Validate with server/client loop + relevant tests.
5. Update this project file status and handoff notes.

## Current Status
- See `Project Snapshot` at top of file for active owner and status.

## Open Questions
- See `Project Snapshot` and `Newcomer Read Order` at top of file.

## Handoff Checklist
- [ ] Mount precedence rules documented.
- [ ] Package apply safety constraints preserved.
- [ ] Relevant runtime tests/log captures attached.
- [ ] Content-mount project doc updated.
