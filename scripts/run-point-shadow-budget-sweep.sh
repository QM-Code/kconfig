#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BGFX_BUILD_DIR="build-sdl3-bgfx-physx-imgui-sdl3audio"
DILIGENT_BUILD_DIR="build-sdl3-diligent-physx-imgui-sdl3audio"

DURATION_SEC="${1:-16}"
POINT_MAP_SIZE="${2:-1024}"
BUDGETS_CSV="${3:-1,2,4}"
POINT_LIGHTS="${4:-2}"
GROUND_TILES="${5:-1}"
GROUND_EXTENT="${6:-20}"
MOTION_SPEED="${7:-0.9}"
HARD_TIMEOUT_SEC="$((DURATION_SEC + 4))"

RUN_TS="$(date -u +%Y%m%dT%H%M%SZ)"
LOG_DIR="/tmp/point-shadow-budget-sweep-${RUN_TS}"
SUMMARY_CSV="${LOG_DIR}/summary.csv"
mkdir -p "${LOG_DIR}"

IFS=',' read -r -a BUDGETS <<<"${BUDGETS_CSV}"

echo "backend,budget,exit_code,avg_fps,avg_ms,max_ms,status_reason,faces_updated,waiting_budget,cache_hit,log" >"${SUMMARY_CSV}"

extract_sandbox_dt() {
  local log_file="$1"
  grep -E "\\[sandbox\\].*dt_raw\\(avg=" "${log_file}" | tail -n 1 | \
    sed -n 's/.*dt_raw(avg=\([0-9.]*\) max=\([0-9.]*\)).*/\1 \2/p'
}

extract_status_reason() {
  local log_file="$1"
  rg "point shadow status" "${log_file}" | tail -n 1 | \
    sed -n 's/.*reason=\([^ ]*\).*/\1/p'
}

run_case() {
  local backend="$1"
  local build_dir="$2"
  local trace_channels="$3"
  local budget="$4"
  local backend_video_driver_var="SANDBOX_VIDEO_DRIVER_${backend^^}"
  local backend_video_driver="${!backend_video_driver_var:-${SANDBOX_VIDEO_DRIVER:-}}"
  local binary="${REPO_ROOT}/${build_dir}/src/engine/renderer_shadow_sandbox"
  local log_file="${LOG_DIR}/point-shadow-${backend}-budget${budget}.log"

  if [[ ! -x "${binary}" ]]; then
    echo "ERROR: missing executable ${binary}. Build first with ./abuild.py -c ${build_dir}" >&2
    exit 1
  fi

  local -a base_args=(
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
    --point-shadow-lights "${POINT_LIGHTS}"
    --point-shadow-map-size "${POINT_MAP_SIZE}"
    --point-shadow-max-lights "${POINT_LIGHTS}"
    --point-shadow-face-budget "${budget}"
    --point-shadow-light-range 14
    --point-shadow-light-intensity 2
    --point-shadow-scene-motion
    --point-shadow-motion-speed "${MOTION_SPEED}"
    --trace "${trace_channels}"
    --verbose
  )
  local -a cmd=("${base_args[@]}")
  if [[ -n "${backend_video_driver}" ]]; then
    cmd+=(--video-driver "${backend_video_driver}")
  fi

  printf '[point-shadow-sweep] backend=%s budget=%s command:' "${backend}" "${budget}"
  printf ' %q' "${cmd[@]}"
  printf '\n'

  set +e
  "${cmd[@]}" >"${log_file}" 2>&1
  local exit_code=$?
  set -e

  if [[ "${backend}" == "diligent" && ${exit_code} -ne 0 && -z "${backend_video_driver}" ]]; then
    if grep -q "Failed to create OS-specific surface" "${log_file}" &&
       grep -q "ERROR_INITIALIZATION_FAILED" "${log_file}"; then
      local retry_log_file="${LOG_DIR}/point-shadow-${backend}-budget${budget}-wayland-retry.log"
      local -a retry_cmd=("${base_args[@]}" --video-driver "wayland")
      echo "[point-shadow-sweep] backend=${backend} budget=${budget} retry reason=x11_swapchain_init_failed fallback=wayland"
      printf '[point-shadow-sweep] backend=%s budget=%s retry command:' "${backend}" "${budget}"
      printf ' %q' "${retry_cmd[@]}"
      printf '\n'
      set +e
      "${retry_cmd[@]}" >"${retry_log_file}" 2>&1
      exit_code=$?
      set -e
      log_file="${retry_log_file}"
    fi
  fi

  local dt_pair
  dt_pair="$(extract_sandbox_dt "${log_file}")"
  local avg_dt="0"
  local max_dt="0"
  if [[ -n "${dt_pair}" ]]; then
    avg_dt="$(awk '{print $1}' <<<"${dt_pair}")"
    max_dt="$(awk '{print $2}' <<<"${dt_pair}")"
  fi
  local avg_fps
  avg_fps="$(awk -v d="${avg_dt}" 'BEGIN { if (d > 0) printf "%.2f", 1.0 / d; else printf "0.00"; }')"
  local avg_ms
  avg_ms="$(awk -v d="${avg_dt}" 'BEGIN { printf "%.2f", d * 1000.0; }')"
  local max_ms
  max_ms="$(awk -v d="${max_dt}" 'BEGIN { printf "%.2f", d * 1000.0; }')"
  local status_reason
  status_reason="$(extract_status_reason "${log_file}")"
  if [[ -z "${status_reason}" ]]; then
    status_reason="missing"
  fi
  local faces_updated
  faces_updated="$(rg -c "point_shadow_faces_updated" "${log_file}" || true)"
  if [[ -z "${faces_updated}" ]]; then
    faces_updated="0"
  fi
  local waiting_budget
  waiting_budget="$(rg -c "point_shadow_waiting_budget" "${log_file}" || true)"
  if [[ -z "${waiting_budget}" ]]; then
    waiting_budget="0"
  fi
  local cache_hit
  cache_hit="$(rg -c "point_shadow_cache_hit_clean" "${log_file}" || true)"
  if [[ -z "${cache_hit}" ]]; then
    cache_hit="0"
  fi

  echo "${backend},${budget},${exit_code},${avg_fps},${avg_ms},${max_ms},${status_reason},${faces_updated},${waiting_budget},${cache_hit},${log_file}" >>"${SUMMARY_CSV}"

  echo "[point-shadow-sweep] backend=${backend} budget=${budget} exit_code=${exit_code} avg_fps=${avg_fps} avg_ms=${avg_ms} max_ms=${max_ms} status=${status_reason} updated=${faces_updated} waiting=${waiting_budget} cacheHit=${cache_hit} log=${log_file}"
  rg "point shadow status|point shadow refresh" "${log_file}" | tail -n 6 || true
}

echo "[point-shadow-sweep] repo_root=${REPO_ROOT}"
echo "[point-shadow-sweep] log_dir=${LOG_DIR}"
echo "[point-shadow-sweep] duration_sec=${DURATION_SEC} point_map_size=${POINT_MAP_SIZE} point_lights=${POINT_LIGHTS} budgets=${BUDGETS_CSV} motion_speed=${MOTION_SPEED}"
echo "[point-shadow-sweep] video_driver_default=${SANDBOX_VIDEO_DRIVER:-<auto>} bgfx=${SANDBOX_VIDEO_DRIVER_BGFX:-<default>} diligent=${SANDBOX_VIDEO_DRIVER_DILIGENT:-<default>}"

cd "${REPO_ROOT}"

for budget in "${BUDGETS[@]}"; do
  run_case "bgfx" "${BGFX_BUILD_DIR}" "render.system,render.bgfx" "${budget}"
  run_case "diligent" "${DILIGENT_BUILD_DIR}" "render.system,render.diligent" "${budget}"
done

echo "[point-shadow-sweep] summary csv: ${SUMMARY_CSV}"
echo "[point-shadow-sweep] summary table:"
printf "%-8s %-6s %-5s %-8s %-8s %-8s %-28s %-8s %-8s %-8s %s\n" "backend" "budget" "exit" "avg_fps" "avg_ms" "max_ms" "status" "updated" "waiting" "cacheHit" "log"
tail -n +2 "${SUMMARY_CSV}" | while IFS=',' read -r backend budget exit_code avg_fps avg_ms max_ms status_reason faces_updated waiting_budget cache_hit log_file; do
  printf "%-8s %-6s %-5s %-8s %-8s %-8s %-28s %-8s %-8s %-8s %s\n" \
    "${backend}" "${budget}" "${exit_code}" "${avg_fps}" "${avg_ms}" "${max_ms}" "${status_reason}" "${faces_updated}" "${waiting_budget}" "${cache_hit}" "${log_file}"
done
