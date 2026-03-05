#include <gamma/sdk.hpp>

#include <kconfig.hpp>

#include <iostream>

namespace {

bool g_initialized = false;

} // namespace

namespace kconfig::demo::gamma {

void Initialize() {
    if (!g_initialized) {
        kconfig::Initialize();
        g_initialized = true;
    }
}

void EmitDemoOutput() {
    Initialize();
    std::cout << "[gamma] kconfig demo library initialized\n";
}

} // namespace kconfig::demo::gamma
