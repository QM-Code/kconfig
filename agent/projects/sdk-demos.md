# SDK Demo Build Refactor

## Preparation

Read all of the following before making any changes. The list is intentionally explicit so a new contributor can reconstruct current behavior and dependencies.

- m-karma/CMakeLists.txt
- m-karma/examples/demo-sdk-consumer/CMakeLists.txt
- m-karma/examples/demo-sdk-consumer/README.md
- m-karma/examples/demo-sdk-consumer/src/client_help_main.cpp
- m-karma/examples/demo-sdk-consumer/src/server_help_main.cpp
- m-karma/src/demo/client/main.cpp
- m-karma/src/demo/client/runtime.cpp
- m-karma/src/demo/client/runtime.hpp
- m-karma/src/demo/net/protocol.cpp
- m-karma/src/demo/net/protocol.hpp
- m-karma/src/demo/server/main.cpp
- m-karma/src/demo/server/runtime.cpp
- m-karma/src/demo/server/runtime.hpp
- m-karma/src/demo/tests/protocol_contract_test.cpp

## Overview

We are consolidating SDK demo builds under `m-karma/demo/` in phases.

The intent is to absorb both:
- `examples/demo-sdk-consumer/` (minimal SDK-consumer sample), and
- `src/demo/` (current richer demo logic)

into one structured demo layout that is easier to navigate, easier to extend, and aligned with SDK-consumer-first build/test flows.

## Procudure

### Phase 1

Define the final tree and move immediately.

What this phase does:
- Establish the destination layout up front.
- Move current demo source and runtime fixture/state directories into that layout.
- Treat this as a direct move to end-state structure, not an intermediate compatibility pass.

Authoritative move mapping:
- `src/demo/* -> demo/full/src/*`
- `examples/demo-sdk-consumer/* -> demo/barebones/*`
- `demo/{servers,users,communities} + top-level data/ -> demo/full/runtime/{servers,users,communities,data}`

Target tree (current required + planned extension points):

Notes:
- Every demo subdirectory should include `CMakeLists.txt`, `README.md`, `build/`, `src/`, and `tests/`.

- `demo/`
- `barebones/`
  - `CMakeLists.txt`
  - `README.md`
  - `build/`
  - `src/`
  - `tests/`
- `cli/` (test cli up to handoff)
  - `CMakeLists.txt`
  - `README.md`
  - `build/`
  - `src/`
  - `tests/`
  - `runtime/`
    - `data/`
      - `server/`
        - `config.json`
      - `client/`
        - `config.json`
- `full/`
  - `CMakeLists.txt`
  - `README.md`
  - `build/`
  - `src/`
    - `client/`
    - `net/`
    - `server/`
    - `tests/`
  - `runtime/`
    - `data/`
    - `communities/`
    - `users/`
    - `servers/`

### Phase 2

Build model switch.

What this phase does:
- Switch demo build ownership from in-tree app wiring to SDK-consumer projects.
- Keep core SDK build behavior unchanged.
- Remove root-level `examples/` as part of root-tree cleanup.

Required changes:
- Remove in-tree demo app wiring from `cmake/sdk/apps.cmake`.
- Keep SDK build as-is.
- Build demos only as SDK consumers (`find_package(KarmaSDK)`) via:
  - `demo/full/CMakeLists.txt`
  - `demo/barebones/CMakeLists.txt`
- Do not keep an `examples/` shim for discoverability.
- Delete `examples/` entirely to reduce root-tree pollution.
- Update references/tests/docs to `demo/barebones` and `demo/full`.

### Phase 3

Remove old paths and dead references.

What this phase does:
- Clean out all references to the old layout once the new layout is in place.
- Ensure there is one canonical path model throughout build/test/docs.

Required changes:
- Delete `src/demo` usage from core CMake/tests.
- Replace `examples/demo-sdk-consumer` references with `demo/barebones` or `demo/full`.
- Update docs last.

### Phase 4

Rewrite tests to new demo outputs.

What this phase does:
- Retarget existing demo-related tests to run against SDK-consumer demo binaries produced from the new locations.
- Move fixture/data references to the new runtime hierarchy.

Required changes:
- Update smoke/trace scripts to run binaries from demo consumer build dirs (not `<build-dir>/client/server` from core build).
- Update fixture/data paths to `demo/full/runtime/...`.

Verification (lightweight only).

- Run shell syntax and path/reference checks (`bash -n`, `rg`) without full build.

### Phase 5

Implement the `demo/cli/` SDK build and define its scope.

What this phase does:
- Add and wire a dedicated SDK-consumer build for `demo/cli/`.
- Keep `demo/barebones/` as the minimal buildability proof only.
- Introduce a middle validation tier (`cli`) between barebones and full runtime demo behavior.

Scope definition:
- `barebones` confirms only that a binary can be built.
- `cli` must run through the CLI handoff boundary, specifically:
  - command-line parsing,
  - trace logging initialization,
  - config file loading.

Required changes:
- Implement `demo/cli/CMakeLists.txt` as an SDK-consumer build (`find_package(KarmaSDK)`).
- Add `demo/cli/src/` entry points that exercise startup through CLI handoff.
- Add or update `demo/cli/tests/` to validate the `cli` scope above.
- Keep `full` as the comprehensive runtime demo track.
