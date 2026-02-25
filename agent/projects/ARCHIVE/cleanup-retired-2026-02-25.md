# Cleanup Superproject (`m-karma/src` + `m-bz3/src`)

## Project Snapshot
- Current owner: `overseer`
- Status: `complete (all cleanup lanes closed by explicit decision)`
- Immediate next task: `none (superproject closed; spawn new lane only for new cleanup scope)`.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`.

## Mission
Coordinate cross-repo cleanup/refactor work across `m-karma/src` and `m-bz3/src` through independent, parallelizable subprojects with clear ownership boundaries and shared acceptance criteria.

## Foundation References
- `projects/cleanup/renderer-backend-core-decomposition.md`
- `../docs/building.md`
- `../docs/testing.md`

## Why This Is Separate
Cleanup is cross-cutting and spans runtime behavior, build wiring, naming, tests, and architecture boundaries in multiple repos. A parent orchestration track is required to keep high-value work parallel without drifting contracts.

## Active Subproject Map
- None (all cleanup child lanes closed).

## Parallelization Lanes
1. None (closeout state).

## Interface Boundaries
- Inputs consumed:
  - child-track status, blockers, and acceptance evidence.
  - cross-repo build/test constraints from `../docs/building.md` and `../docs/testing.md`.
- Outputs exposed:
  - priority and sequencing lock across child tracks.
  - shared acceptance gates for merge readiness.
- Coordinate before changing:
  - `projects/ASSIGNMENTS.md`
  - all `projects/cleanup/*.md` files

## Milestones
### C0: Superproject Conversion (this slice)
- convert monolithic cleanup plan into parent + child docs.
- acceptance:
  - initial cleanup lanes split into independent child docs with assignment tracking.

### C1: Highest-Value Execution
- execute `CLN-S2`, then `CLN-S9` in parallel lanes.
- acceptance:
  - at least two independent lanes show validated progress.

### C2: Cross-Lane Standardization
- align factory behavior contracts, test harness strategy, and naming policy.
- acceptance:
  - no cross-lane blocker caused by naming/API drift.

### C3: Closeout
- remaining active lanes integrated or closed by explicit decision.
- acceptance:
  - child docs resolved and parent marked complete.

## Non-Goals
- Do not place implementation detail for every child track in this parent doc.
- Do not allow child tracks to silently redefine shared contracts.
- Do not merge partially validated cleanup slices that regress parity/test gates.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

## Trace Channels
- `cleanup.plan`
- `cleanup.server`
- `cleanup.physics`
- `cleanup.renderer`
- `cleanup.factory`
- `cleanup.tests`

## Current Status
- `2026-02-21`: deep-dive analysis completed and `CLN-S1..CLN-S10` backlog defined.
- `2026-02-21`: source-tree CMake ownership anti-pattern removed (`src/game` and `src/engine` build wiring relocation).
- `2026-02-22`: cleanup program converted into parent/child superproject architecture.
- `2026-02-22`: `CLN-S1` and `CLN-S7` UI-focused child docs removed from cleanup scope by operator direction; active UI work continues under `projects/ui.md` lanes.
- `2026-02-22`: `CLN-S3` advanced from extraction to placement decision; `path_utils` remains internal and `S3-3` contract-test slice is next.
- `2026-02-22`: `CLN-S3` `S3-3` contract tests landed (`data_path_contract_test`) and validated; next `CLN-S3` action is `S3-4` canonicalization dedupe follow-on.
- `2026-02-22`: `CLN-S3` `S3-4` targeted canonicalization dedupe landed (`audio/*`, `root_policy`, `cli/server/runtime_options`) and validated; next `CLN-S3` action is `S3-5` closeout decision for `directory_override`.
- `2026-02-22`: `CLN-S3` `S3-5` decision landed by migrating `src/common/data/directory_override.cpp` canonicalization to shared `path_utils::Canonicalize`; validated via `./abuild.py -c -d build-cln-s3`, `ctest --test-dir build-cln-s3 -R "data_path_contract_test" --output-on-failure`, and `./scripts/test-engine-backends.sh build-cln-s3`. CLN-S3 implementation slices are now closed.
- `2026-02-22`: `CLN-S3` lane marked complete in project tracking (`ASSIGNMENTS.md`) before closeout handoff sequencing.
- `2026-02-22`: `CLN-S3` closeout completed and removed from active assignment tracking.
- `2026-02-22`: `CLN-S8` `S8-1` landed by introducing shared selector utility `src/common/backend/selector.hpp` and migrating `src/audio/backend_factory.cpp` as reference usage; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: `CLN-S8` `S8-2` landed by migrating `src/physics/backend_factory.cpp` to shared selector helpers in `src/common/backend/selector.hpp`; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: `CLN-S8` `S8-3` landed by migrating `src/window/backend_factory.cpp` to shared selector helpers in `src/common/backend/selector.hpp`; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: `CLN-S8` `S8-4` landed by migrating `src/ui/backend_factory.cpp` to shared selector helpers in `src/common/backend/selector.hpp` while preserving `software-overlay` alias parsing; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: `CLN-S8` `S8-5` landed by migrating `src/renderer/backend_factory.cpp` to shared selector helpers in `src/common/backend/selector.hpp` while preserving `backend->isValid()` acceptance semantics; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`.
- `2026-02-22`: `CLN-S8` `S8-6` landed by adding `backend_selector_contract_test` (`src/common/tests/backend_selector_contract_test.cpp`) and wiring `cmake/sdk/tests.cmake` test registration; validated via `./abuild.py -c -d build-cln-s8`, `./scripts/test-engine-backends.sh build-cln-s8`, and `ctest --test-dir build-cln-s8 -R "backend_selector_contract_test|physics_backend_parity_.*|audio_backend_smoke_.*" --output-on-failure`. CLN-S8 rollout is now closed behavior-neutrally.
- `2026-02-22`: `CLN-S8` closeout completed and removed from active assignment tracking.
- `2026-02-22`: `CLN-S4` closeout completed and removed from active assignment tracking.
- `2026-02-23`: `CLN-S9` advanced from queued prep to active execution: `m-bz3` shared test harness modules landed (`src/tests/support/test_harness.hpp`, `src/tests/support/loopback_harness.hpp`), broad test migration validated (`ctest` `30/30`), and next step is mirrored `m-karma` parity/gating follow-on.
- `2026-02-23`: `CLN-S9` parity follow-on landed in `m-karma` (`src/tests/support/test_harness.hpp` + targeted suite migrations), validated with targeted contract tests (`5/5`), leaving only closeout decisions (client transport diagnostics helper exception + m-bz3 loopback target-gating policy).
- `2026-02-23`: `CLN-S9` closeout decisions documented (loopback gating rationale in `m-bz3/cmake/targets/tests.cmake`; client transport diagnostics-helper exception in `m-karma/src/network/tests/contracts/client_transport_contract_test.cpp`); lane is now closeout-ready pending final closeout decision.
- `2026-02-23`: `CLN-S9` closeout completed by operator decision not to export `network_test_support` through Karma SDK targets; lane removed from active assignment tracking.
- `2026-02-23`: `CLN-S10` closeout completed by operator direction and removed from active assignment tracking.
- `2026-02-25`: `CLN-S2` closeout completed after landing server runtime O(1) actor/session indexing improvements and authoritative actor drift removal with passing server/runtime validation gates.
- `2026-02-25`: `CLN-S6` activated from queued state under owner `codex-cln-s6`; first decomposition slice landed in `m-karma` by splitting BGFX/Diligent backend `core.cpp` implementations into `core.cpp`, `lifecycle.cpp`, and `frame.cpp` units with successful BGFX + Diligent builds and passing directional-shadow contract binaries.
- `2026-02-25`: `CLN-S6` `S6-2` follow-on landed by extracting backend shadow resource/cache helpers into dedicated `shadow_resources.cpp` units for BGFX + Diligent, with both renderer profiles rebuilding cleanly and directional shadow contract binaries passing; backend smoke gate remains blocked by existing non-renderer `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-3` landed by moving frame-local submission/uniform assembly into dedicated backend `submission.cpp` units and reducing `frame.cpp` to orchestration flow; overlapping partial `frame_uniforms.cpp` branch work was removed to keep one coherent split strategy.
- `2026-02-25`: `CLN-S6` `S6-4` landed by moving backend layer renderable/shadow-caster collection out of `frame.cpp` into dedicated `collection.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-5` landed by moving backend local-light/point-shadow packing out of `submission.cpp` into dedicated `lighting.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-6` landed by moving backend directional-shadow cascade/uniform packing out of `submission.cpp` into dedicated `shadow_uniforms.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-7` landed by moving render-submission input assembly out of backend `submission.cpp` files into dedicated `submission_input.cpp` seams for BGFX + Diligent; directional-shadow contract binaries passed for both renderer profiles, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-8` landed by moving point-shadow status/refresh trace emission out of backend `submission.cpp` files into dedicated `submission_trace.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-9` landed by moving submission-layer state preparation out of backend `submission.cpp` files into dedicated `submission_state.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-10` landed by moving render-submission dispatch out of backend `submission.cpp` files into dedicated `submission_dispatch.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-11` landed by removing thin backend `submission.cpp` wrappers and connecting `frame.cpp` directly to extracted submission seams (`buildLayerSubmissionState`, `dispatchRenderSubmission`) for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-12` landed by moving frame-level layer draw execution out of backend `frame.cpp` files into dedicated `frame_layer.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-13` landed by moving frame pre-draw setup out of backend `frame.cpp` files into dedicated `frame_view.cpp`/`frame_target.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-14` landed by moving frame-layer context assembly out of backend `frame.cpp` files into dedicated `frame_context.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-15` landed by moving frame-layer directional-shadow cache/update flow out of backend `frame_layer.cpp` files into dedicated `frame_shadow.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-16` landed by moving frame-layer submission orchestration out of backend `frame_layer.cpp` files into dedicated `frame_submit.cpp` seams for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-17` landed by moving backend frame-layer render-target/viewport binding out of `frame_layer.cpp` into dedicated seams (`prepareLayerRenderTargetState`) for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-18` landed by moving backend frame-layer readiness guards out of `frame.cpp` into dedicated `frame_ready.cpp` seams (`canRenderLayer`) for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-19` landed by collapsing backend frame-layer draw execution entrypoints to consume unified layer-context seams directly (reducing frame-to-layer argument fanout) for BGFX + Diligent; both renderer profiles rebuilt successfully and directional-shadow contract binaries passed, while backend smoke remains blocked by unchanged `physics_backend_parity_jolt`.
- `2026-02-25`: `CLN-S6` `S6-20` closeout-readiness audit landed with backend `core.cpp`/`frame.cpp` boundaries reduced to thin orchestration/API surfaces and extracted seams owning prior complexity; validation remains stable with passing renderer contract binaries and unchanged external `physics_backend_parity_jolt` smoke blocker.
- `2026-02-25`: `CLN-S6` `S6-CLOSE` decision landed by operator direction; renderer core decomposition lane is closed as complete with remaining `physics_backend_parity_jolt` smoke failure tracked as an external physics parity blocker.
- `2026-02-25`: Cleanup superproject closeout completed (`C3`) after confirming no remaining active cleanup child lanes.

## Open Questions
- None.

## Handoff Checklist
- [x] Child docs stay aligned with parent sequencing.
- [x] Assignment board tracks every active cleanup child project.
- [x] Cross-lane blockers are recorded and routed.
- [x] Parent status reflects actual child execution state.
