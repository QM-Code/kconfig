# m-kconfig

Standalone configuration/data/i18n/serialization SDK extracted from `m-karma`.

Public headers are installed under:

- `include/kconfig`
- `include/kconfig/data`

Trace setup API for consumers:

- `#include <kconfig/trace.hpp>`
- `kconfig::InitializeTraceLogging()`

When built via `abuild.py`, these public headers are installed to:

- `<build-dir>/sdk/include/kconfig/...`

The CMake package exports target:

- `kconfig::sdk` via `find_package(KConfigSDK CONFIG REQUIRED)`

## Build

```bash
./abuild.py -a <name> -d build/test
```

Default SDK install output:

- `build/test/sdk/include`
- `build/test/sdk/lib`
- `build/test/sdk/lib/cmake/KConfigSDK`
