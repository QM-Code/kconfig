# AGENTS.md

This file provides quick, repo-specific instructions for coding agents.

## Alignment Tenets (must follow)
### Debugging core tenets (must follow)
- **Do not rely on "eureka" moments from reading code.** Use the logging system to isolate problems.
- **Always compare against working builds.** We routinely have multiple working build targets (bgfx/diligent, physx/jolt, imgui/rmlui). Use differences in output and behavior to narrow the fault domain.
- **Prefer trace-first debugging once a quick fix fails.** If one or two targeted changes don't solve it, switch to trace instrumentation and comparison across builds.
### Trace logging system (how to use)
- **Enable debug:** `-v` (debug level). Use this for general diagnostics.
- **Enable trace channels:** `-t <comma-separated>` (e.g., `-t ui.rmlui,render.bgfx`). Trace is opt-in by channel.
- **No implicit trace:** `-t` requires an explicit list. Use `-t ui` or `-t ui.rmlui` etc.
- **Channel behavior:** Only trace lines in enabled channels print. Uncategorized trace should be avoided; add categories.
- **Use macros:** Prefer `KARMA_TRACE` / `KARMA_TRACE_CHANGED` over `spdlog::trace` so channels work consistently.
- **Runtime gating:** Expensive trace logic must be wrapped in `ShouldTraceChannel()` or `KARMA_TRACE_CHANGED()` so it doesn't run when disabled.
- **Change-only helpers:** Use `KARMA_TRACE_CHANGED(cat, key, ...)` at noisy callsites (render loops, per-frame systems).
- **Debug vs trace:** Use **[debug]** for temporary, problem-specific logging that should be removed once fixed. Use **[trace]** for recurring, broadly useful diagnostics (most debugging should be trace). When unsure, prefer trace and categorize it.
### Comparing build targets (recommended workflow)
- Reproduce the issue in the failing build.
- Run a known-working build with the same trace channels.
- Diff the trace output to see where behavior diverges (first missing/extra action, different IDs, sizes, or sequence).
- Make the smallest change necessary to align the behavior, then re-test across the matrix.

## Project summary
- BZ3 is a C++20 client/server 3D game inspired by BZFlag.
- Two binaries: `bz3` (client) and `bz3-server` (server).
- Runtime assets/config resolve from `KARMA_DATA_DIR` (usually `data/`).

## Key directories
- `src/game/client/`: client gameplay and EngineApp wiring.
- `src/game/server/`: server gameplay and EngineApp wiring.
- `src/game/engine/`: BZ3-specific engine orchestrators.
- `src/engine/core/`: shared types and constants.
- `src/engine/graphics/`: engine-agnostic graphics API + backends.
- `src/game/renderer/`: Game render orchestration and radar rendering.
- `src/engine/geometry/`: mesh loading utilities.
- `src/engine/audio/`: audio system.
- `src/engine/input/`: input handling.
- `src/engine/physics/`: physics world and bodies.
- `src/engine/network/`: networking transports.
- `src/game/net/`: game protocol, codec, and message-level networking.
- `src/engine/platform/`: platform glue (SDL window/events).
- `src/game/ui/`: UI entry point, backends, and shared UI interfaces.
- `src/engine/common/`: config/data root resolution helpers.
- `src/karma-extras/`: optional UI frontends + helpers.
- `src/game/protos/`: protobuf schema.
- `data/`: configs, assets, worlds, plugins.
- `webserver/`: optional Python community server.

## Build and run
Linux/macOS:
- `./setup.sh`
- `cmake --build build`
- `KARMA_DATA_DIR="$PWD/data" ./build/bz3`
- `KARMA_DATA_DIR="$PWD/data" ./build/bz3-server`

Windows:
- `setup.bat`
- `cmake --build build --config Release`
- `set KARMA_DATA_DIR=%CD%\data`
- `build\Release\bz3.exe`
- `build\Release\bz3-server.exe`

## Common workflows
- New networked feature: update `src/game/protos/messages.proto`, then encode/decode in
  `src/game/net/*`, then handle in `src/game/client/*` and/or `src/game/server/*`.
- World loading changes: `src/game/server/world_session.*` and `src/game/client/world_session.*`.
- UI/HUD changes: `src/game/ui.*`.
- Plugins: `src/game/server/plugin.*` and `data/plugins/*`.

## Notes / gotchas
- Config is layered via `src/engine/common/data_path_resolver.*`; prefer asset keys over hard paths.
- Network messages are "peeked" and must be freed on `flushPeekedMessages()`. Do not store pointers.
- Keep `architecture.md` and `README.md` in sync when behavior or layout changes.
- When moving or renaming code/modules, update any related docs in `README.md`, `architecture.md`, and `CONFIG-SCHEMA.md`.
- If you add a `ReadRequired*Config` or `ui::config::GetRequired*` call, update `ClientRequiredKeys()` / `ServerRequiredKeys()` in `src/engine/common/config_validation.*`.
- Use `scripts/check_required_config.py` to verify the required-key list stays in sync.
- Input gating: gameplay input is suppressed when UI input is active (console visible or chat focused). See `src/game/engine/client_engine.cpp` and `src/game/ui/core/system.*` if you need to tweak which actions remain global.

## Prompt index
Prompt files live in `docs/agent-prompts/`. Ask to use one by name (title) or file.

- Webserver: `docs/agent-prompts/webserver.md`
- UiSystem/HUD: `docs/agent-prompts/gui-hud.md`
- Template: `docs/agent-prompts/_TEMPLATE.md`

## Tests
- None automated in this repo (manual run is typical).
- If `python` is missing, prefer `python3` for scripts/tools.
