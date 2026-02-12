# Engine Defaults Program

This document tracks long-lived architecture rollout posture for defaults-model adoption.

Canonical contracts:
- `docs/foundation/architecture/engine-defaults-model.md`
- `docs/foundation/architecture/core-engine-contracts.md`

Transient execution state:
- `docs/projects/core-engine-infrastructure.md`
- subsystem project docs in `docs/projects/`

## Program Objectives
1. Keep `95% defaults / 5% overrides` policy enforceable in code and tests.
2. Preserve engine-owned boundaries while expanding capability parity.
3. Ensure staged implementation order remains explicit and reviewable.

## Canonical Adoption Sequence
1. Scheduler core contract.
2. Component catalog core contract.
3. Physics core contract, then physics facade/override boundaries.
4. Audio core contract, then audio facade/override boundaries.

Authoritative stage definitions, tests, and handoff evidence are maintained in:
- `docs/foundation/architecture/core-engine-contracts.md`

## Current Program Posture
- Scheduler/component/physics/audio staged contract definitions are codified.
- Ongoing execution is tracked in transient project docs, not in this foundation file.
- Any change to staged contract semantics must be made in foundation docs first, then reflected in active project tasks.

## Change Control Rules
- Do not put per-slice owner/status/handoff logs in this file.
- Do not duplicate full contract text from `core-engine-contracts.md`.
- Do not reference archived project paths as active guidance.

## Open Questions
- Should component metadata live docs-first, code-adjacent, or both?
- Which advanced capability gates should be standardized first after core closure?
