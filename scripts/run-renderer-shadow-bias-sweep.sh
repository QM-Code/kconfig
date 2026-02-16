#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BGFX_BUILD_DIR="build-sdl3-bgfx-physx-imgui-sdl3audio"
DILIGENT_BUILD_DIR="build-sdl3-diligent-physx-imgui-sdl3audio"
BASE_CONFIG="data/client/config.json"

DURATION_SEC="${1:-16}"
HARD_TIMEOUT_SEC="$((DURATION_SEC + 2))"
RUN_TS="$(date -u +%Y%m%dT%H%M%SZ)"
LOG_DIR="/tmp/renderer-shadow-bias-sweep-${RUN_TS}"
mkdir -p "${LOG_DIR}"

if ! command -v jq >/dev/null 2>&1; then
  echo "ERROR: jq is required for config generation." >&2
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "ERROR: python3 is required for summary generation." >&2
  exit 1
fi

cleanup_configs() {
  rm -f "${REPO_ROOT}/data/client/config_bias_sweep_${RUN_TS}_"*.json
}
trap cleanup_configs EXIT

create_profile_config() {
  local profile="$1"
  local receiver="$2"
  local normal="$3"
  local raster_depth="$4"
  local raster_slope="$5"
  local out_cfg="${REPO_ROOT}/data/client/config_bias_sweep_${RUN_TS}_${profile}.json"

  jq \
    --argjson receiver "${receiver}" \
    --argjson normal "${normal}" \
    --argjson raster_depth "${raster_depth}" \
    --argjson raster_slope "${raster_slope}" \
    '.roamingMode.graphics.lighting.shadows.receiverBiasScale=$receiver
     | .roamingMode.graphics.lighting.shadows.normalBiasScale=$normal
     | .roamingMode.graphics.lighting.shadows.rasterDepthBias=$raster_depth
     | .roamingMode.graphics.lighting.shadows.rasterSlopeBias=$raster_slope' \
    "${REPO_ROOT}/${BASE_CONFIG}" > "${out_cfg}"
}

run_capture() {
  local backend="$1"
  local profile="$2"
  local config_path="$3"
  local binary=""
  local trace_channels=""
  local log_file="${LOG_DIR}/${backend}-${profile}.log"

  if [[ "${backend}" == "bgfx" ]]; then
    binary="${REPO_ROOT}/${BGFX_BUILD_DIR}/bz3"
    trace_channels="engine.sim,render.system,render.bgfx"
  else
    binary="${REPO_ROOT}/${DILIGENT_BUILD_DIR}/bz3"
    trace_channels="engine.sim,render.system,render.diligent"
  fi

  if [[ ! -x "${binary}" ]]; then
    echo "ERROR: missing executable ${binary}. Build first with ./bzbuild.py -c <build-dir>" >&2
    exit 1
  fi

  local -a cmd=(
    timeout -k 2s "${HARD_TIMEOUT_SEC}s"
    "${binary}"
    -d "${REPO_ROOT}/data"
    --strict-config=true
    --config "${config_path}"
    -v
    -t "${trace_channels}"
  )

  printf '[bias-sweep] backend=%s profile=%s command:' "${backend}" "${profile}"
  printf ' %q' "${cmd[@]}"
  printf '\n'

  set +e
  "${cmd[@]}" > "${log_file}" 2>&1
  local exit_code=$?
  set -e

  echo "[bias-sweep] backend=${backend} profile=${profile} exit_code=${exit_code} log=${log_file}" \
    | tee -a "${LOG_DIR}/exit-codes.txt"
}

echo "[bias-sweep] repo_root=${REPO_ROOT}"
echo "[bias-sweep] log_dir=${LOG_DIR}"
echo "[bias-sweep] duration_sec=${DURATION_SEC} hard_timeout_sec=${HARD_TIMEOUT_SEC}"

create_profile_config "low" "0.03" "0.12" "0.0" "0.2"
create_profile_config "default" "0.08" "0.35" "0.0" "0.0"
create_profile_config "high" "0.14" "0.60" "0.0012" "2.0"

for profile in low default high; do
  run_capture "bgfx" "${profile}" "${REPO_ROOT}/data/client/config_bias_sweep_${RUN_TS}_${profile}.json"
  run_capture "diligent" "${profile}" "${REPO_ROOT}/data/client/config_bias_sweep_${RUN_TS}_${profile}.json"
done

python3 - <<'PY' "${LOG_DIR}"
import re
import sys
from pathlib import Path

log_dir = Path(sys.argv[1])
profiles = ["low", "default", "high"]
backends = ["bgfx", "diligent"]

print("[bias-sweep] summary (steady-state means; first perf1s sample dropped)")
print("| Backend | Profile | recv | norm | rDepth | rSlope | attachment | steady_fps | steady_ms | steady_steps | worst_max_ms | log |")
print("|---|---|---:|---:|---:|---:|---|---:|---:|---:|---:|---|")

for backend in backends:
    for profile in profiles:
        log_path = log_dir / f"{backend}-{profile}.log"
        if not log_path.exists():
            print(f"| {backend} | {profile} | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | {log_path} |")
            continue

        recv = norm = rdepth = rslope = None
        attachment = "n/a"
        fps = []
        frame_ms = []
        max_ms = []
        steps = []

        for line in log_path.read_text(errors="ignore").splitlines():
            m = re.search(
                r"shadows\(enabled=.*?recv=([0-9.]+) norm=([0-9.]+) rasterDepth=([0-9.]+) rasterSlope=([0-9.]+).*mode=([a-z_]+)\)",
                line,
            )
            if m:
                recv = float(m.group(1))
                norm = float(m.group(2))
                rdepth = float(m.group(3))
                rslope = float(m.group(4))
            m = re.search(r"gpu shadow pass size=[0-9]+ draws=[0-9]+ attachment=([a-z_]+)", line)
            if m:
                attachment = m.group(1)
            m = re.search(
                r"perf1s avg_fps=([0-9.]+) avg_frame_ms=([0-9.]+) max_frame_ms=([0-9.]+) avg_steps=([0-9.]+)",
                line,
            )
            if m:
                fps.append(float(m.group(1)))
                frame_ms.append(float(m.group(2)))
                max_ms.append(float(m.group(3)))
                steps.append(float(m.group(4)))

        sfps = fps[1:] if len(fps) > 1 else fps
        sframe = frame_ms[1:] if len(frame_ms) > 1 else frame_ms
        smax = max_ms[1:] if len(max_ms) > 1 else max_ms
        ssteps = steps[1:] if len(steps) > 1 else steps

        if sfps:
            steady_fps = sum(sfps) / len(sfps)
            steady_ms = sum(sframe) / len(sframe)
            steady_steps = sum(ssteps) / len(ssteps)
            worst_max = max(smax)
            print(
                f"| {backend} | {profile} | "
                f"{recv if recv is not None else 'n/a'} | "
                f"{norm if norm is not None else 'n/a'} | "
                f"{rdepth if rdepth is not None else 'n/a'} | "
                f"{rslope if rslope is not None else 'n/a'} | "
                f"{attachment} | {steady_fps:.2f} | {steady_ms:.2f} | {steady_steps:.2f} | {worst_max:.2f} | {log_path} |"
            )
        else:
            print(
                f"| {backend} | {profile} | "
                f"{recv if recv is not None else 'n/a'} | "
                f"{norm if norm is not None else 'n/a'} | "
                f"{rdepth if rdepth is not None else 'n/a'} | "
                f"{rslope if rslope is not None else 'n/a'} | "
                f"{attachment} | n/a | n/a | n/a | n/a | {log_path} |"
            )
PY

echo "[bias-sweep] done log_dir=${LOG_DIR}"
