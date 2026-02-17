#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BGFX_BUILD_DIR="build-sdl3-bgfx-physx-imgui-sdl3audio"
DILIGENT_BUILD_DIR="build-sdl3-diligent-physx-imgui-sdl3audio"

BACKEND="${1:-all}"          # all|bgfx|diligent
DURATION_SEC="${2:-20}"      # per-backend runtime
GROUND_TILES="${3:-1}"
GROUND_EXTENT="${4:-20}"
MOTION_SPEED="${5:-0.9}"
ENABLE_SCENE_MOTION="${6:-0}" # 0|1 (default 0 for stable visual validation)
HARD_TIMEOUT_SEC="$((DURATION_SEC + 4))"

RUN_TS="$(date -u +%Y%m%dT%H%M%SZ)"
LOG_DIR="/tmp/point-shadow-visual-closeout-${RUN_TS}"
mkdir -p "${LOG_DIR}"

run_backend() {
  local backend="$1"
  local build_dir="$2"
  local trace_channels="$3"
  local backend_video_driver_var="SANDBOX_VIDEO_DRIVER_${backend^^}"
  local backend_video_driver="${!backend_video_driver_var:-${SANDBOX_VIDEO_DRIVER:-}}"
  local binary="${REPO_ROOT}/${build_dir}/src/engine/renderer_shadow_sandbox"
  local log_file="${LOG_DIR}/visual-${backend}.log"
  local -a base_cmd=(
    timeout "${HARD_TIMEOUT_SEC}s"
    "${binary}"
    --backend-render "${backend}"
    --duration-sec "${DURATION_SEC}"
    --ground-tiles "${GROUND_TILES}"
    --ground-extent "${GROUND_EXTENT}"
    --shadow-map-size 2048
    --shadow-pcf 2
    --shadow-strength 0.85
    --shadow-execution-mode gpu_default
    --point-shadow-lights 2
    --point-shadow-map-size 1024
    --point-shadow-max-lights 2
    --point-shadow-face-budget 2
    --point-shadow-light-range 14
    --point-shadow-light-intensity 2
    --trace "${trace_channels}"
    --verbose
  )
  if [[ "${ENABLE_SCENE_MOTION}" == "1" ]]; then
    base_cmd+=(--point-shadow-scene-motion --point-shadow-motion-speed "${MOTION_SPEED}")
  fi
  local -a cmd=("${base_cmd[@]}")
  if [[ -n "${backend_video_driver}" ]]; then
    cmd+=(--video-driver "${backend_video_driver}")
  fi

  if [[ ! -x "${binary}" ]]; then
    echo "ERROR: missing executable ${binary}. Build first with ./abuild.py -c ${build_dir}" >&2
    exit 1
  fi

  printf '[point-shadow-visual] backend=%s command:' "${backend}"
  printf ' %q' "${cmd[@]}"
  printf '\n'
  echo "[point-shadow-visual] backend=${backend} log=${log_file}"

  set +e
  "${cmd[@]}" >"${log_file}" 2>&1
  local exit_code=$?
  set -e

  if [[ "${backend}" == "diligent" && ${exit_code} -ne 0 && -z "${backend_video_driver}" ]]; then
    if grep -q "Failed to create OS-specific surface" "${log_file}" &&
       grep -q "ERROR_INITIALIZATION_FAILED" "${log_file}"; then
      local retry_log_file="${LOG_DIR}/visual-${backend}-wayland-retry.log"
      local -a retry_cmd=("${base_cmd[@]}" --video-driver "wayland")
      echo "[point-shadow-visual] backend=${backend} retry reason=x11_swapchain_init_failed fallback=wayland"
      printf '[point-shadow-visual] backend=%s retry command:' "${backend}"
      printf ' %q' "${retry_cmd[@]}"
      printf '\n'
      set +e
      "${retry_cmd[@]}" >"${retry_log_file}" 2>&1
      exit_code=$?
      set -e
      log_file="${retry_log_file}"
    fi
  fi

  local status_reason
  status_reason="$(rg "point shadow status" "${log_file}" | tail -n 1 | sed -n 's/.*reason=\([^ ]*\).*/\1/p')"
  if [[ -z "${status_reason}" ]]; then
    status_reason="missing"
  fi
  echo "[point-shadow-visual] backend=${backend} exit_code=${exit_code} status=${status_reason} log=${log_file}"
  rg "point shadow status|point shadow refresh|\\[sandbox\\]" "${log_file}" | tail -n 10 || true
}

echo "[point-shadow-visual] repo_root=${REPO_ROOT}"
echo "[point-shadow-visual] log_dir=${LOG_DIR}"
echo "[point-shadow-visual] backend=${BACKEND} duration_sec=${DURATION_SEC} scene_motion=${ENABLE_SCENE_MOTION} motion_speed=${MOTION_SPEED}"
echo "[point-shadow-visual] defaults: pointMap=1024 pointMaxLights=2 pointFaceBudget=2 shadowMap=2048 shadowPCF=2"
echo "[point-shadow-visual] video_driver_default=${SANDBOX_VIDEO_DRIVER:-<auto>} bgfx=${SANDBOX_VIDEO_DRIVER_BGFX:-<default>} diligent=${SANDBOX_VIDEO_DRIVER_DILIGENT:-<default>}"

cd "${REPO_ROOT}"

case "${BACKEND}" in
  all)
    run_backend "bgfx" "${BGFX_BUILD_DIR}" "render.system,render.bgfx"
    run_backend "diligent" "${DILIGENT_BUILD_DIR}" "render.system,render.diligent"
    ;;
  bgfx)
    run_backend "bgfx" "${BGFX_BUILD_DIR}" "render.system,render.bgfx"
    ;;
  diligent)
    run_backend "diligent" "${DILIGENT_BUILD_DIR}" "render.system,render.diligent"
    ;;
  *)
    echo "ERROR: invalid backend '${BACKEND}' (expected all|bgfx|diligent)" >&2
    exit 1
    ;;
esac

echo "[point-shadow-visual] summary log_dir=${LOG_DIR}"
