# Cleanup S6 (`CLN-S6`): Renderer Backend Core Decomposition

## Project Snapshot
- Current owner: `codex-cln-s6`
- Status: `complete (closed 2026-02-25 after S6-CLOSE decision)`
- Immediate next task: `none (lane closed; follow-on non-core decomposition should be queued as a new lane if needed)`.
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
- `2026-02-25`: naming/layout decision applied: no `core_` prefixes; backend directories now use flat unit names (`core.cpp`, `lifecycle.cpp`, `frame.cpp`).
- `2026-02-25`: `S6-1` landed in `m-karma` for both BGFX and Diligent by moving class declarations to `internal.hpp`, keeping `core.cpp` on API/factory paths, and splitting lifecycle/frame execution into `lifecycle.cpp` and `frame.cpp`.
- `2026-02-25`: validation evidence captured via `./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6 --configure`, `./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6-diligent --configure -b diligent`, and direct execution of `build-codex-cln-s6/directional_shadow_contract_test` plus `build-codex-cln-s6-diligent/directional_shadow_contract_test` (both exit `0`). Backend smoke script currently reports a pre-existing `physics_backend_parity_jolt` failure and remains an open non-renderer blocker for full gate closure.
- `2026-02-25`: `S6-2` landed by adding dedicated backend state units (`bgfx/state.cpp`, `diligent/state.cpp`) and moving shadow/resource state wiring out of `frame.cpp`/`lifecycle.cpp`; Diligent pipeline wrapper methods were moved from `lifecycle.cpp` into `pipeline.cpp` to align lifecycle/pipeline boundaries.
- `2026-02-25`: `S6-2` validation rerun passed for both renderer profiles (`./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6 --configure`, `./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6-diligent --configure -b diligent`) and directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` still fails on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-2` follow-on extraction landed by moving dense shadow texture/cache helpers from backend `shadow.cpp` files into dedicated `shadow_resources.cpp` units (`Ensure*ShadowTexture`, `Update*ShadowTexture`, point-shadow cache reset helpers, and `ResetDiligentShadowResources`), with declarations promoted to backend `internal.hpp` and build wiring updated in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-3` landed by making backend `frame.cpp` files orchestration-only and moving per-layer submission/uniform assembly into dedicated units (`bgfx/submission.cpp`, `diligent/submission.cpp`), with build wiring updated in `cmake/sdk/sources.cmake`.
- `2026-02-25`: conflict resolution completed after overlapping parallel edits by removing the alternate in-progress `frame_uniforms.cpp` path and normalizing to a single split strategy (`frame.cpp` orchestration + `submission.cpp` implementation) for both BGFX and Diligent.
- `2026-02-25`: `S6-3` validation rerun passed for both renderer profiles (`./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6 --configure`, `./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6-diligent --configure -b diligent`) and directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` still fails on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-4` landed by extracting backend layer renderable/shadow-caster collection from `frame.cpp` into dedicated units (`bgfx/collection.cpp`, `diligent/collection.cpp`) backed by new private backend seams (`collectLayerDraws`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-4` validation rerun passed for both renderer profiles (`./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6 --configure`, `./abuild.py --agent codex-cln-s6 -d build-codex-cln-s6-diligent --configure -b diligent`) and directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-5` landed by extracting backend local-light and point-shadow packing/update logic from `submission.cpp` into dedicated units (`bgfx/lighting.cpp`, `diligent/lighting.cpp`) backed by new private backend seams (`buildLocalLightPointShadowState`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-5` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-6` landed by extracting backend directional-shadow cascade/uniform packing from `submission.cpp` into dedicated units (`bgfx/shadow_uniforms.cpp`, `diligent/shadow_uniforms.cpp`) backed by new private backend seams (`buildDirectionalShadowSubmissionState`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-6` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-7` landed by extracting render-submission input population from backend `submission.cpp` into dedicated units (`bgfx/submission_input.cpp`, `diligent/submission_input.cpp`) backed by new private backend seams (`populateRenderSubmissionInput`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-7` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-8` landed by extracting point-shadow status/refresh trace emission from backend `submission.cpp` into dedicated units (`bgfx/submission_trace.cpp`, `diligent/submission_trace.cpp`) backed by new private backend seams (`tracePointShadowStatus`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-8` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-9` landed by extracting backend submission-layer state preparation (directional/local state assembly + point-shadow trace trigger) from `submission.cpp` into dedicated units (`bgfx/submission_state.cpp`, `diligent/submission_state.cpp`) backed by new private backend seams (`buildLayerSubmissionState`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-9` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-10` landed by extracting backend render-submission dispatch (render-input assembly invocation + final draw submission call) from `submission.cpp` into dedicated units (`bgfx/submission_dispatch.cpp`, `diligent/submission_dispatch.cpp`) backed by new private backend seams (`dispatchRenderSubmission`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-10` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-11` landed by retiring thin backend `submission.cpp` wrappers after prior seam extraction; `frame.cpp` now orchestrates `buildLayerSubmissionState` + `dispatchRenderSubmission` directly for BGFX + Diligent, with `submission.cpp` removed from backend source lists in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-11` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-12` landed by extracting frame-level layer draw execution (shadow cache/update pass + submission state/dispatch orchestration) from backend `frame.cpp` into dedicated units (`bgfx/frame_layer.cpp`, `diligent/frame_layer.cpp`) backed by new private backend seams (`renderLayerDraws`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-12` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-13` landed by extracting frame pre-draw setup from backend `frame.cpp` into dedicated units (`bgfx/frame_view.cpp`, `diligent/frame_target.cpp`) backed by new private backend seams (`prepareMainViewTransform`, `prepareMainRenderTargets`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-13` validation rerun passed for both renderer profiles via incremental rebuilds of `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, plus directional-shadow contract binaries in both build dirs; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-14` landed by extracting frame-layer context assembly from backend `frame.cpp` into dedicated units (`bgfx/frame_context.cpp`, `diligent/frame_context.cpp`) backed by new private backend seams (`buildLayerFrameContext`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-14` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-15` landed by extracting frame-layer directional-shadow cache/update flow from backend `frame_layer.cpp` into dedicated units (`bgfx/frame_shadow.cpp`, `diligent/frame_shadow.cpp`) backed by new private backend seams (`buildLayerShadowContext`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-15` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-16` landed by extracting frame-layer submission orchestration from backend `frame_layer.cpp` into dedicated units (`bgfx/frame_submit.cpp`, `diligent/frame_submit.cpp`) backed by new private backend seams (`submitLayerRenderables`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-16` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-17` landed by extracting backend final render-target/viewport binding from `frame_layer.cpp` into dedicated backend seams (`prepareLayerRenderTargetState` in BGFX + Diligent), with Diligent binding logic moved under `frame_target.cpp` and BGFX aligned via an explicit no-op seam.
- `2026-02-25`: `S6-17` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-18` landed by extracting render-layer readiness guards from backend `frame.cpp` into dedicated units (`bgfx/frame_ready.cpp`, `diligent/frame_ready.cpp`) backed by new private backend seams (`canRenderLayer`) and shared build wiring updates in `cmake/sdk/sources.cmake`.
- `2026-02-25`: `S6-18` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-19` landed by collapsing frame-layer draw execution signatures to consume unified per-layer context seams (`BgfxLayerFrameContext`, `DiligentLayerFrameContext`) directly, reducing frame-to-layer argument fanout while preserving orchestration boundaries.
- `2026-02-25`: `S6-19` validation rerun passed for both renderer profiles via directional-shadow contract binaries in `build-codex-cln-s6` and `build-codex-cln-s6-diligent`; `./scripts/test-engine-backends.sh build-codex-cln-s6` continues to fail on unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-20` closeout-readiness audit completed: backend `core.cpp` files are now API/factory-facing only and backend `frame.cpp` files are thin orchestration entrypoints (`canRenderLayer` + main-target prep + context build + layer dispatch), with extracted frame/submission seams owning prior core-frame complexity.
- `2026-02-25`: `S6-20` validation evidence remains stable with successful incremental builds for `build-codex-cln-s6` and `build-codex-cln-s6-diligent`, passing directional-shadow contract binaries in both build dirs, and unchanged external smoke blocker `physics_backend_parity_jolt`.
- `2026-02-25`: `S6-CLOSE` decision applied by operator direction: lane marked complete because core/frame decomposition goals are met and remaining smoke blocker (`physics_backend_parity_jolt`) is a physics parity test failure outside renderer decomposition scope.

## Open Questions
- None; optional further splitting work (`pipeline.cpp`/`shadow.cpp`/`render.cpp` compaction) is explicitly out-of-scope for this closed lane.

## Handoff Checklist
- [x] Core decomposition completed with coherent module boundaries.
- [ ] Backend parity smoke/tests remain green (`physics_backend_parity_jolt` remains an external blocker outside this lane).
- [x] No renderer contract regressions in downstream consumers.
