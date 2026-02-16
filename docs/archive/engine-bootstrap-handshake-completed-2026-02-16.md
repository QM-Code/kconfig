# Engine Bootstrap + Handshake Generalization

## Project Snapshot
- Current owner: `overseer`
- Status: `completed`
- Immediate next task: none (track closed; follow-up enhancements, if any, should be opened as new project slices).
- Validation gate: `./scripts/test-engine-backends.sh <build-dir>` and `./scripts/test-server-net.sh <build-dir>`.

## Mission
Generalize client/server startup infrastructure so game teams can launch multiplayer with minimal custom code:

```bash
./mygame-server --listen-port 12345 --server-config /path/config.json
./mygame --server localhost:12345 --username user --password secret
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
  - Standard client/server CLI contract (`--listen-port`, `--server-config`, `--server`, `--username`, `--password`, `--community`).
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
   - Replace game-local `--name` + `--addr` with engine-owned `--username` + `--password` + `--server`.
   - Remove `--dev-quick-start` and `--community-list-active`.
3. Engine handshake scaffolding:
   - Introduce pre-auth handshake model and callback interfaces.
   - Keep password opaque payload passed to game auth hook (do not log cleartext).
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
- `2026-02-15`: Engine now owns client `--username` + `--password` + `--server` parse/help (`karma::cli::ConsumeClientRuntimeCliOption` + `AppendClientRuntimeCliHelp`); game-local `--name`/`--addr` parsing was removed with no compatibility aliases.
- `2026-02-15`: Removed client `--dev-quick-start` and `--community-list-active` from parse/help/runtime paths; both flags now error as unknown options.
- `2026-02-15`: Added engine pre-auth handshake scaffolding (`karma::network::ServerPreAuth*`) and wired one working accept/reject slice through server join runtime. Client now sends opaque join `auth_payload` from `--password`, transport join events carry it to runtime, and server can enforce `network.PreAuthPassword` with `network.PreAuthRejectReason`.
- `2026-02-15`: Added engine-owned full app CLI parsers (`karma::cli::ParseClientAppCliOptions`, `karma::cli::ParseServerAppCliOptions`) and removed game-local parser implementations in `src/game/client/cli_options.cpp` and `src/game/server/cli_options.cpp`. Game headers now thinly alias engine option structs/parsers.
- `2026-02-15`: Removed remaining game CLI adapter headers (`src/game/client/cli_options.hpp`, `src/game/server/cli_options.hpp`) and switched runtime/tests to use `karma::cli::ClientAppOptions` + `karma::cli::ServerAppOptions` directly.
- `2026-02-15`: Reduced game bootstrap wrappers by moving client/server logging+config bootstrap runners into engine-owned helpers (`karma::app::RunClientBootstrap`, `karma::app::RunServerBootstrap`) and deleting game bootstrap implementation files.
- `2026-02-15`: Added engine-owned entry runners (`karma::app::RunClient`, `karma::app::RunServer`) that own parse/bootstrap/error lifecycle and dispatch to game runtime hooks. Game `main()` now only wires runtime callbacks and app-name spec.
- `2026-02-15`: Added first engine-owned server session hook flow (`karma::network::ServerSessionHooks`) for pre-auth + join/leave callback orchestration. `src/game/server/runtime.cpp` now uses engine join/leave decisions and no longer logs raw CLI flag names (e.g., `--listen-port`).
- `2026-02-15`: Moved community heartbeat ownership from game server code to engine network layer (`karma::network::CommunityHeartbeat`, `karma::network::HeartbeatClient`). Heartbeat remains disabled by default unless enabled via merged config `community.*` (e.g., world/server overlay config) or explicit `--community` override.
- `2026-02-15`: Moved server join-result packaging/emission scaffolding into engine network helper (`karma::network::BuildServerJoinResultPayload`, `karma::network::EmitServerJoinResult`) and rewired `src/game/server/runtime.cpp` to use it with an added contract test.
- `2026-02-15`: Moved server leave-event application branching into engine runtime helper (`karma::network::ApplyServerSessionLeaveEvent`) with centralized result mapping/description and rewired `src/game/server/runtime.cpp` to consume it via callback wiring only.
- `2026-02-15`: Explicitly scoped gameplay event routing (spawn/create-shot and similar) as game-owned; this project no longer targets moving gameplay event rules into engine.
- `2026-02-16`: Closed this track. Engine now owns CLI/bootstrap/session/auth/discovery scaffolding boundaries targeted by this project; gameplay event routing remains game-owned by policy.

## Open Questions
- Should `--password` remain raw CLI text for now, or gain safer input forms soon (env/file/stdin)?
- Should `--community` remain a general engine server option, or become an optional engine feature flag/module?
- Which stable engine hook shape is preferred first: callback struct, interface class, or event bus contract?

## Handoff Checklist
- [x] CLI contract changes reflected in help and docs.
- [x] Game-local parsing removed for migrated options.
- [x] Handshake/auth hook interfaces documented with one working slice.
- [x] Engine/network validation and multiplayer smoke summarized.
