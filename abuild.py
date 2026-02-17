#!/usr/bin/env python3

import os
import shutil
import socket
import subprocess
import sys
from datetime import datetime, timezone


PLATFORM_OPTIONS = ["sdl3"]
RENDERER_OPTIONS = ["bgfx", "diligent"]
PHYSICS_OPTIONS = ["jolt", "physx"]
UI_OPTIONS = ["rmlui", "imgui"]
AUDIO_OPTIONS = ["sdl3audio", "miniaudio"]

DEFAULT_PLATFORM = "sdl3"
DEFAULT_RENDERER = "bgfx"
DEFAULT_PHYSICS = "jolt"
DEFAULT_UI = "rmlui"
DEFAULT_AUDIO = "sdl3audio"

LOCK_FILENAME = ".agent-locked"

CATEGORY_ORDER = ["platform", "renderer", "physics", "ui", "audio"]
CATEGORY_OPTIONS = {
    "platform": PLATFORM_OPTIONS,
    "renderer": RENDERER_OPTIONS,
    "physics": PHYSICS_OPTIONS,
    "ui": UI_OPTIONS,
    "audio": AUDIO_OPTIONS,
}
CATEGORY_DEFAULTS = {
    "platform": DEFAULT_PLATFORM,
    "renderer": DEFAULT_RENDERER,
    "physics": DEFAULT_PHYSICS,
    "ui": DEFAULT_UI,
    "audio": DEFAULT_AUDIO,
}


def usage(exit_code: int = 1) -> None:
    prog = os.path.basename(sys.argv[0])
    print(
        f"Usage: {prog} [-D|--defaults] [-c|--configure] [-a|--all] "
        "[-d|--dir <build-dir>] [-b|--backends <csv>] "
        "[--agent <name>] [--claim-lock|--release-lock|--lock-status] "
        "[--ignore-lock] [--test-physics] [--test-audio] [--test-engine]",
        file=sys.stderr,
    )
    print("  -D, --defaults    print default backends and exit", file=sys.stderr)
    print("  -c, --configure   run cmake -S configure step before building", file=sys.stderr)
    print("  -a, --all         build consolidated dev profile into build-dev/", file=sys.stderr)
    print("  -d, --dir         build directory (for example: build-a1)", file=sys.stderr)
    print(
        "  -b, --backends    comma list of backend tokens "
        "(for example: diligent,bgfx,physx)",
        file=sys.stderr,
    )
    print("                    by category:", file=sys.stderr)
    print(f"                      platform: {', '.join(PLATFORM_OPTIONS)}", file=sys.stderr)
    print(f"                      renderer: {', '.join(RENDERER_OPTIONS)}", file=sys.stderr)
    print(f"                      physics : {', '.join(PHYSICS_OPTIONS)}", file=sys.stderr)
    print(f"                      ui      : {', '.join(UI_OPTIONS)}", file=sys.stderr)
    print(f"                      audio   : {', '.join(AUDIO_OPTIONS)}", file=sys.stderr)
    print("  --agent           agent/session owner name for lock ownership checks", file=sys.stderr)
    print("  --claim-lock      claim .agent-locked for --dir and exit", file=sys.stderr)
    print("  --release-lock    release .agent-locked for --dir and exit", file=sys.stderr)
    print("  --lock-status     print lock metadata for --dir and exit", file=sys.stderr)
    print("  --ignore-lock     ignore build-dir .agent-locked presence check", file=sys.stderr)
    print("  --test-physics    run physics backend parity tests via ctest after build", file=sys.stderr)
    print("  --test-audio      run audio backend smoke tests via ctest after build", file=sys.stderr)
    print("  --test-engine     run both physics and audio backend tests via ctest after build", file=sys.stderr)
    print("  env: ABUILD_AGENT_NAME can supply default agent name", file=sys.stderr)
    raise SystemExit(exit_code)


def fail(message: str) -> None:
    print(f"Error: {message}", file=sys.stderr)
    usage(1)


def run(cmd: list[str], *, env: dict[str, str] | None = None) -> None:
    subprocess.run(cmd, check=True, env=env)


def read_cached_toolchain(build_dir: str) -> str:
    cache_path = os.path.join(build_dir, "CMakeCache.txt")
    if not os.path.isfile(cache_path):
        return ""

    try:
        with open(cache_path, "r", encoding="utf-8") as cache:
            for line in cache:
                if line.startswith("CMAKE_TOOLCHAIN_FILE:"):
                    _, value = line.split("=", 1)
                    return value.strip()
    except OSError:
        return ""

    return ""


def read_cache_internal(cache_path: str, key: str) -> str:
    if not os.path.isfile(cache_path):
        return ""

    needle = f"{key}:INTERNAL="
    try:
        with open(cache_path, "r", encoding="utf-8") as cache:
            for line in cache:
                if line.startswith(needle):
                    return line.split("=", 1)[1].strip()
    except OSError:
        return ""

    return ""


def is_subpath(path: str, root: str) -> bool:
    path_abs = os.path.abspath(path)
    root_abs = os.path.abspath(root)
    try:
        return os.path.commonpath([path_abs, root_abs]) == root_abs
    except ValueError:
        return False


def cleanup_relocated_fetchcontent_caches(build_dir: str) -> int:
    deps_dir = os.path.join(build_dir, "_deps")
    if not os.path.isdir(deps_dir):
        return 0

    cleared = 0
    for root, _dirs, files in os.walk(deps_dir):
        if "CMakeCache.txt" not in files:
            continue
        cache_path = os.path.join(root, "CMakeCache.txt")
        cache_dir = read_cache_internal(cache_path, "CMAKE_CACHEFILE_DIR")
        if not cache_dir:
            continue
        if is_subpath(cache_dir, build_dir):
            continue

        cmake_files = os.path.join(root, "CMakeFiles")
        try:
            os.remove(cache_path)
            cleared += 1
        except OSError:
            pass

        if os.path.isdir(cmake_files):
            # Remove stale nested generate state from old build directory paths.
            shutil.rmtree(cmake_files, ignore_errors=True)

    return cleared


def is_local_vcpkg_bootstrapped(vcpkg_root: str) -> bool:
    candidates = [
        os.path.join(vcpkg_root, "vcpkg"),
        os.path.join(vcpkg_root, "vcpkg.exe"),
        os.path.join(vcpkg_root, "vcpkg.bat"),
    ]
    return any(os.path.isfile(path) for path in candidates)


def ensure_local_vcpkg(repo_root: str) -> tuple[str, str]:
    local_vcpkg_root = os.path.join(repo_root, "vcpkg")
    local_toolchain = os.path.join(local_vcpkg_root, "scripts", "buildsystems", "vcpkg.cmake")

    if not os.path.isdir(local_vcpkg_root):
        print(
            "Error: local vcpkg is mandatory for builds, but ./vcpkg is missing.\n"
            "Bootstrap once from repo root:\n"
            "  git clone https://github.com/microsoft/vcpkg.git vcpkg\n"
            "  ./vcpkg/bootstrap-vcpkg.sh -disableMetrics",
            file=sys.stderr,
        )
        raise SystemExit(2)

    if not os.path.isfile(local_toolchain):
        print(
            "Error: local vcpkg exists but toolchain file is missing.\n"
            "Expected:\n"
            "  ./vcpkg/scripts/buildsystems/vcpkg.cmake\n"
            "Re-bootstrap local vcpkg and retry.",
            file=sys.stderr,
        )
        raise SystemExit(2)

    if not is_local_vcpkg_bootstrapped(local_vcpkg_root):
        print(
            "Error: local ./vcpkg is present but not bootstrapped.\n"
            "Run:\n"
            "  ./vcpkg/bootstrap-vcpkg.sh -disableMetrics",
            file=sys.stderr,
        )
        raise SystemExit(2)

    return os.path.abspath(local_vcpkg_root), os.path.abspath(local_toolchain)


def backend_category_map() -> dict[str, str]:
    mapping: dict[str, str] = {}
    for category in CATEGORY_ORDER:
        for token in CATEGORY_OPTIONS[category]:
            mapping[token] = category
    return mapping


def parse_backend_tokens(raw_specs: list[str]) -> dict[str, list[str]]:
    by_category = {category: [] for category in CATEGORY_ORDER}
    token_to_category = backend_category_map()
    saw_any = False

    for raw in raw_specs:
        for token in raw.split(","):
            normalized = token.strip().lower()
            if not normalized:
                continue
            saw_any = True
            category = token_to_category.get(normalized)
            if category is None:
                valid = ", ".join(sorted(token_to_category.keys()))
                fail(f"unknown backend token '{normalized}' (valid: {valid})")
            if normalized not in by_category[category]:
                by_category[category].append(normalized)

    if raw_specs and not saw_any:
        fail("backend list was empty; provide at least one backend token")

    return by_category


def defaults_profile() -> dict[str, object]:
    return {
        "build_dir": "",
        "platform_default": DEFAULT_PLATFORM,
        "platforms": [DEFAULT_PLATFORM],
        "renderer_default": DEFAULT_RENDERER,
        "renderers": [DEFAULT_RENDERER],
        "physics_default": DEFAULT_PHYSICS,
        "physics_backends": [DEFAULT_PHYSICS],
        "ui_default": DEFAULT_UI,
        "ui_backends": [DEFAULT_UI],
        "audio_default": DEFAULT_AUDIO,
        "audio_backends": [DEFAULT_AUDIO],
        "all": False,
    }


def all_profile() -> dict[str, object]:
    profile = defaults_profile()
    profile["build_dir"] = "build-dev"
    profile["renderers"] = ["bgfx", "diligent"]
    profile["physics_backends"] = ["jolt", "physx"]
    profile["ui_backends"] = ["imgui", "rmlui"]
    profile["audio_backends"] = ["sdl3audio", "miniaudio"]
    profile["all"] = True
    return profile


def resolve_profile(build_dir: str, backend_specs: list[str]) -> dict[str, object]:
    if not build_dir:
        fail("build dir is required; use -d <build-dir> or use -a")

    if not build_dir.startswith("build-"):
        fail("build directory must start with 'build-' (example: build-a1)")

    selections = parse_backend_tokens(backend_specs)
    profile = defaults_profile()
    profile["build_dir"] = build_dir

    for category in CATEGORY_ORDER:
        selected = selections[category]
        default_value = str(CATEGORY_DEFAULTS[category])
        if not selected:
            continue

        if len(selected) == 1:
            values = [selected[0]]
            selected_default = selected[0]
        else:
            if category == "platform":
                fail("platform runtime multi-select is not supported; choose one platform backend")
            values = selected
            if default_value in selected:
                selected_default = default_value
            else:
                selected_default = selected[0]

        if category == "platform":
            profile["platforms"] = values
            profile["platform_default"] = selected_default
        elif category == "renderer":
            profile["renderers"] = values
            profile["renderer_default"] = selected_default
        elif category == "physics":
            profile["physics_backends"] = values
            profile["physics_default"] = selected_default
        elif category == "ui":
            profile["ui_backends"] = values
            profile["ui_default"] = selected_default
        elif category == "audio":
            profile["audio_backends"] = values
            profile["audio_default"] = selected_default

    return profile


def print_defaults() -> None:
    print(
        "Build defaults: "
        f"{DEFAULT_PLATFORM}, {DEFAULT_RENDERER}, {DEFAULT_PHYSICS}, {DEFAULT_UI}, {DEFAULT_AUDIO}"
    )


def profile_summary(profile: dict[str, object]) -> str:
    return (
        f"Build profile -> dir={profile['build_dir']} | "
        f"platform={profile['platform_default']} [{','.join(profile['platforms'])}] | "
        f"renderer={profile['renderer_default']} [{','.join(profile['renderers'])}] | "
        f"physics={profile['physics_default']} [{','.join(profile['physics_backends'])}] | "
        f"ui={profile['ui_default']} [{','.join(profile['ui_backends'])}] | "
        f"audio={profile['audio_default']} [{','.join(profile['audio_backends'])}]"
    )


def lock_path(build_dir: str) -> str:
    return os.path.join(build_dir, LOCK_FILENAME)


def resolve_agent_name(agent_arg: str) -> str:
    if agent_arg.strip():
        return agent_arg.strip()
    return os.environ.get("ABUILD_AGENT_NAME", "").strip()


def read_lock_metadata(build_dir: str) -> dict[str, str]:
    path = lock_path(build_dir)
    if not os.path.isfile(path):
        return {}

    metadata: dict[str, str] = {}
    raw_lines: list[str] = []
    try:
        with open(path, "r", encoding="utf-8") as handle:
            for raw_line in handle:
                line = raw_line.rstrip("\n")
                raw_lines.append(line)
                if "=" not in line:
                    continue
                key, value = line.split("=", 1)
                metadata[key.strip()] = value.strip()
    except OSError:
        return {}

    if metadata:
        return metadata

    legacy_text = "\n".join(raw_lines).strip()
    if legacy_text:
        return {"legacy": legacy_text}
    return {}


def write_lock_metadata(build_dir: str, owner: str) -> None:
    os.makedirs(build_dir, exist_ok=True)
    path = lock_path(build_dir)
    claimed_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    lines = [
        f"owner={owner}",
        f"claimed_at={claimed_at}",
        f"host={socket.gethostname()}",
        f"pid={os.getpid()}",
    ]
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def format_lock(metadata: dict[str, str]) -> str:
    owner = metadata.get("owner", "<unknown>")
    claimed_at = metadata.get("claimed_at", "<unknown>")
    host = metadata.get("host", "<unknown>")
    pid = metadata.get("pid", "<unknown>")
    if "legacy" in metadata:
        return f"legacy lock ({metadata['legacy']})"
    return f"owner={owner}, claimed_at={claimed_at}, host={host}, pid={pid}"


def claim_lock(build_dir: str, agent_name: str) -> None:
    metadata = read_lock_metadata(build_dir)
    owner = metadata.get("owner", "")
    if owner and owner != agent_name:
        print(
            f"Error: build directory '{build_dir}' is already claimed by '{owner}' ({format_lock(metadata)}).",
            file=sys.stderr,
        )
        raise SystemExit(3)
    if owner == agent_name:
        print(f"Lock already claimed by '{agent_name}' on {build_dir}.")
        return
    if metadata.get("legacy"):
        print(
            f"Error: build directory '{build_dir}' has a legacy/unowned lock: {metadata['legacy']}. "
            "Clear it manually or use --ignore-lock for one-off builds.",
            file=sys.stderr,
        )
        raise SystemExit(3)

    write_lock_metadata(build_dir, agent_name)
    print(f"Claimed lock for '{agent_name}' on {build_dir}.")


def release_lock(build_dir: str, agent_name: str) -> None:
    path = lock_path(build_dir)
    if not os.path.isfile(path):
        print(f"No lock present for {build_dir}.")
        return

    metadata = read_lock_metadata(build_dir)
    owner = metadata.get("owner", "")
    if owner and owner != agent_name:
        print(
            f"Error: cannot release lock for '{build_dir}'; owner is '{owner}' ({format_lock(metadata)}).",
            file=sys.stderr,
        )
        raise SystemExit(3)
    if metadata.get("legacy"):
        print(
            f"Error: cannot safely release legacy lock for '{build_dir}' ({metadata['legacy']}). "
            "Remove the lock file manually if intended.",
            file=sys.stderr,
        )
        raise SystemExit(3)

    os.remove(path)
    print(f"Released lock for '{agent_name}' on {build_dir}.")


def show_lock_status(build_dir: str) -> None:
    path = lock_path(build_dir)
    if not os.path.isfile(path):
        print(f"{build_dir}: unclaimed")
        return
    metadata = read_lock_metadata(build_dir)
    print(f"{build_dir}: {format_lock(metadata)}")


def ensure_build_lock(build_dir: str, agent_name: str) -> None:
    metadata = read_lock_metadata(build_dir)
    owner = metadata.get("owner", "")
    if not metadata:
        if not agent_name:
            print(
                f"Error: build directory '{build_dir}' is unclaimed. "
                "Provide --agent <name> (or ABUILD_AGENT_NAME) so abuild can claim it, "
                "or run with --ignore-lock.",
                file=sys.stderr,
            )
            raise SystemExit(3)
        write_lock_metadata(build_dir, agent_name)
        print(f"Info: claimed '{build_dir}' for agent '{agent_name}'.")
        return

    if metadata.get("legacy"):
        print(
            f"Error: build directory '{build_dir}' has legacy lock data ({metadata['legacy']}). "
            "Clear .agent-locked manually or use --ignore-lock for one-off builds.",
            file=sys.stderr,
        )
        raise SystemExit(3)

    if not owner:
        print(
            f"Error: build directory '{build_dir}' lock is missing owner metadata. "
            "Recreate lock with --release-lock/--claim-lock or use --ignore-lock.",
            file=sys.stderr,
        )
        raise SystemExit(3)

    if not agent_name:
        print(
            f"Error: build directory '{build_dir}' is claimed by '{owner}'. "
            "Provide --agent <name> (or ABUILD_AGENT_NAME), or use --ignore-lock.",
            file=sys.stderr,
        )
        raise SystemExit(3)

    if owner != agent_name:
        print(
            f"Error: build directory '{build_dir}' is claimed by '{owner}', not '{agent_name}'. "
            f"Lock metadata: {format_lock(metadata)}",
            file=sys.stderr,
        )
        raise SystemExit(3)


def main() -> int:
    args = sys.argv[1:]
    run_configure = False
    run_defaults = False
    all_build = False
    ignore_lock = False
    claim_lock_only = False
    release_lock_only = False
    lock_status_only = False
    run_physics_tests = False
    run_audio_tests = False
    agent_name_arg = ""
    build_dir = ""
    backend_specs: list[str] = []

    i = 0
    while i < len(args):
        arg = args[i]
        if arg in ("-h", "--help"):
            usage(0)
        elif arg in ("-c", "--configure"):
            run_configure = True
        elif arg in ("-D", "--defaults"):
            run_defaults = True
        elif arg in ("-a", "--all"):
            all_build = True
        elif arg in ("-d", "--dir"):
            i += 1
            if i >= len(args):
                fail(f"missing value for '{arg}'")
            build_dir = args[i]
        elif arg in ("-b", "--backends"):
            i += 1
            if i >= len(args):
                fail(f"missing value for '{arg}'")
            backend_specs.append(args[i])
        elif arg == "--agent":
            i += 1
            if i >= len(args):
                fail("missing value for '--agent'")
            agent_name_arg = args[i]
        elif arg == "--claim-lock":
            claim_lock_only = True
        elif arg == "--release-lock":
            release_lock_only = True
        elif arg == "--lock-status":
            lock_status_only = True
        elif arg == "--ignore-lock":
            ignore_lock = True
        elif arg == "--test-physics":
            run_physics_tests = True
        elif arg == "--test-audio":
            run_audio_tests = True
        elif arg == "--test-engine":
            run_physics_tests = True
            run_audio_tests = True
        elif arg.startswith("-"):
            fail(f"unknown option '{arg}'")
        else:
            if build_dir:
                fail("only one build directory may be provided")
            build_dir = arg
        i += 1

    if run_defaults:
        if all_build or build_dir or backend_specs or run_configure or run_physics_tests or run_audio_tests:
            fail("--defaults cannot be combined with build/test options")
        print_defaults()
        return 0

    if (1 if claim_lock_only else 0) + (1 if release_lock_only else 0) + (1 if lock_status_only else 0) > 1:
        fail("choose only one lock operation: --claim-lock, --release-lock, or --lock-status")

    if claim_lock_only or release_lock_only or lock_status_only:
        if all_build or backend_specs or run_configure or run_physics_tests or run_audio_tests:
            fail("lock operations cannot be combined with build/config/test options")
        if not build_dir:
            fail("lock operations require --dir <build-dir>")

        agent_name = resolve_agent_name(agent_name_arg)
        if claim_lock_only:
            if not agent_name:
                fail("claiming a lock requires --agent <name> or ABUILD_AGENT_NAME")
            claim_lock(build_dir, agent_name)
            return 0
        if release_lock_only:
            if not agent_name:
                fail("releasing a lock requires --agent <name> or ABUILD_AGENT_NAME")
            release_lock(build_dir, agent_name)
            return 0
        show_lock_status(build_dir)
        return 0

    if all_build and build_dir:
        fail("cannot combine --all with explicit build directory")
    if all_build and backend_specs:
        fail("cannot combine --all with --backends; --all already enables all runtime-selectable backends")

    profile = all_profile() if all_build else resolve_profile(build_dir, backend_specs)
    print(profile_summary(profile), flush=True)
    agent_name = resolve_agent_name(agent_name_arg)

    build_dir = str(profile["build_dir"])
    platform_default = str(profile["platform_default"])
    renderer_default = str(profile["renderer_default"])
    renderer_values = [str(value) for value in profile["renderers"]]
    physics_default = str(profile["physics_default"])
    physics_values = [str(value) for value in profile["physics_backends"]]
    ui_default = str(profile["ui_default"])
    ui_values = [str(value) for value in profile["ui_backends"]]
    audio_default = str(profile["audio_default"])
    audio_values = [str(value) for value in profile["audio_backends"]]

    cmake_args = [
        f"-DKARMA_WINDOW_BACKEND={platform_default}",
        f"-DKARMA_RENDER_BACKENDS={';'.join(renderer_values)}",
        f"-DKARMA_PHYSICS_BACKEND={physics_default}",
        f"-DKARMA_PHYSICS_BACKENDS={';'.join(physics_values)}",
        f"-DKARMA_UI_BACKEND={ui_default}",
        f"-DKARMA_UI_BACKENDS={';'.join(ui_values)}",
        f"-DKARMA_AUDIO_BACKEND={audio_default}",
        f"-DKARMA_AUDIO_BACKENDS={';'.join(audio_values)}",
    ]

    if "rmlui" in ui_values:
        cmake_args.append("-DKARMA_ENABLE_RMLUI_BACKEND=ON")
    else:
        cmake_args.append("-DKARMA_ENABLE_RMLUI_BACKEND=OFF")

    if run_physics_tests or run_audio_tests:
        cmake_args.append("-DBUILD_TESTING=ON")

    # Default to offline-safe FetchContent behavior to avoid unexpected
    # dependency git updates during normal local builds.
    fetch_updates_disconnected = os.environ.get(
        "ABUILD_FETCH_UPDATES_DISCONNECTED",
        os.environ.get("BZBUILD_FETCH_UPDATES_DISCONNECTED", "1"),
    ).strip().lower()
    if fetch_updates_disconnected in ("0", "false", "off", "no"):
        cmake_args.append("-DFETCHCONTENT_UPDATES_DISCONNECTED=OFF")
    else:
        cmake_args.append("-DFETCHCONTENT_UPDATES_DISCONNECTED=ON")

    env = os.environ.copy()
    repo_root = os.path.abspath(os.path.dirname(__file__))
    local_vcpkg_root, local_toolchain = ensure_local_vcpkg(repo_root)

    if not ignore_lock:
        ensure_build_lock(build_dir, agent_name)

    if os.path.isdir(build_dir):
        cleared = cleanup_relocated_fetchcontent_caches(build_dir)
        if cleared > 0:
            print(
                f"Info: cleared {cleared} relocated FetchContent cache entries under {build_dir}/_deps"
            )

    cached_toolchain = read_cached_toolchain(build_dir)
    expected_toolchain = os.path.abspath(local_toolchain)
    cached_toolchain_abs = os.path.abspath(cached_toolchain) if cached_toolchain else ""
    if cached_toolchain and cached_toolchain_abs != expected_toolchain:
        print(
            "Error: build directory is pinned to a different vcpkg toolchain than required local ./vcpkg.\n"
            f"Current cached toolchain:\n  {cached_toolchain}\n"
            f"Required toolchain:\n  {expected_toolchain}\n"
            "Fix:\n"
            f"  rm -f {build_dir}/CMakeCache.txt\n"
            f"  rm -rf {build_dir}/CMakeFiles\n"
            f"  ./abuild.py -c -d {build_dir}",
            file=sys.stderr,
        )
        return 2

    env_vcpkg_root = env.get("VCPKG_ROOT")
    if env_vcpkg_root and os.path.abspath(env_vcpkg_root) != local_vcpkg_root:
        print(
            "Error: VCPKG_ROOT points outside mandatory local ./vcpkg.\n"
            f"Current VCPKG_ROOT:\n  {env_vcpkg_root}\n"
            f"Required VCPKG_ROOT:\n  {local_vcpkg_root}\n"
            "Unset VCPKG_ROOT or set it to local ./vcpkg and retry.",
            file=sys.stderr,
        )
        return 2

    env["VCPKG_ROOT"] = local_vcpkg_root
    cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={local_toolchain}")

    if not os.path.isdir(build_dir):
        os.makedirs(build_dir, exist_ok=True)
        run_configure = True

    cache_path = os.path.join(build_dir, "CMakeCache.txt")
    if not os.path.isfile(cache_path):
        run_configure = True
    else:
        has_makefile = os.path.isfile(os.path.join(build_dir, "Makefile"))
        has_ninja = os.path.isfile(os.path.join(build_dir, "build.ninja"))
        if not (has_makefile or has_ninja):
            run_configure = True

    if run_configure:
        run(["cmake", "-S", ".", "-B", build_dir, *cmake_args], env=env)

    run(["cmake", "--build", build_dir, "-j4"], env=env)
    if run_physics_tests:
        run(
            ["ctest", "--test-dir", build_dir, "-R", "physics_backend_parity", "--output-on-failure"],
            env=env,
        )
    if run_audio_tests:
        run(["ctest", "--test-dir", build_dir, "-R", "audio_backend_smoke", "--output-on-failure"], env=env)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
