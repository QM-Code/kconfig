# Testing + CI + Docs Governance

## Project Snapshot
- Current owner: `codex`
- Status: `in progress`
- Immediate next task: monitor wrapper adoption of optional build-dir usage across delegated tracks while preserving CI default behavior.
- Validation gate: `./scripts/test-engine-backends.sh [build-dir]` and `./scripts/test-server-net.sh [build-dir]`

## Mission
Own quality gates, test wrappers, CI workflows, and cross-track documentation consistency.

## Primary Specs
- `docs/projects/server-network.md`
- `docs/projects/engine-backend-testing.md`
- `docs/projects/core-engine-infrastructure.md` (references + delegation readiness)

## Why This Is Separate
This project is support infrastructure and can run in parallel with implementation projects.

## Owned Paths
- `m-rewrite/scripts/test-*.sh`
- `m-rewrite/.github/workflows/*`
- `docs/projects/server-network.md`
- `docs/projects/engine-backend-testing.md`
- `docs/projects/*`

## Interface Boundaries
- Inputs: new tests or run requirements from implementation projects.
- Outputs: enforced and documented validation procedures.
- Coordinate before changing:
  - `AGENTS.md`
  - `docs/projects/core-engine-infrastructure.md`

## Non-Goals
- Implementing subsystem behavior changes unless explicitly assigned.

## Validation
From `m-rewrite/`:

```bash
./scripts/test-engine-backends.sh
./scripts/test-server-net.sh
```

## Trace Channels
- Not subsystem-specific; this project validates process and docs.

## Build/Run Commands
```bash
./scripts/test-engine-backends.sh
./scripts/test-server-net.sh
./scripts/test-engine-backends.sh build-sdl3-bgfx-jolt-rmlui-sdl3audio
./scripts/test-server-net.sh build-sdl3-bgfx-jolt-rmlui-sdl3audio
```

## First Session Checklist
1. Confirm all CTest targets are mapped to a documented run path.
2. Ensure wrapper scripts and playbooks are in sync.
3. Ensure CI workflows match wrapper behavior.
4. Ensure wrapper docs preserve default `build-dev` behavior while documenting optional build-dir usage.

## Current Status
- Baseline wrappers and CI coverage exist for engine backend tests and server/net tests.
- Wrapper scripts now support optional build-dir arguments while keeping backward-compatible default `build-dev` behavior.
- Documentation hierarchy is normalized and delegation-ready.
- Wrapper validation completed for both forms:
  - default path: `./scripts/test-engine-backends.sh`, `./scripts/test-server-net.sh`
  - explicit path form: `./scripts/test-engine-backends.sh build-dev`, `./scripts/test-server-net.sh build-dev`
- Ongoing work is to keep docs and gates synchronized as subsystem behavior evolves.

## Open Questions
- Should wrapper scripts enforce a strict fail on missing expected tests for selected build profiles?
- Should wrapper internals migrate configure/build plumbing into `bzbuild.py` after adding explicit server/net test hooks?

## Handoff Checklist
- [ ] New tests documented.
- [ ] Wrapper scripts updated if needed.
- [ ] CI workflow updated if needed.
- [ ] `AGENTS.md`, `docs/AGENTS.md`, and `docs/projects/*.md` aligned.
- [x] Wrapper default (`build-dev`) and optional build-dir usage both validated/documented.
