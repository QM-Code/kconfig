# Build Policy

## Per-branch/repo building

- m-karma and m-bz3 both provide the abuild.py script for building
- m-overseer does not build anything
- m-dev and q-karma provide bzbuild.py, but generally we will never be building there
- Always use abuild.py for building.
- Always build from the branch/repo root.

## Toolchain Policy
- m-karma and m-bz3 each must have their own branch-level `./vcpkg`
- A missing/unbootstrapped local `./vcpkg` in either m-karma or m-bz3 is a blocker
- Do not improvise or attempt workarounds for vcpkg errors.
- On bootstrap, if a repo exists but vcpkg has not been set up:
  - Inform the human operator that builds will not work until vcpkg is set up.
  - Offer to set up vcpkg.

## Restrictions

- Never use raw `cmake -S/-B` except for very specific tests. In general, always use abuild.py.
- Build directories must always start with 'build-' and live in the corresponding branch/repo root. No exceptions.
- Builds must always take place in the corresponding repo/brach root. No exceptions.
- Do not build until being explicitly assigned an ID and a build directory, either from a human operator or an agent overseer/manager.

## Basics of abuild.py

- abuild.py assigns agents their own build directories and handles all build configuration
- abuild.py ensures ownership by having agents claim and lock directories before usage
- Basic usage:
  - Claiming and locking a build directory:
    - ./abuild.py --agent <agent-name> --directory <build-dir> --claim-lock
	- Note: If this fails, it means the build directory is already claimed
	- Never try to claim an already-claimed directory.
  - Building: 
    - ./abuild.py --agent <agent-name> --directory <build-dir>
	- Note: Configure runs by default; use `--no-configure` only when intentionally reusing a configured build dir.
  - Releasing a build directory:
    - ./abuild.py --agent <agent-name> --directory <build-dir> --release-lock
- Note:
  - -a is an alias for --agent
  - -d is an alias for --directory

## Agent naming and build directories

- Overseers/managers should receive a name/ID and a build directory from the human operator.
- A specialist should receive a build names/ID and a build directory from the project manager/overseer.
- If you need to build and you have not been assigned a build name/id and/or a build directory, do not build. You must request and receive the neccessary information (id and directory) before building.


## SDK builds

- m-karma installs SDK to `<build-dir>/sdk` automatically on each build.
  - Optional override:
    - ./abuild.py -d <build-dir> --install-sdk <sdk-output-dir>
- m-bz3 builds must specify an SDK intake directory:
  - ./abuild.py -d <build-dir> --karma-sdk <karma-sdk-dir>
- These must align in practice: `<karma-sdk-dir>` should point to the m-karma SDK output for the producer build.
- Real-world example:
  - m-karma:
    - `./abuild.py -a <agent> -d build-sdk`
  - m-bz3:
    - `./abuild.py -a <agent> -d <build-dir> --karma-sdk ../m-karma/build-sdk/sdk`



### Advanced SDK builds

- static SDK contract: `./abuild.py -d <build-dir> --sdk-linkage static --install-sdk <prefix>`
- mobile shared override (explicit-only): `./abuild.py -d <build-dir> --sdk-linkage shared --mobile-allow-shared`

## Backends

- m-karma (engine/sdk) has a branched backend structure
- Different subsystems can choose which infrastructure they build around
  - physics: jolt/physx
  - audio: sdl3audio/miniaudio
  - renderer: bgfx/diligent
  - ui: imgui/rmlui
- abuild.py allows you to select which backends to build around.
- You can also include multiple backends in a single build, allowing for runtime backend selection.
- The default backend selection can be seen by running `./abuild.py --print-defaults`

### Enabling alternate/multiple backends

- Use `--backends` for building non-default backend options
- IMPORTANT: Most agents should not use `--backends`, and just use the default build configuration.
- Only use `--backends` when making changes that affect multiple backends
- Example usage:
  - Build using the diligent backend instead of the bgfx backend:
    - ./abuild.py -d <build-dir> -b diligent
  - Build allowing bgfx/diligent to be selected at runtime
    - ./abuild.py -d <build-dir> -b bgfx,diligent
  - Build overriding ui backend to use imgui and allowing bgfx/diligent to be selected at runtime
    - ./abuild.py -d <build-dir> -b imgui,bgfx,diligent

### Backend bugs

- Combined renderer mode (`-b bgfx,diligent`) is Linux shared-mode only.
- Non-Linux targets and static SDK linkage must select one renderer backend.
