#include "kconfig/trace.hpp"

#include <ktrace/trace.hpp>

#include <mutex>

namespace {

void RegisterKConfigChannels() {
    ktrace::RegisterChannel("config", ktrace::ResolveColor("DeepSkyBlue1"));
    ktrace::RegisterChannel("config.requests", ktrace::ResolveColor("LightGoldenrod2"));
    ktrace::RegisterChannel("data", ktrace::ResolveColor("MediumSpringGreen"));
    ktrace::RegisterChannel("cli", ktrace::ResolveColor("Orange3"));
    ktrace::RegisterChannel("content", ktrace::ResolveColor("MediumOrchid1"));

    ktrace::EnableChannel("kconfig.config");
    ktrace::EnableChannel("kconfig.config.requests");
    ktrace::EnableChannel("kconfig.data");
    ktrace::EnableChannel("kconfig.cli");
    ktrace::EnableChannel("kconfig.content");
}

std::once_flag g_trace_init_once;

} // namespace

namespace kconfig {

void InitializeTraceLogging() {
    std::call_once(g_trace_init_once, RegisterKConfigChannels);
}

} // namespace kconfig
