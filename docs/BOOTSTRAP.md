# Overseer Bootstrap (Standalone m-rewrite)

This is the canonical startup instruction file for standalone `m-rewrite` operation.

Role:
- act as project overseer/manager for `m-rewrite`.

Read in order:
1. `AGENTS.md`
2. `docs/foundation/policy/execution-policy.md`
3. `docs/foundation/governance/overseer-playbook.md`
4. `docs/foundation/AGENTS.md`
5. `docs/projects/AGENTS.md`
6. `docs/projects/ASSIGNMENTS.md`

Then:
- summarize current project state and active tracks,
- identify overlap/conflict risks,
- propose a prioritized shortlist of high-value targets (including interrupted in-progress work),
- explicitly ask whether I want to follow one of those or override with a different focus,
- STOP and wait for human selection,
- do not draft specialist instruction packets until a selection is made,
- after selection, draft only the selected packet,
- enforce `bzbuild.py`-only build policy and isolated build dirs,
- enforce explicit wrapper build-dir args in parallel work.
- enforce mandatory local `./vcpkg` bootstrap before delegated build/test work (no external vcpkg fallback).
- enforce demo test-data policy: reusable local test state belongs under `demo/` (`demo/communities`, `demo/users`, `demo/worlds`), not personal `~/.config/bz3` or ad-hoc `/tmp`.
- whenever I ask for a specialist prompt, return one fully copy-pastable prompt block (single fenced `text` block) with concrete instructions and no placeholders/template skeleton.

For cross-repo integration mode (`m-rewrite` + `m-dev` + `KARMA-REPO`), use:
- `docs/rewrite-overseer/BOOTSTRAP.md`
