# Platform Backend Policy

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued (P2 medium; ready to assign)`
- Immediate next task: execute Slice 1 (SDL3-only policy inventory + removal plan + guardrails) and update docs/assignments in one handoff.
- Validation gate: docs lint must pass for every slice; code-touching slices must also pass assigned `bzbuild.py` and wrapper gates.

## Mission
Keep an engine-owned platform seam while standardizing on SDL3 as the single active platform backend.

## Priority Directive (2026-02-11)
- This project is medium priority and should run behind active P0 renderer/network slices.
- The project must remove dormant platform backend complexity without collapsing the engine/platform seam.
- SDL3 remains the only active backend unless a concrete blocker justifies re-expansion.

## Primary Specs
- `AGENTS.md`
- `docs/AGENTS.md`
- `docs/projects/core-engine-infrastructure.md`
- `docs/projects/testing-ci-docs.md`
- `docs/projects/engine-backend-testing.md`

## Why This Is Separate
This is policy/boundary hardening work that can proceed without changing gameplay rules, network protocol semantics, or renderer feature slices.

## Owned Paths
- `docs/projects/platform-backend-policy.md`
- `docs/projects/core-engine-infrastructure.md` (platform/lifecycle policy references only)
- `m-rewrite/src/engine/platform/*`
- `m-rewrite/include/karma/platform/*`
- `m-rewrite/CMakeLists.txt` and `m-rewrite/src/engine/CMakeLists.txt` (platform backend wiring only)

## Interface Boundaries
- Engine/game code outside platform adapters must not consume SDL types directly.
- Keep one engine-facing platform contract; implementation is SDL3-only until a concrete second-backend requirement is approved.
- Coordinate before changing:
  - `docs/projects/core-engine-infrastructure.md`
  - `docs/projects/testing-ci-docs.md`
  - `docs/projects/engine-backend-testing.md`

## Non-Goals
- Do not implement GLFW support.
- Do not reintroduce SDL2 compatibility paths.
- Do not add speculative runtime backend switching/registry machinery.
- Do not move platform details into `src/game/*`.
- Do not widen scope into renderer/network/gameplay tracks.

## Validation
Docs-only slices (from repository root):

```bash
./docs/scripts/lint-project-docs.sh
```

Code-touching slices (from `m-rewrite/` with assigned build dir):

```bash
./bzbuild.py -c build-sdl3-bgfx-jolt-rmlui-sdl3audio
./scripts/test-engine-backends.sh build-sdl3-bgfx-jolt-rmlui-sdl3audio
```

## Trace Channels
- `engine.app`
- `engine.platform`

## Build/Run Commands
From `m-rewrite/`:

```bash
./bzbuild.py -c build-sdl3-bgfx-jolt-rmlui-sdl3audio
./scripts/test-engine-backends.sh build-sdl3-bgfx-jolt-rmlui-sdl3audio
```

## First Session Checklist
1. Read `AGENTS.md`, then `docs/AGENTS.md`, then this file.
2. Confirm SDL3-only policy and thin-abstraction constraint.
3. Implement exactly one slice from the queue below.
4. Validate with required commands.
5. Update this file and `docs/projects/ASSIGNMENTS.md` in the same handoff.

## Current Status
- `2026-02-11`: Project created and queued at medium priority.
- `2026-02-11`: Policy direction confirmed:
  - keep thin engine/platform abstraction seam,
  - keep SDL3 as only active backend,
  - remove dormant GLFW/SDL2 implementation and documentation references,
  - retain `bzbuild.py`/CMake scaffolding for future adapter reintroduction only if justified by concrete requirements.

## Slice Queue
1. Slice 1 (docs+inventory, no engine-code behavior changes):
   - Inventory all GLFW/SDL2 references across docs, CMake, vcpkg, and platform-related source.
   - Classify each reference as remove/retain-with-rationale.
   - Add a concrete removal plan and acceptance gate to this file.
   - Status: `Queued`.
2. Slice 2 (docs/build cleanup):
   - Remove stale GLFW/SDL2 references from docs and build wiring where no longer used.
   - Keep SDL3-only wiring explicit and deterministic.
   - Status: `Queued`.
3. Slice 3 (seam guardrails):
   - Add/strengthen checks to prevent SDL type leakage outside platform/backend boundaries.
   - Keep checks deterministic and fast for CI use.
   - Status: `Queued`.
4. Slice 4 (future reintroduction contract):
   - Codify requirements for adding a future second backend (e.g., SDL4/competitor) through contract + conformance tests, not speculative dormant code paths.
   - Status: `Queued`.

## Active Specialist Packet (Slice 1)
```text
Read in order:
1) AGENTS.md
2) docs/AGENTS.md
3) docs/projects/README.md
4) docs/projects/ASSIGNMENTS.md
5) docs/projects/platform-backend-policy.md

Take ownership of: docs/projects/platform-backend-policy.md

Goal:
- Produce an implementation-ready SDL3-only policy inventory/removal plan (Slice 1) without changing runtime behavior.

Scope:
- Enumerate current GLFW/SDL2 references in:
  - docs/,
  - m-rewrite/CMakeLists.txt,
  - m-rewrite/src/engine/CMakeLists.txt,
  - platform-related headers/sources under m-rewrite/include/karma/platform/* and m-rewrite/src/engine/platform/*.
- Add a concise reference table to this project file:
  - path/reference,
  - keep/remove decision,
  - rationale.
- Add explicit Slice 2 execution checklist derived from inventory findings.

Constraints:
- Do not implement GLFW/SDL2 functionality.
- Do not remove SDL3 seam abstractions.
- Do not touch renderer/network/gameplay paths.
- Use bzbuild.py only if you run build validation.

Validation (required):
- ./docs/scripts/lint-project-docs.sh

Docs updates (required):
- Update docs/projects/platform-backend-policy.md snapshot/status/slice queue.
- Update docs/projects/ASSIGNMENTS.md row (status/next task/last-update).

Handoff must include:
- files changed
- exact commands run + results
- inventory summary (keep/remove decisions)
- risks/open questions
- explicit next slice task
```

## Slice 1 Acceptance Gate (Must Pass)
- Inventory covers docs + build + platform-owned paths listed in scope.
- Each identified GLFW/SDL2 reference has explicit keep/remove decision with rationale.
- No runtime behavior changes are introduced.
- `./docs/scripts/lint-project-docs.sh` passes.
- `docs/projects/platform-backend-policy.md` and `docs/projects/ASSIGNMENTS.md` are both updated in the same handoff.

## Handoff Review Rubric (Overseer)
- `Accept` when Slice 1 acceptance gate is fully satisfied with explicit evidence.
- `Revise` when inventory is partial, rationale is missing, scope is widened, or docs updates are incomplete.
- Required evidence format:
  - file list with paths,
  - exact commands and outcomes,
  - concise keep/remove decision table summary,
  - explicit statement of what was intentionally not changed.

## Open Questions
- What is the minimal platform contract surface we want to freeze before any backend swap is reconsidered?
- Which tests should become mandatory conformance gates for any future SDL4 or competitor adapter?

## Handoff Checklist
- [ ] Slice scope completed
- [ ] Docs updated
- [ ] Validation run and summarized
- [ ] Build policy constraints preserved (`bzbuild.py` only)
- [ ] Risks/open questions listed
