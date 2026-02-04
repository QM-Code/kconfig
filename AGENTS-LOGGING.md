# AGENTS-LOGGING.md

This file is for agents working on logging cleanup and trace channelization.

## Goal
Replace **all** `spdlog::trace` usage with `KARMA_TRACE` (or `KARMA_TRACE_CHANGED`) so trace output is opt-in by channel, low-overhead when disabled, and consistent across the codebase.

## What to change
- **Replace** `spdlog::trace(...)` with:
  - `KARMA_TRACE("<category>", "...", ...)` for normal trace.
  - `KARMA_TRACE_CHANGED("<category>", key_expr, "...", ...)` for noisy, per-frame logs.
- **Do not** change `info/warn/error` levels unless explicitly asked.
- **Do not** add compile-time guards. Runtime channel gating is the standard.

## Trace categories (current baseline)
Use existing categories unless there is a strong reason to add a new one.

Core categories (non-exhaustive):
- `platform.sdl`
- `engine.app`
- `engine.client`
- `ui`, `ui.rmlui`, `ui.rmlui.fonts`, `ui.imgui`
- `render`, `render.bgfx`, `render.bgfx.ui`, `render.diligent`, `render.diligent.ui`
- `game.radar`
- `config`
- `net.client`, `net.server`

If you need a new category, add it to `GetDefaultTraceChannelsHelp()` in `src/engine/common/logging.cpp`.

## When to use KARMA_TRACE_CHANGED
Use change-only at high-frequency callsites (render loops, per-frame systems), especially when the output is repetitive. Good examples:
- Per-frame UI draw summaries.
- Per-frame render target updates.
- Frequent entity/camera state logs.

Example:
```
KARMA_TRACE_CHANGED("render",
                    std::to_string(cmds) + "|" + std::to_string(verts),
                    "Render summary cmds={} verts={}", cmds, verts);
```

## Do's and Don'ts
- Do keep trace logs cheap; avoid heavy formatting when trace isn't enabled.
- Do categorize every trace call.
- Do compare outputs across working build targets (bgfx/diligent, rmlui/imgui).
- Don't leave uncategorized `spdlog::trace` calls.
- Don't change non-trace levels unless asked.

## How to search and patch
Use ripgrep:
- `rg -n "spdlog::trace" src`
- `rg -n "trace\\(" src` (to find custom trace wrappers)

As you convert, prefer `KARMA_TRACE` unless a change-only key is clearly needed.

## Output expectations
When running with `-t ui.rmlui,render.bgfx`, only trace lines in those channels should appear (plus any uncategorized trace, which we're eliminating).

## Recent changes (for context)
- Trace channels are opt-in with `-t`.
- `-v` enables debug; `-t` enables trace channels; `-vv` is removed.
- Category names are colorized in terminal output (Linux).
