# Webserver Agent Guide

## Purpose
This file is the canonical agent guide for `src/webserver/`.

Use it for:
- community-host operations support (initialize/start/check/maintenance),
- developer changes inside the webserver code.

## Scope And Ownership
- `src/webserver/` is a game-agnostic community-management sidecar service.
- It is not `src/game/` ownership.
- It is not engine runtime SDK surface (`src/engine/`), but part of the engine ecosystem toolchain.

## Read Order
1. `src/webserver/AGENTS.md` (this file)
2. Relevant entrypoint code:
   - `src/webserver/bin/start.py`
   - `src/webserver/bin/initialize.py`
   - `src/webserver/karma/app.py`
   - `src/webserver/karma/cli.py`
3. If changing behavior, inspect affected handlers under `src/webserver/karma/handlers/`.

## Runtime Model
- App type: Python WSGI.
- Server backend:
  - prefers `waitress` when installed,
  - falls back to stdlib `wsgiref`.
- Config layers:
  - distribution defaults: `src/webserver/config.json` (authoritative),
  - community overrides: `<community>/config.json`.

## Community-Host Operations Mode
Primary operator tasks:
1. Initialize a new community directory:
   - `python3 ./src/webserver/bin/initialize.py <community-dir>`
2. Start service:
   - `python3 ./src/webserver/bin/start.py <community-dir>`
   - optional port override: `-p <port>`
3. Health checks:
   - `curl -fsS http://127.0.0.1:<port>/api/health`
   - `curl -fsS http://127.0.0.1:<port>/api/info`
4. Data maintenance helpers:
   - `python3 ./src/webserver/bin/db-snapshot.py <community-dir> <output.json>`
   - `python3 ./src/webserver/bin/db-restore.py <community-dir> -f <input.json>`
   - `python3 ./src/webserver/bin/db-merge.py <community-dir> -f <input.json>`
   - `python3 ./src/webserver/bin/clean-images.py <community-dir> [--dry-run]`

Community-dir policy:
- use tracked demo community roots under `demo/communities/*` for local testing.
- avoid ad-hoc `/tmp/*` community dirs for reusable test fixtures.

## Developer Mode
When changing webserver code:
1. Keep changes scoped to `src/webserver/*`.
2. Run string validation after any translation or UI-text key change:
   - `python3 ./src/webserver/tests/validate_strings.py --all`
3. For basic runtime smoke:
   - start server on a tracked demo community (for example `./demo/communities/r55man`),
   - verify `/api/health` and `/api/info`.
4. Optional dev-data helpers:
   - `python3 ./src/webserver/tests/makedata.py <community-dir> -s <n> -u <n>`
   - `python3 ./src/webserver/tests/heartbeat.py <community-dir> [-w <seconds>]`

## API Surface (High-Level)
- `/api/health`: service health check.
- `/api/info`: community metadata.
- `/api/servers`, `/api/servers/active`, `/api/servers/inactive`: server list endpoints.
- `/api/server/<token>`: server details.
- `/api/auth`: auth endpoint.

## Guardrails
- Do not store plaintext passwords.
- Keep config defaults in `src/webserver/config.json`; avoid new hardcoded defaults in Python code.
- Keep UI strings in `src/webserver/strings/*.json`; avoid hardcoded English text in handlers/views.
- Preserve path-containment checks for static/uploads serving.
- Preserve CSRF behavior and auth/session validation paths.

## Path Conventions
- Run commands from `m-rewrite/`.
- Use explicit repo-relative paths (`src/webserver/...`) in handoffs.

## Handoff Minimum
- Scope: what changed and what did not.
- Validation: exact commands and outcomes.
- Risk: open issues/assumptions.
