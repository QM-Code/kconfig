# Specialist Validation Matrix

Purpose:
- define required validation per touch scope with minimal ambiguity.

## Build Command Baseline
Use delegated wrapper build command:
- `./abuild.py -c -d <build-dir>`

Add backend selectors only when required by scope:
- renderer: `-b bgfx,diligent`
- ui: `-b imgui,rmlui`
- physics: `-b jolt,physx`

## Required Gates By Touch Scope

| Touch scope | Required validation |
|---|---|
| Network / transport / protocol / server runtime | `./scripts/test-server-net.sh <build-dir>` |
| Physics / audio / backend test registration | `./scripts/test-engine-backends.sh <build-dir>` |
| Cross-scope (network + physics/audio/backend) | Run both wrappers with explicit `<build-dir>` |
| Community webserver/auth paths | Run community runbook checks in `docs/specialist/community-runbook.md` plus wrapper(s) if engine/game code also changed |
| Docs-only / project-tracking only | Wrapper gates optional unless project doc explicitly requires them |

## Wrapper Invocation Rules
- In parallel delegated work, always pass explicit build-dir args.
- Avoid relying on wrapper defaults (`build-dev`) during concurrent specialist work.

## Evidence Format
For each required gate, report:
1. exact command,
2. pass/fail outcome,
3. rerun notes if flaky behavior observed.
