# Testing Modernization (`m-karma` + `m-bz3`)

## Project Snapshot
- Current owner: `unassigned`
- Status: `in progress`
- Immediate next task: Inquire from human operator what to do
- Validation gate: `cd m-overseer && ./agent/scripts/lint-projects.sh`

## Mission
Consolidate ad-hoc testing strategies using ad-hoc data in m-karma into demo client/server binary test using data from m-karma/data/.

End goal:
- ad-hoc test setup replaced by reproducible `demo/` fixtures and binary traces.
- few to no test binaries built; try to get as much testing as possible into <build-dir>/{client,server}

## Foundation References
- `m-overseer/agent/docs/testing.md`
- `m-overseer/agent/projects/testing.md`
- `m-karma/webserver/AGENTS.md`

## Owned Paths
- `m-overseer/agent/projects/testing.md`
- `m-karma/demo/servers/*`
- `m-karma/demo/users/*` (new clones only; no edits to existing fixtures)
- `m-karma/demo/communities/*` (new clones only; no edits to existing fixtures)
- `m-karma/src/*` config callsites touched by approved batches

## Interface Boundaries
- Inputs consumed:
  - runtime config contracts from `data/{client,server}/config.json`
  - community API behavior from `m-karma/webserver`
- Outputs/contracts exposed:
  - canonical config keys and required-read behavior
  - deterministic `demo/` fixtures and binary-driven verification steps
- Coordination constraints:
  - do not modify existing `demo/users/*`
  - do not modify existing `demo/communities/*`; create clones when needed

## Worked Example (Heartbeat Migration)
- Canonicalized heartbeat overlays:
  - `m-karma/demo/servers/test-heartbeat/config.json`
  - `m-bz3/demo/servers/test-heartbeat/config.json`
- Added cloned community fixtures:
  - `m-karma/demo/communities/test-heartbeat/`
  - `m-bz3/demo/communities/test-heartbeat/`
- Updated cloned DBs to include `127.0.0.1:11899` server registration for heartbeat acceptance.
- Added runbook docs:
  - `m-karma/demo/servers/test-heartbeat/README.md`
  - `m-bz3/demo/servers/test-heartbeat/README.md`
- Verified with traces and community heartbeat log entries.

## Key Extension Points
- Convert in-process config mutation tests to binary/fixture paths where practical:
  - `m-bz3/src/tests/community_heartbeat_integration_test.cpp`
  - selected flows in `m-bz3/src/tests/server_net_contract_test.cpp`
- Normalize remaining `demo/servers/*/config.json` overlays to canonical key contracts.

## Non-Goals
- Do not remove fast unit/contract tests that still provide high signal.
- Do not rely on personal home config paths or ad-hoc `/tmp` fixture trees for reusable tests.
- Do not mutate existing community/user fixtures without explicit approval.

## Validation
```bash
cd m-overseer
./agent/scripts/lint-projects.sh
```

```bash
# Example heartbeat smoke (m-bz3)
python3 ../m-karma/webserver/bin/start.py demo/communities/test-heartbeat
timeout 10s ./build-test/bz3-server \
  --server-config demo/servers/test-heartbeat/config.json \
  --trace net.server,engine.server
tail -n 20 demo/communities/test-heartbeat/logs/heartbeat.log
```

## Trace Channels
- `net.server`
- `engine.server`
- `net.client`
- `engine.app`

## Build/Run Commands
```bash
cd m-karma && ./abuild.py -a mike -d build-test --install-sdk out/karma-sdk
cd m-bz3 && ./abuild.py -a mike -d build-test --karma-sdk ../m-karma/out/karma-sdk
```

## Current Status
- `2026-02-23`: heartbeat fixture migration completed as initial worked example.
- `2026-02-23`: cloned `test-heartbeat` communities created in both repos to keep existing fixtures untouched.
- `2026-02-23`: smoke run generated untracked artifact `m-bz3/demo/servers/test-heartbeat.zip` (cleanup policy pending).

## Open Questions
- Which suites should remain in-process for speed, and which should move to binary fixture runs?
- Should generated artifacts such as `demo/servers/test-heartbeat.zip` be gitignored, auto-cleaned, or intentionally tracked?

## Handoff Checklist
- [ ] Candidate batch presented using the matrix template.
- [ ] Approved keys implemented in code and canonical JSON.
- [ ] Binary trace/log evidence captured for changed behavior.
- [ ] Existing `demo/users/*` and `demo/communities/*` remain untouched unless explicitly approved.
