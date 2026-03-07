#include <kconfig.hpp>

#include <ktrace.hpp>

#include <mutex>

namespace {

void RegisterKConfigChannels() {
    ktrace::RegisterChannel("config", ktrace::Color("DeepSkyBlue1"));
    ktrace::RegisterChannel("store", ktrace::Color("LightGoldenrod2"));
    ktrace::RegisterChannel("store.requests");
    ktrace::RegisterChannel("asset", ktrace::Color("SteelBlue1"));
    ktrace::RegisterChannel("asset.requests");
    ktrace::RegisterChannel("io", ktrace::Color("MediumSpringGreen"));
    ktrace::RegisterChannel("cli", ktrace::Color("Orange3"));
    ktrace::RegisterChannel("content", ktrace::Color("MediumOrchid1"));
}

std::once_flag g_trace_init_once;

} // namespace

namespace kconfig {

void Initialize() {
    std::call_once(g_trace_init_once, RegisterKConfigChannels);
}

} // namespace kconfig
