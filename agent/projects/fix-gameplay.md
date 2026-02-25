# Gameplay Stabilization & Glitch Fixes (Localhost)

## Project Snapshot
- Current owner: `unassigned`
- Status: `on hold (paused by operator)`
- Immediate next task: wait for operator reactivation before dispatching `FG-S1`.
- Validation gate:
  - `m-overseer`: `./agent/scripts/lint-projects.sh`
  - `m-bz3`: `./abuild.py -c -d <bz3-build-dir> --karma-sdk ../m-karma/out/karma-sdk`, `./scripts/test-server-net.sh <bz3-build-dir>`, targeted `ctest`
  - Manual localhost smoke for touched behavior (server + two clients) before closing any stabilization slice.

## Mission
Keep the playable localhost loop that GP-S1..S5 established, and eliminate the remaining gameplay quality/runtime glitches blocking a dependable player experience.

## Foundation References
- `m-bz3/src/client/runtime/lifecycle.cpp`
- `m-bz3/src/client/runtime/tank_motion.cpp`
- `m-bz3/src/client/domain/tank_drive_controller.cpp`
- `m-bz3/src/client/net/connection/inbound/session_events.cpp`
- `m-bz3/src/client/net/connection/outbound.cpp`
- `m-bz3/src/server/runtime/server_game.cpp`
- `m-bz3/src/server/runtime/event_loop.cpp`
- `m-bz3/src/server/runtime/shot_pilot_step.cpp`

## Why This Is Separate
The prior `gameplay.md` track delivered required feature contracts (join/spawn, shot authority, score authority, ricochet, jump/landing) and reached retirement gating. This new track focuses on quality, feel, and visible runtime correctness of those features.

## Confirmed Baseline (Manual Runtime)
Manual localhost run reported on `2026-02-22`:
- First-person POV worked.
- World rendering looked correct.
- Rotation/look worked.
- Fire input triggered shot audio.
- Shot visual presence was not observed.
- Back/forward/jump movement felt glitchy/stuttery ("stop-motion" feel).
- HUD/score visibility was not available in the manual run.

## Stabilization Targets
1. Movement quality:
- Remove stutter/jitter in forward/backward/jump movement under client-server authority.

2. Shot visibility:
- Ensure authoritative shots are visibly present and traceable in client runtime when firing.

3. HUD/score visibility:
- Ensure score/HUD presentation is visible and verifiable in manual runtime.

4. Preserve delivered contracts:
- No regressions to explicit spawn gating, authoritative score model, or ricochet/jump server authority.

## Owned Paths
- `m-overseer/agent/projects/fix-gameplay.md`
- `m-overseer/agent/projects/ASSIGNMENTS.md`
- `m-bz3/src/client/runtime/*`
- `m-bz3/src/client/domain/*`
- `m-bz3/src/client/net/*`
- `m-bz3/src/server/runtime/*`
- `m-bz3/src/server/domain/*` (only if required by a bounded stabilization fix)
- `m-bz3/src/tests/*`

## Interface Boundaries
- Keep gameplay semantics in `m-bz3`.
- `m-karma` changes are allowed only if a strictly necessary generic seam blocker is proven.
- Preserve server authority for spawn, movement reconciliation, shot lifecycle, damage/death, and score mutation.

## Non-Goals
- No gameplay feature expansion beyond stabilization of existing outcomes `0`-`5`.
- No broad UI architecture rewrite.
- No transport protocol redesign beyond bounded bug fixes.

## Execution Plan (Stabilization Slices)
### `FG-S1` (first)
- Goal: deterministic reproduction + instrumentation of stutter/shot-visibility/HUD issues.
- Scope:
  - capture trace evidence for movement/jump stutter and shot visibility path.
  - verify whether HUD/score data exists but is hidden, or is missing upstream.
  - convert findings into ranked root-cause candidates and bounded fix plan.
- Acceptance:
  - reproducible symptom report with concrete traces and code-level suspect seams.

### `FG-S2`
- Goal: movement/jump quality stabilization.
- Scope:
  - fix jitter/stutter caused by client/server reconciliation cadence or state handoff.
- Acceptance:
  - noticeably stable movement/jump in manual localhost smoke with no authority regressions.

### `FG-S3`
- Goal: shot visibility correctness.
- Scope:
  - make fired shots visibly present while preserving server authority and existing hit logic.
- Acceptance:
  - manual smoke confirms visible shot lifecycle on fire events.

### `FG-S4`
- Goal: HUD/score visibility completion for runtime validation.
- Scope:
  - ensure score/HUD information is rendered and usable in manual verification.
- Acceptance:
  - manual smoke confirms visible, updating score/HUD state from authoritative messages.

### `FG-S5`
- Goal: polish + retirement gate prep.
- Scope:
  - close remaining high-impact gameplay quality gaps discovered in prior slices.
- Acceptance:
  - manual localhost outcomes `0`-`5` pass with acceptable feel and evidence.

## Validation
```bash
# m-bz3 required per implementation slice
cd m-bz3
export ABUILD_AGENT_NAME=specialist-fix-gameplay
./abuild.py --claim-lock -d <bz3-build-dir>
./abuild.py -c -d <bz3-build-dir> --karma-sdk ../m-karma/out/karma-sdk
KARMA_SDK_ROOT=../m-karma/out/karma-sdk ./scripts/test-server-net.sh <bz3-build-dir>

# targeted ctest packet (expand per stabilization scope)
ctest --test-dir <bz3-build-dir> -R "tank_.*|server_runtime_.*|server_net_contract_test|client_.*score.*|client_shot_reconciliation_test|shot_system_ricochet_test" --output-on-failure

./abuild.py --release-lock -d <bz3-build-dir>
```

## Manual Localhost Gate (Required)
1. Start server:
```bash
./<build-dir>/bz3-server --listen-port 11899
```
2. Start two clients:
```bash
./<build-dir>/bz3 --server 127.0.0.1:11899
./<build-dir>/bz3 --server 127.0.0.1:11899
```
3. Verify in one session:
- Join + explicit spawn behavior.
- First-person drive/jump feel (no visible stutter regression).
- Shot visibility + kill behavior.
- Server-authoritative score/HUD visibility and updates.
- Ricochet behavior against buildings.

## Current Status
- `2026-02-22`: Previous gameplay bring-up track closed after GP-S1..S5 completion.
- `2026-02-22`: New stabilization track created from manual runtime findings (shot visibility gap, movement/jump stutter, HUD visibility gap).
- `2026-02-25`: Work unassigned and paused by operator direction; no active gameplay stabilization slice is currently dispatched.

## Open Questions
- Is movement stutter primarily from reconciliation cadence, local prediction state resets, or render/update integration?
- Is shot invisibility a spawn/render/camera issue, a lifetime issue, or a presentation filtering issue?
- Is HUD hidden by config/state, or are score events not reaching the display layer in runtime conditions?

## Handoff Checklist
- [ ] Reproduction evidence captured
- [ ] Bounded fix implemented
- [ ] Validation commands run and summarized
- [ ] Manual localhost smoke executed for touched behavior
- [ ] Docs and assignments updated
- [ ] Risks/open questions listed
