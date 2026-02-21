# Overseer Execution Policy

Purpose:
- define canonical delegated execution mechanics,
- keep build/validation behavior deterministic across specialists.

## Execution Root
- Run commands from the assigned repo root for the active project.
- If launching from workspace root, use explicit repo prefixes or `cd` first.

## Build Policy
- Use `./abuild.py -c -d <build-dir>` for delegated configure/build/test flows.
- Omit `-c` only when intentionally reusing a configured build dir.
- Do not use raw `cmake -S/-B` for delegated specialist work.

Default-first backend rule:
- Most slices should omit `-b`.
- Use `-b` only for categories touched by the slice.

Examples:
- default: `./abuild.py -c -d <build-dir>`
- renderer runtime-select: `./abuild.py -c -d <build-dir> -b bgfx,diligent`
- ui runtime-select: `./abuild.py -c -d <build-dir> -b imgui,rmlui`
- physics runtime-select: `./abuild.py -c -d <build-dir> -b jolt,physx`
- static SDK contract: `./abuild.py -c -d <build-dir> --sdk-linkage static --install-sdk <prefix>`
- mobile shared override (explicit-only): `./abuild.py -c -d <build-dir> --sdk-linkage shared --mobile-allow-shared`

Renderer policy:
- Combined renderer mode (`-b bgfx,diligent`) is Linux shared-mode only.
- Non-Linux targets and static SDK linkage must select one renderer backend.

## Build Slot Ownership
Required in parallel delegated work:
- set agent identity: `export ABUILD_AGENT_NAME=<agent-name>`
- claim slot before first build: `./abuild.py --claim-lock -d <build-dir>`
- release on retire/transfer: `./abuild.py --release-lock -d <build-dir>`

Rules:
- use only assigned `build-*` slots,
- pass explicit build-dir args to wrapper scripts,
- `--ignore-lock` is emergency-only and requires overseer approval.

## Toolchain Policy
- Local repo `./vcpkg` is mandatory for delegated builds.
- Missing/unbootstrapped local `./vcpkg` is a blocker; escalate instead of improvising external toolchains.

## Validation Gates
Core wrappers:
- `./scripts/test-engine-backends.sh <build-dir>`
- `./scripts/test-server-net.sh <build-dir>`

Touch-scope rules:
- network/transport/protocol scope -> server-net wrapper required.
- physics/audio/backend-test wiring scope -> engine-backends wrapper required.
- SDK packaging/runtime scope (Linux) -> `m-karma/scripts/test-sdk-runtime-linux.sh <sdk-prefix> [consumer-bin ...]` required.
- SDK packaging/runtime scope (macOS) -> `m-karma/scripts/test-sdk-runtime-macos.sh <sdk-prefix> [consumer-bin ...]` required.
- SDK packaging/runtime scope (Windows) -> `m-karma/scripts/test-sdk-runtime-windows.sh <sdk-prefix> [consumer-bin ...]` required.
- SDK packaging/policy scope (mobile static contract) -> `m-karma/scripts/test-sdk-mobile-static.sh <sdk-prefix>` required.
- cross-scope changes -> run both wrappers.
- docs-only/project-tracking edits -> wrappers optional unless project doc says otherwise.

## CI Gate Bring-Up (MP5)
Required-check target set (after first green stabilization):
- `SDK Packaging Matrix / SDK Desktop (ubuntu-latest)` in `m-karma`
- `SDK Packaging Matrix / SDK Desktop (macos-latest)` in `m-karma`
- `SDK Packaging Matrix / SDK Desktop (windows-latest)` in `m-karma`
- `SDK Packaging Matrix / SDK Mobile Policy Contract` in `m-karma`
- `SDK Consumer Smoke / Consumer Desktop (ubuntu-latest)` in `m-bz3`
- `SDK Consumer Smoke / Consumer Desktop (macos-latest)` in `m-bz3`
- `SDK Consumer Smoke / Consumer Desktop (windows-latest)` in `m-bz3`

First-run CI triage order:
1. Run Linux legs first (`ubuntu-latest`) in both repos.
2. Fix toolchain/bootstrap issues before touching runtime gate logic.
3. Run `macOS` and `Windows` legs one at a time and capture per-leg artifacts.
4. If host-specific runtime scripts fail, compare dependency output (`ldd`/`otool`/`dumpbin|objdump`) from uploaded artifacts before changing packaging rules.
5. Promote jobs to required only after two consecutive green runs on both `pull_request` and branch push triggers.

## Demo Fixture Policy
Reusable local test/demo state must live under tracked `demo/` roots:
- `demo/communities/*`
- `demo/users/*`
- `demo/worlds/*`

Avoid relying on personal `~/.config/...` or ad-hoc `/tmp` state for durable workflows.

## KARMA -> BZ3 SDK Contract (Locked)
- `m-karma` build graph is SDK-only; package/export generation is always configured (no non-SDK mode toggle).
- Consumer integration path is package-based only:
  - `find_package(KarmaSDK CONFIG REQUIRED)`
- Diligent renderer dependency is package-based in both repos:
  - `find_package(DiligentEngine CONFIG REQUIRED)`
  - source should come from repo overlay port `vcpkg-overlays/diligentengine` (no `FetchContent` fallback)
- Raw include/lib wiring from consumer into KARMA build artifacts is disallowed.

Canonical commands:
- producer (`m-karma`):
  - `./abuild.py -c -d build-sdk --install-sdk out/karma-sdk`
- consumer (`m-bz3`):
  - `./abuild.py -c -d build-sdk --karma-sdk ../m-karma/out/karma-sdk --ignore-lock`

Required imported targets:
- `karma::core`
- `karma::client`

## Handoff Minimum
Every specialist handoff must include:
- files changed,
- exact commands run + outcomes,
- open risks/questions,
- project-doc and assignment-row updates.
