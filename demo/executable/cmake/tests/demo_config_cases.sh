#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <demo-binary> <case>"
    echo "Cases:"
    echo "  baseline_success"
}

if [[ $# -ne 2 ]]; then
    usage
    exit 2
fi

binary="$1"
test_case="$2"

if [[ ! -x "$binary" ]]; then
    echo "Error: demo binary is not executable: $binary" >&2
    exit 2
fi

require_contains() {
    local output="$1"
    local needle="$2"
    if ! grep -Fq "$needle" <<<"$output"; then
        echo "Expected output to contain: $needle" >&2
        echo "--- output begin ---" >&2
        echo "$output" >&2
        echo "--- output end ---" >&2
        exit 1
    fi
}

run_case() {
    local -a args=("$@")
    local binary_dir
    local repo_root
    local output
    binary_dir="$(cd "$(dirname "$binary")" && pwd)"
    repo_root="$(cd "$binary_dir/../../../.." && pwd)"
    set +e
    output="$(cd "$repo_root" && "$binary" "${args[@]}" 2>&1)"
    local status=$?
    set -e
    echo "$status"$'\n'"$output"
}

run_and_split() {
    local payload
    payload="$(run_case "$@")"
    status="$(head -n1 <<<"$payload")"
    output="$(tail -n +2 <<<"$payload")"
}

status=0
output=""

case "$test_case" in
    baseline_success)
        run_and_split
        if [[ "$status" -ne 0 ]]; then
            echo "Expected zero exit status for baseline run" >&2
            echo "--- output begin ---" >&2
            echo "$output" >&2
            echo "--- output end ---" >&2
            exit 1
        fi
        require_contains "$output" "KConfig demo executable compile/link/integration check passed"
        ;;
    *)
        echo "Error: unknown case '$test_case'" >&2
        usage
        exit 2
        ;;
esac

echo "PASS: $test_case"
