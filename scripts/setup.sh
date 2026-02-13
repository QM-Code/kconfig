#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_DIR="${ROOT_DIR}/vcpkg"
DEFAULT_BUILD_DIR="build-sdl3-bgfx-jolt-rmlui-sdl3audio"

usage() {
  cat <<'EOF'
Usage: ./scripts/setup.sh [--verify [build-dir]]

Bootstraps mandatory local ./vcpkg for m-rewrite.

Options:
  --verify [build-dir]   After bootstrap, run ./bzbuild.py -c <build-dir>.
                         Default build-dir: build-sdl3-bgfx-jolt-rmlui-sdl3audio
  -h, --help             Show this help.
EOF
}

run_verify=0
build_dir="${DEFAULT_BUILD_DIR}"

case "${1:-}" in
  "")
    ;;
  --verify)
    run_verify=1
    if [[ "${2:-}" != "" ]]; then
      build_dir="$2"
    fi
    ;;
  -h|--help)
    usage
    exit 0
    ;;
  *)
    echo "[setup] ERROR: unknown argument '$1'" >&2
    usage >&2
    exit 1
    ;;
esac

cd "${ROOT_DIR}"

if ! command -v git >/dev/null 2>&1; then
  echo "[setup] ERROR: git is required to bootstrap vcpkg." >&2
  exit 1
fi

if [[ ! -d "${VCPKG_DIR}" ]]; then
  echo "[setup] vcpkg/ not found; cloning..."
  git clone https://github.com/microsoft/vcpkg.git "${VCPKG_DIR}"
fi

if [[ ! -f "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake" ]]; then
  echo "[setup] ERROR: ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake missing." >&2
  echo "[setup] Recreate ./vcpkg and rerun setup." >&2
  exit 1
fi

if [[ ! -x "${VCPKG_DIR}/vcpkg" ]]; then
  echo "[setup] Bootstrapping vcpkg..."
  "${VCPKG_DIR}/bootstrap-vcpkg.sh" -disableMetrics
fi

echo "[setup] Local vcpkg ready at ${VCPKG_DIR}"

if [[ ${run_verify} -eq 1 ]]; then
  echo "[setup] Verifying build with ./bzbuild.py -c ${build_dir}"
  ./bzbuild.py -c "${build_dir}"
fi

echo "[setup] Done."
