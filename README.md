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
# Uses kbuild.json "build-demos" order.
./kbuild.py --build-demos

./demo/executable/build/latest/test
```
Demos:
- Executable: `demo/compile/`

Demo builds are orchestrated by the root `kbuild.py`

Demo libraries demonstrate how other libraries can implement and expose ktrace.

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
