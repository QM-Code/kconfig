# Projects Index (Transient Execution Tracks)

`docs/projects/` is for active, discardable execution tracking only.

Durable policy/governance/architecture belongs in `docs/foundation/`.

## How To Use
1. Read `AGENTS.md`.
2. Read `docs/foundation/policy/execution-policy.md`.
3. Pick one project file below.
4. Treat that project file as the execution source of truth for the slice.
5. Update `docs/projects/ASSIGNMENTS.md` in the same handoff.

## Active Project Files
- `content-mount.md`
- `core-engine-infrastructure.md`
- `gameplay-netcode.md`
- `physics-backend.md`
- `renderer-parity.md`
- `renderer-shadow-hardening.md`
- `ui-integration.md`

## Current Focus Board
- `renderer-shadow-hardening.md`: P0 active.
- `renderer-parity.md`: P0 active (ledger + parity guardrails while shadow hardening executes).
- `core-engine-infrastructure.md`: in progress (integration sequencing and contract rollout tracking).
- `physics-backend.md`: in progress (backend parity depth and contract hardening).
- `content-mount.md`: in progress (transfer integrity + compatibility hardening).
- `ui-integration.md`: queued/in progress follow-up slices.
- `gameplay-netcode.md`: queued behind current renderer/network priorities.

## Assignment and Build Policy
- Keep one owner per project whenever possible.
- Use `./bzbuild.py <build-dir>` only for configure/build/test workflows.
- In parallel work, use isolated build dirs and explicit wrapper build-dir args:
  - `./scripts/test-engine-backends.sh <build-dir>`
  - `./scripts/test-server-net.sh <build-dir>`

## Related Files
- `PROJECT_TEMPLATE.md`: template for new transient project docs.
- `ASSIGNMENTS.md`: active owner/status board.
- `docs/archive/`: completed or legacy snapshots (reference-only).
