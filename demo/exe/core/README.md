# Core Demo

Basic load, merge, read, and integration showcase for KConfigSDK and the alpha demo SDK.

Trace integration in this demo matches the rewritten `ktrace` API:

- local demo channels are declared with a `ktrace::TraceLogger`
- `kconfig::GetTraceLogger()` is added to the executable-owned `ktrace::Logger`
- the logger is activated before CLI parsing
- trace CLI uses global `ktrace::GetInlineParser()`
- config CLI uses `kconfig::cli::GetInlineParser()`
