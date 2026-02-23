# Hardcoded Literal Migration (`m-karma` + `m-bz3`)

## Project Snapshot
- Current owner: `unassigned`
- Status: `in progress`
- Immediate next task: execute Part (3) fallback-control-flow removals, then run first Part (1) i18n string migration slice.
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`

## Scope
- Runtime/source scope: `m-karma/src/*` and `m-bz3/src/*`
- Exclusions:
  - trace logging strings (`KARMA_TRACE`, `KARMA_TRACE_CHANGED`) stay hardcoded.
  - test-only strings/values are not part of this migration track.

## Part 1: Non-Trace Feedback Strings -> i18n (`data/strings/*`)

### Objective
Identify all non-trace hardcoded feedback strings and migrate them to string keys resolved from `data/strings/`.

### Inventory Baseline (authoritative)
- Snapshot file: `m-overseer/agent/projects/hardcoded-feedback-strings.txt`
- Candidate extraction patterns:
  - `spdlog::(error|warn|info|critical)("...")`
  - `throw std::runtime_error("...")`
- Snapshot size: `354` lines (includes headers).
- Current raw counts from extraction commands:
  - `289` `spdlog` feedback literals
  - `60` literal `std::runtime_error` throws

### Flagging Policy (required)
For every candidate in the snapshot:
1. Assign `i18n key` (for example: `feedback.demo.client.JoinRejected`).
2. Add English source text in `data/strings/en.json`.
3. Replace hardcoded literal with i18n lookup.
4. If a message is operational-only and intentionally non-localized, mark explicitly as `defer/ops-only` with rationale.

### Migration Notes
- This project treats all non-trace feedback strings as i18n candidates by default.
- Trace channels remain exempt and hardcoded.

## Part 2: Non-Feedback Hardcoded Values -> Required Config

### Objective
Identify non-feedback hardcoded values (strings, ints, floats, etc.) and migrate runtime behavior to required config reads (`ReadRequired*Config`).

### Current Remaining Inventory (authoritative)
- Snapshot file: `m-overseer/agent/projects/hardcoded-nonfeedback-values.txt`

| Source | Hardcoded value | Proposed required config key | Flag |
|---|---|---|---|
| `m-bz3/src/server/net/transport_event_source/internal.hpp:224` | `kMaxClients = 50` | `server.clients.Max` | `convert` |
| `m-bz3/src/server/net/transport_event_source/internal.hpp:225` | `kNumChannels = 2` | `server.network.ChannelCount` | `convert` |
| `m-bz3/src/server/net/transport_event_source/internal.hpp:226` | `kFirstClientId = 2` | `server.runtime.session.StartClientId` | `convert` |
| `m-bz3/src/ui/frontends/imgui/console/console.hpp:191` | `serverPortInput = 11899` | `client.ui.console.startServer.DefaultPort` | `convert` |
| `m-bz3/src/ui/frontends/rmlui/console/panels/panel_start_server.hpp:117` | `serverPortValue = 11899` | `client.ui.console.startServer.DefaultPort` | `convert` |
| `m-karma/src/ui/backends/rmlui/adapter.cpp:51` | fallback path `"client/fonts/GoogleSans.ttf"` | `client.ui.rmlui.fonts.Regular` | `convert` |
| `m-karma/src/common/config/store.cpp:282` | fallback `SaveIntervalSeconds` default `0.0` | `client.config.SaveIntervalSeconds` | `convert` |
| `m-karma/src/common/config/store.cpp:283` | fallback `MergeIntervalSeconds` default `0.0` | `client.config.MergeIntervalSeconds` | `convert` |
| `m-karma/src/common/config/store.cpp:33` | `saveIntervalSeconds = 0.0` member init | `client.config.SaveIntervalSeconds` driven | `convert` |
| `m-karma/src/common/config/store.cpp:34` | `mergeIntervalSeconds = 0.0` member init | `client.config.MergeIntervalSeconds` driven | `convert` |
| `m-karma/src/common/config/store.cpp:286` | `saveIntervalSeconds = 0.0` (persistence-disabled branch) | policy-gated required path | `convert/policy` |
| `m-karma/src/common/config/store.cpp:287` | `mergeIntervalSeconds = 0.0` (persistence-disabled branch) | policy-gated required path | `convert/policy` |

### Conversion Rule
- All behavior-defining values above should be read from required config keys.
- Keep range guards/clamps after required reads.

## Part 3: Remaining Fallback-Control-Flow Issues

### Issue A
- File: `m-karma/src/common/i18n/i18n.cpp`
- Current behavior: tries `client.Language`, then falls back to `server.Language`.
- Why this remains fallback flow: key lookup path still branches as compatibility behavior.

#### Resolution Plan
1. Split language loading by runtime role:
   - client runtime: required `client.Language`
   - server runtime: required `server.Language`
2. Remove cross-role fallback branch from `I18n::loadFromConfig()`.
3. Keep normalization and `en` fallback only for empty/whitespace normalization result, not cross-key fallback.

### Issue B
- File: `m-karma/src/demo/client/runtime.cpp`
- Current behavior: `ReadStringFallback(...)` iterates legacy endpoint keys (`network.ServerEndpoint`, `network.DefaultServer`, `network.Server`, `network.ServerHost`).
- Why this remains fallback flow: startup path still uses multi-key compatibility resolution.

#### Resolution Plan
1. Replace `ReadStringFallback(...)` call path with single canonical required key for non-CLI startup endpoint.
2. Proposed canonical key:
   - `client.network.ServerEndpoint`
3. Keep CLI `--server` override behavior unchanged.
4. Remove legacy key probing from runtime path.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

```bash
# fallback/default callsite guard
rg -n -P "\\b(ReadBoolConfig|ReadUInt16Config|ReadFloatConfig|ReadStringConfig)\\s*\\(" m-karma/src m-bz3/src \
  | rg -v "m-karma/src/common/config/helpers\\.(cpp|hpp)"
```

## Handoff Checklist
- [ ] Part (3) Issue A implemented and validated.
- [ ] Part (3) Issue B implemented and validated.
- [ ] Part (1) feedback string batch-1 migrated to `data/strings/en.json` keys.
- [ ] Part (2) non-feedback hardcoded value batch-1 migrated to required config keys.
- [ ] Snapshot files refreshed after each migration slice.
