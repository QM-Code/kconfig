#!/bin/bash
set -euo pipefail

usage() {
    echo "Usage: ./tests/full-test.sh --version <slot> [--agent <name>] [--ktrace-sdk <path>]"
}

version=""
agent="mike"
ktrace_sdk=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            if [[ $# -lt 2 ]]; then
                echo "Error: --version requires a value" >&2
                usage
                exit 1
            fi
            version="$2"
            shift 2
            ;;
        --agent)
            if [[ $# -lt 2 ]]; then
                echo "Error: --agent requires a value" >&2
                usage
                exit 1
            fi
            agent="$2"
            shift 2
            ;;
        --ktrace-sdk)
            if [[ $# -lt 2 ]]; then
                echo "Error: --ktrace-sdk requires a value" >&2
                usage
                exit 1
            fi
            ktrace_sdk="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown argument '$1'" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${version}" ]]; then
    echo "Error: --version is required" >&2
    exit 1
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "${repo_root}"

if [[ -z "${ktrace_sdk}" ]]; then
    ktrace_sdk="${repo_root}/../m-ktrace/build/${version}/sdk/"
fi
# Normalize to absolute path so demo/compile/abuild.py sees a stable SDK prefix.
ktrace_sdk="$(cd "$(dirname "${ktrace_sdk}")" && pwd)/$(basename "${ktrace_sdk}")"

# Build kconfig SDK
./abuild.py -a "${agent}" -d "build/${version}/"

# Build compile executable demo
cd demo/compile
./abuild.py -a "${agent}" -d "build/${version}/" \
    --kconfig-sdk "../../build/${version}/sdk/" \
    --ktrace-sdk "${ktrace_sdk}"

exit 0
