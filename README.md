# Karma JSON Config Storage SDK

JSON configuration/data/i18n/serialization SDK.

## Build SDK

```bash
./kbuild.py --build-latest
```

SDK output:
- `build/latest/sdk/include`
- `build/latest/sdk/lib`
- `build/latest/sdk/lib/cmake/KConfigSDK`

## Build and Test Demos

```bash
# Builds SDK plus kbuild.json "build.defaults.demos".
./kbuild.py --build-latest

# Explicit demo-only run (uses build.demos when no args are provided).
./kbuild.py --build-demos

./demo/exe/core/build/latest/test
```

Demos:
- Bootstrap compile/link check: `demo/bootstrap/`
- SDKs: `demo/sdk/{alpha,beta,gamma}`
- Executables: `demo/exe/{core,omega}`

Demo builds are orchestrated by the root `kbuild.py`.

The core demo validates the common load/merge/read path. The omega demo exercises the fuller config, i18n, user-store, and backing-file flows.

## Trace Integration

`kconfig` now follows the rewritten `ktrace` model:

- the library exposes `kconfig::GetTraceLogger()`
- executables own and activate a `ktrace::Logger`
- trace CLI remains global via `ktrace::GetInlineParser()`
- config CLI remains separate via `kconfig::cli::GetInlineParser()`

Executable integration looks like:

```cpp
ktrace::Logger logger;
logger.addTraceLogger(kconfig::GetTraceLogger());
logger.activate();

kcli::PrimaryParser parser;
parser.addInlineParser(ktrace::GetInlineParser());
parser.addInlineParser(kconfig::cli::GetInlineParser());
```

`kconfig.hpp` forward-declares `ktrace::TraceLogger` so non-tracing consumers do not
inherit the `KTRACE_NAMESPACE` requirement just by including KConfig headers.
Any translation unit that materializes a `ktrace::TraceLogger` from
`kconfig::GetTraceLogger()` should include `ktrace.hpp`.

## Coding Agents

If you are using a coding agent, paste the following prompt:

```bash
Follow the instructions in agent/BOOTSTRAP.md
```
