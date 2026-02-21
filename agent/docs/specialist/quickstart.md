# Specialist Quickstart

Purpose:
- give coding specialists the minimum required instructions to execute one slice safely.

## Read Order
1. `docs/specialist/quickstart.md` (this file)
2. `docs/specialist/validation-matrix.md`
3. `projects/ASSIGNMENTS.md`
4. assigned `projects/<project>.md`
5. optional: `docs/overseer/execution-policy.md` for policy details

## Start-of-Slice Checklist
1. Confirm assigned project, scope, and owned paths.
2. Set identity: `export ABUILD_AGENT_NAME=<agent-name>`.
3. Claim assigned slot(s): `./abuild.py --claim-lock -d <build-dir>`.
4. Run default configure/build command for assigned dir:
- `./abuild.py -c -d <build-dir>`

## Execution Rules
- Use only assigned build dirs.
- Use `abuild.py` for delegated build/test flows.
- Keep changes inside project scope and owned paths.
- Keep backend-specific details out of game-facing API surfaces.
- Treat missing local `./vcpkg` as blocker and escalate.

## Validation Rules
- Use `docs/specialist/validation-matrix.md` to choose required commands by touch scope.
- In parallel work, always pass explicit build-dir args to wrappers.

## Required Docs Updates Before Handoff
- Update assigned project snapshot/status fields.
- Update `projects/ASSIGNMENTS.md` row (`Owner`, `Status`, `Next Task`, `Last Update`) when changed.

## Handoff Must Include
- files changed,
- exact commands run + outcomes,
- remaining risks/open questions.

## End-of-Slice
- Release lock when retiring/transferring slot:
  - `./abuild.py --release-lock -d <build-dir>`
