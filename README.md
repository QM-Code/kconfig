# Karma JSON Config Storage SDK

Standalone JSON configuration/data/i18n/serialization SDK.

## Build SDK

```bash
./kbuild.py
```

SDK output:
- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/KConfigSDK`

## Build and Test Demos

```bash
# Builds SDK plus kbuild.json "build.defaults.demos".
./kbuild.py

# Explicit demo-only run (uses build.demos when no args are provided).
./kbuild.py --build-demos

./demo/executable/build/latest/test
```

Demos:
- Bootstrap compile/link check: `demo/bootstrap/`
- Libraries: `demo/libraries/{alpha,beta,gamma}`
- Executable: `demo/executable/`

Demo builds are orchestrated by the root `kbuild.py`.

Demo executable validates KConfigSDK compile/link/load behavior while consuming demo libraries.

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
