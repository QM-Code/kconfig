#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

TEST_REGEX="physics_backend_parity_jolt|physics_backend_parity_physx|audio_backend_smoke_sdl3audio|audio_backend_smoke_miniaudio"
BUILD_DIR="build-dev"

usage() {
  cat <<EOF
Usage: $(basename "$0") [build-dir]

Runs engine backend validation tests in the provided build directory.
Default build directory: build-dev
EOF
}

if [[ $# -gt 1 ]]; then
  usage >&2
  exit 1
fi

if [[ $# -eq 1 ]]; then
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *)
      BUILD_DIR="$1"
      ;;
  esac
fi

cd "${REPO_ROOT}"

cmake -S . -B "${BUILD_DIR}"

cmake --build "${BUILD_DIR}" --target \
  physics_backend_parity_test \
  audio_backend_smoke_test

REGISTERED_COUNT="$(
  ctest --test-dir "${BUILD_DIR}" -N -R "${TEST_REGEX}" \
    | awk '/Total Tests:/ {print $3}'
)"

if [[ -z "${REGISTERED_COUNT}" ]]; then
  echo "Unable to determine registered engine backend test count."
  exit 1
fi

if [[ "${REGISTERED_COUNT}" == "0" ]]; then
  echo "No engine backend tests are registered in this build profile; skipping CTest run."
  exit 0
fi

ctest --test-dir "${BUILD_DIR}" -R "${TEST_REGEX}" --output-on-failure
