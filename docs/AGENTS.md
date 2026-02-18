# AGENTS.md (m-rewrite)

This file is a bootstrap pointer for rewrite work.

Canonical long-lived docs are under `docs/foundation/`:
- rewrite invariants: `docs/foundation/policy/rewrite-invariants.md`
- execution policy: `docs/foundation/policy/execution-policy.md`
- manager workflow: `docs/foundation/governance/overseer-playbook.md`
- durable decisions: `docs/foundation/policy/decisions-log.md`
- architecture contracts/models: `docs/foundation/architecture/*`

Active execution tracking stays in:
- `docs/projects/AGENTS.md`
- `docs/projects/ASSIGNMENTS.md`
- `docs/projects/<project>.md`

Workspace guardrails:
- Treat `m-rewrite/` as the only active codebase for edits/builds/git operations.
- Use `./abuild.py -c -d <build-dir>` for delegated configure/build/test flows (omit `-c` only when intentionally reusing a configured build dir).
- Use named build-slot ownership for delegated work:
  - specialists must run with an explicit agent identity (`ABUILD_AGENT_NAME` or `./abuild.py --agent <name>`),
  - specialists claim assigned `build-a*` slots with `./abuild.py --claim-lock -d <build-dir>`,
  - specialists release slots with `./abuild.py --release-lock -d <build-dir>` when retiring/transferring ownership.
- In parallel work, use isolated build dirs and explicit wrapper build-dir args.
- Local `./vcpkg` is mandatory for delegated build/test work; specialists treat missing/unbootstrapped local `./vcpkg` as a blocker and escalate to manager/human.
- Use `demo/` as the canonical tracked test-data root for local fixtures/state:
  - communities: `demo/communities/*`
  - user homes/config state: `demo/users/*`
  - world fixtures: `demo/worlds/*`
- Do not create durable test fixtures/state under personal `~/.config/bz3` or ad-hoc `/tmp` paths.
