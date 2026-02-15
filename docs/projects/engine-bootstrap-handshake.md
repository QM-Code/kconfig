# Engine Bootstrap + Handshake Generalization

## Project Snapshot
- Current owner: `specialist-engine-bootstrap`
- Status: `in progress`
- Immediate next task: replace game-local client `--name` + `--addr` with engine-owned `--credentials` + `--server`.
- Validation gate: `./scripts/test-engine-backends.sh <build-dir>` and `./scripts/test-server-net.sh <build-dir>`.

## Mission
Generalize client/server startup infrastructure so game teams can launch multiplayer with minimal custom code:

```bash
./mygame-server --listen-port 12345 --server-config /path/config.json
./mygame --server localhost:12345 --credentials "user:secret"
```

Engine should own CLI parsing, config layering, connection/listen lifecycle, and pre-auth handshake scaffolding. Game code should only implement auth policy and gameplay behavior.

## Foundation References
- `docs/foundation/architecture/core-engine-contracts.md`
- `docs/foundation/architecture/engine-defaults-model.md`
- `docs/foundation/policy/rewrite-invariants.md`

## Why This Is Separate
This work cuts across CLI, bootstrap/config layering, networking, and game startup APIs. It needs a single integration track to avoid duplicated one-off logic in `src/game/*`.

## Owned Paths
- `include/karma/app/*`
- `src/engine/app/*`
- `src/engine/network/*`
- `include/karma/common/*` and `src/engine/common/*` (only for shared bootstrap/config helpers)
- Game adapters only where needed to consume engine interfaces:
  - `src/game/client/*`
  - `src/game/server/*`

## Interface Boundaries
- Engine owns:
  - Standard client/server CLI contract (`--listen-port`, `--server-config`, `--server`, `--credentials`, `--community`).
  - Generic overlay file load/parse/apply ordering.
  - Transport connect/listen and pre-auth handshake flow.
  - Accept/reject/disconnect plumbing and reason propagation.
- Game owns:
  - Auth logic and credential semantics.
  - Game config schema beyond generic bootstrap keys.
  - Gameplay protocol and post-auth message handling.
- Coordination files:
  - `docs/projects/gameplay-netcode.md`
  - `docs/projects/core-engine-infrastructure.md`

## Non-Goals
- Do not hardcode auth schemes in engine (`user:pass`, hash, key, token).
- Do not keep legacy CLI aliases for migrated flags.
- Do not entangle engine handshake scaffolding with gameplay state/rules.

## Target Developer Experience
Game developer should not need to write bespoke command-line parsing or low-level handshake wiring to get a working connect/auth hook path.

Minimum expected game integration should look like:

```cpp
return karma::app::RunServer(argc, argv, game_server_hooks);
return karma::app::RunClient(argc, argv, game_client_hooks);
```

Where hooks include callbacks for:
- auth request validation,
- session accepted/rejected behavior,
- gameplay message dispatch after auth.

## Execution Plan
1. Server CLI ownership completion:
   - Move `--community` into engine runtime options.
   - Keep game server code consuming resolved values only.
2. Client CLI contract normalization:
   - Replace game-local `--name` + `--addr` with engine-owned `--credentials` + `--server`.
   - Remove `--dev-quick-start` and `--community-list-active`.
3. Engine handshake scaffolding:
   - Introduce pre-auth handshake model and callback interfaces.
   - Keep credentials opaque payload passed to game auth hook.
4. Game adapter thinning:
   - Reduce client/server mains to engine runner calls + game hooks.
5. Documentation and contract hardening:
   - Finalize stable CLI contract and bootstrap ordering.

## Required Ordering
1. Base config layers load.
2. Engine applies `--server-config` overlay (when present).
3. Explicit CLI key overrides apply last.
4. Required-config validation runs after final merged view.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-engine-backends.sh <build-dir>
./scripts/test-server-net.sh <build-dir>
./docs/scripts/lint-project-docs.sh
```

## Trace Channels
- `engine.app`
- `engine.server`
- `net.client`
- `net.server`
- `config`

## Current Status
- `2026-02-15`: Engine now owns server `--listen-port` + `--server-config` parse and overlay application. Remaining scope is full CLI/auth/bootstrap generalization.
- `2026-02-15`: Engine now owns server `--community` parse/help (`karma::cli::ConsumeServerRuntimeCliOption` + `AppendServerRuntimeCliHelp`); game-local `--community` registration was removed.

## Open Questions
- Should `--credentials` be a single opaque string only, or support multiple engine-level input forms later (file/env/stdin)?
- Should `--community` remain a general engine server option, or become an optional engine feature flag/module?
- Which stable engine hook shape is preferred first: callback struct, interface class, or event bus contract?

## Handoff Checklist
- [ ] CLI contract changes reflected in help and docs.
- [x] Game-local parsing removed for migrated options.
- [ ] Handshake/auth hook interfaces documented with one working slice.
- [ ] Engine/network validation and multiplayer smoke summarized.
