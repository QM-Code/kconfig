# Cleanup S6 (`CLN-S6`): Renderer Backend Core Decomposition

## Project Snapshot
- Current owner: `unassigned`
- Status: `queued`
- Immediate next task: split remaining heavy BGFX/Diligent core flows into init/resource/shadow/submission units and isolate backend-agnostic seams.
- Validation gate: `cd m-karma && ./scripts/test-engine-backends.sh <karma-build-dir>`.

## Mission
Decompose renderer backend cores into clearer architecture layers while preserving backend parity and render-contract behavior.

## Foundation References
- `projects/cleanup.md`
- `m-karma/src/renderer/backends/bgfx/core.cpp`
- `m-karma/src/renderer/backends/diligent/core.cpp`
- `m-karma/src/renderer/backends/bgfx/*`
- `m-karma/src/renderer/backends/diligent/*`

## Why This Is Separate
Large renderer core decomposition can run independently from server/runtime and most physics cleanup tracks.

## Owned Paths
- `m-karma/src/renderer/backends/bgfx/*`
- `m-karma/src/renderer/backends/diligent/*`
- `m-overseer/agent/projects/cleanup/renderer-backend-core-decomposition.md`

## Interface Boundaries
- Inputs consumed:
  - current renderer device/backend contract requirements.
- Outputs exposed:
  - cleaner backend internals and reduced compile fanout.
- Coordinate before changing:
  - `projects/lighting.md`
  - `projects/ui/radar.md`

## Non-Goals
- Do not redesign renderer public API in this slice.
- Do not reduce directional/point shadow contract coverage.

## Validation
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
./scripts/test-engine-backends.sh <karma-build-dir>
ctest --test-dir <karma-build-dir> -R "directional_shadow_contract_test" --output-on-failure
```

## Trace Channels
- `render.system`
- `render.bgfx`
- `render.diligent`
- `cleanup.s6`

## Build/Run Commands
```bash
cd m-karma
./abuild.py -c -d <karma-build-dir>
```

## Current Status
- `2026-02-21`: identified as `P1` decomposition target.
- `2026-02-22`: moved under cleanup superproject child structure.

## Open Questions
- Which shared helper seams should be promoted first without regressing backend-specific optimization paths?
- Should header inlining reductions be bundled or tracked as follow-on micro-slices?

## Handoff Checklist
- [ ] Core decomposition completed with coherent module boundaries.
- [ ] Backend parity smoke/tests remain green.
- [ ] No renderer contract regressions in downstream consumers.
