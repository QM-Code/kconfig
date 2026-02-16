# Projects Folder Guidance

`docs/projects/` contains transient, discardable execution tracks.

Durable policy/governance/architecture belongs in `docs/foundation/`.

## Agent Workflow
Bootstrap workflow (first packet, explicit `refresh bootstrap`, ownership change, or after context compaction):
1. Read `AGENTS.md`.
2. Read `docs/foundation/policy/execution-policy.md`.
3. Read `docs/projects/ASSIGNMENTS.md` to identify active project docs and ownership.
4. Work from one selected `docs/projects/<project>.md`.
5. Update that project doc and `docs/projects/ASSIGNMENTS.md` in the same handoff.

Delta workflow (same specialist session, no bootstrap refresh trigger):
1. Use standing bootstrap context from the active specialist session.
2. Read `docs/projects/ASSIGNMENTS.md` and selected `docs/projects/<project>.md`.
3. Execute only the bounded next slice and update docs in the same handoff.

## Build/Validation Policy
- Canonical build/validation policy is in `docs/foundation/policy/execution-policy.md`.
- Do not restate or fork that policy in project docs; link to it.

## Scope Rules
- Keep project docs project-specific and transient.
- Do not duplicate long-lived rules from `docs/foundation/*`.
- Completed tracks should be moved to `docs/archive/`.
