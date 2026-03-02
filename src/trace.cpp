#include <kconfig/trace.hpp>

#include <ktrace/trace.hpp>

#include <mutex>

namespace {

void RegisterKConfigChannels() {
    ktrace::RegisterChannel("config", ktrace::Color("DeepSkyBlue1"));
    ktrace::RegisterChannel("config.requests", ktrace::Color("LightGoldenrod2"));
    ktrace::RegisterChannel("data", ktrace::Color("MediumSpringGreen"));
    ktrace::RegisterChannel("cli", ktrace::Color("Orange3"));
    ktrace::RegisterChannel("content", ktrace::Color("MediumOrchid1"));

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
