# Demo Data Root

`demo/` is the canonical tracked location for local test/demo fixtures and reusable state.

## Layout
- `demo/communities/`
  - webserver community directories (`config.json`, db/log/upload content).
- `demo/users/`
  - user-home fixtures for client-side config/state simulation.
  - when needed, point `HOME` to a subdir here for deterministic runs.
- `demo/worlds/`
  - world directories and world archives used by `--world` testing.

## Policy
- Prefer `demo/*` paths for reproducible local testing.
- Do not use personal `~/.config/bz3` for reusable test fixtures.
- Do not use ad-hoc `/tmp` paths for reusable test fixtures.
