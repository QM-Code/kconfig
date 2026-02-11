#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

usage() {
  cat <<EOF
Usage: $(basename "$0") -m "commit message" --all-accepted

Creates a checkpoint commit+push for all current changes in m-rewrite.

Required flags:
  -m, --message       Commit message to use
  --all-accepted      Acknowledge all current working-tree changes are accepted

Notes:
  - Run from any directory; script always operates in m-rewrite repo root.
  - The script runs docs lint before commit and verifies clean/synced state after push.
EOF
}

fail() {
  echo "[checkpoint] ERROR: $*" >&2
  exit 1
}

commit_message=""
ack_all_accepted=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -m|--message)
      [[ $# -ge 2 ]] || fail "missing value for $1"
      commit_message="$2"
      shift 2
      ;;
    --all-accepted)
      ack_all_accepted=1
      shift
      ;;
    *)
      fail "unknown option: $1"
      ;;
  esac
done

[[ -n "${commit_message}" ]] || fail "commit message is required (-m/--message)"
[[ ${ack_all_accepted} -eq 1 ]] || fail "missing required acknowledgement flag: --all-accepted"

cd "${REPO_ROOT}"

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || fail "not inside a git worktree"

current_branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "${current_branch}" != "HEAD" ]] || fail "detached HEAD is not supported for checkpoint pushes"

if [[ -z "$(git status --porcelain)" ]]; then
  fail "working tree is clean; nothing to checkpoint"
fi

echo "[checkpoint] Repo: ${REPO_ROOT}"
echo "[checkpoint] Branch: ${current_branch}"
echo "[checkpoint] Running docs lint..."
./docs/scripts/lint-project-docs.sh

echo "[checkpoint] Staging all current changes..."
git add -A

git diff --cached --quiet && fail "no staged changes after git add -A"

echo "[checkpoint] Committing..."
git commit -m "${commit_message}"

echo "[checkpoint] Pushing origin/${current_branch}..."
git push origin "${current_branch}"

if [[ -n "$(git status --porcelain)" ]]; then
  fail "working tree is not clean after checkpoint push"
fi

sync_counts="$(git rev-list --left-right --count "origin/${current_branch}...HEAD")"
if [[ "${sync_counts}" != $'0\t0' && "${sync_counts}" != "0 0" ]]; then
  fail "local/remote are not synchronized after push: ${sync_counts}"
fi

echo "[checkpoint] OK: commit $(git rev-parse --short HEAD) is pushed and synced."
