# Trace Logging UX Standardization (`m-karma` + `m-bz3`)

## Project Snapshot
- Current owner: `overseer`
- Status: `in progress (new trace ergonomics track)`
- Immediate next task: lock the explicit macro/API contract for filename handling and trace-prefix controls before touching broad callsites.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`

## Project Overview
This is a quality-of-life pass on trace logging. Nothing is catastrophically broken, but the current behavior relies on conventions that are easy to drift, and the CLI naming is a bit awkward.

Goal is to make trace output behavior explicit, predictable, and easy to control from startup flags without forcing everyone to remember unwritten formatting rules.

## Authoritative Goals
1. Make `KARMA_TRACE` take `filename` as an explicit argument so we do not rely on an unenforced standard.
2. Make the word inside `[trace]` configurable, sourced from the same runtime/binary-name variable used in `main.cpp` for early command-line feedback (before config files and logging setup are fully loaded).
3. Rename `--timestamp-logging` to `--trace-timestamps`.
4. Suppress trace filenames by default; enable filename display with `--trace-filenames`.
5. Add `--trace-no-prefix` to disable the initial `[trace]` prefix.

## Foundation References
- `m-karma/include/karma/common/logging/logging.hpp`
- `m-karma/src/common/logging/logging.cpp`
- `m-karma/src/app/shared/logging_setup.cpp`
- `m-karma/src/cli/shared/common_options.cpp`
- `m-karma/src/cli/client/parser.cpp`
- `m-karma/src/cli/server/parser.cpp`
- `m-bz3/src/main.cpp`
- `m-karma/src/main.cpp`

## Why This Is Separate
Trace UX touches shared logging contracts, CLI parsing, and cross-binary startup behavior. Keeping it isolated avoids leaking trace-contract churn into unrelated gameplay or networking work.

## Owned Paths
- `m-karma/include/karma/common/logging/*`
- `m-karma/src/common/logging/*`
- `m-karma/src/app/shared/logging_setup.cpp`
- `m-karma/src/cli/shared/common_options.cpp`
- `m-karma/src/cli/client/parser.cpp`
- `m-karma/src/cli/server/parser.cpp`
- `m-karma/src/main.cpp`
- `m-bz3/src/main.cpp`

## Interface Boundaries
- Inputs consumed:
  - early CLI args (`--trace*` flags),
  - runtime/binary identity used before full bootstrap.
- Outputs/contracts exposed:
  - stable trace line shape,
  - stable trace-related CLI options and help text.
- Coordination-sensitive files:
  - shared logging header/macro definitions,
  - client/server/common option parsing and localized help.

## Non-Goals
- Reworking trace channel taxonomy.
- Rewriting subsystem-specific trace content beyond formatting/prefix policy.
- Broad non-trace CLI redesign.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

```bash
cd m-karma
./abuild.py -a trace-overseer -d build-test --install-sdk out/karma-sdk --ignore-lock
```

```bash
cd m-bz3
timeout 5s ./build-test/bz3 --trace '*'
```

## Trace Channels
- `cli`
- `data`
- `config`

## Build/Run Commands
```bash
cd m-bz3
timeout 5s ./build-test/bz3 --trace '*'
timeout 5s ./build-test/bz3 --trace '*' --trace-filenames
timeout 5s ./build-test/bz3 --trace '*' --trace-no-prefix
```

## First Session Checklist
1. Lock macro/API signature changes first (`KARMA_TRACE` + any companion helpers).
2. Land CLI option contract changes (`--trace-timestamps`, `--trace-filenames`, `--trace-no-prefix`) with help text updates.
3. Wire startup identity into trace-prefix label selection.
4. Apply callsite migrations needed by the explicit filename argument.
5. Run startup smoke checks and capture before/after examples.

## Current Status
- `2026-02-25`: Project created after initial trace format cleanup landed (`[file]` prefix style). Next step is hardening the contract so formatting is not convention-driven.

## Open Questions
- Do we keep `--timestamp-logging` as a temporary alias for one transition window, or hard-cut to `--trace-timestamps` immediately?
- Should `--trace-no-prefix` remove only the first bracketed label (for example `[trace]`) or all global prefix metadata?
- For explicit filename argument in `KARMA_TRACE`, do we want a helper macro that injects `__FILE__` for low-friction callsites?

## Handoff Checklist
- [ ] `KARMA_TRACE` signature updated with explicit filename contract.
- [ ] Prefix label source unified with early runtime/binary identity.
- [ ] `--trace-timestamps` implemented and wired.
- [ ] `--trace-filenames` implemented with default-off behavior.
- [ ] `--trace-no-prefix` implemented.
- [ ] CLI help + parser tests updated.
- [ ] Startup trace output evidence captured for all new modes.
