# Testing/CI/Docs Governance

This document defines long-lived governance for validation wrappers, guard scripts, and CI alignment.

## Core Policy
- Operator-facing configure/build/test flows use `./bzbuild.py <build-dir>`.
- In parallel work, wrapper invocations must pass explicit build dirs:
  - `./scripts/test-engine-backends.sh <build-dir>`
  - `./scripts/test-server-net.sh <build-dir>`
- Wrapper defaults to `build-dev` are allowed only for serialized local checks and CI baseline runs.

## Validation Source of Truth
1. Engine backend gate:
   - `./scripts/test-engine-backends.sh <build-dir>`
2. Server/network gate:
   - `./scripts/test-server-net.sh <build-dir>`
3. Additional standalone guards/evidence scripts remain explicit unless integrated into wrappers.

## CI Baseline Contract
- `.github/workflows/core-test-suite.yml` should continue to invoke wrapper gates.
- If a new guard becomes required for acceptance, either:
  - integrate it into an existing wrapper, or
  - document explicit standalone invocation + CI posture in the same change.

## Drift Checklist
When changing wrappers/guards/CI:
1. Update this file.
2. Update any impacted project docs under `docs/projects/`.
3. Update `docs/foundation/policy/execution-policy.md` when policy wording changes.
4. Verify docs lint passes.

## Known Residual Risk Tracking
- Keep residual-risk summaries here only when they are cross-project governance concerns.
- Keep project-specific risk details in the active project file.
- Active cross-project note (`2026-02-12`):
  - `client_transport_contract_test` timeout-race coverage can be scheduler/load-sensitive in some runs (most visible under `./scripts/test-server-net.sh build-sdl3-bgfx-physx-rmlui-miniaudio`).
  - Required posture: rerun once immediately on isolated build dirs before escalation, and record both initial and rerun results in handoff evidence.

## Related Docs
- `docs/foundation/governance/engine-backend-testing.md`
- `docs/foundation/policy/execution-policy.md`
