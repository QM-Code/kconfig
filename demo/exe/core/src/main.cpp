#include <alpha/sdk.hpp>
#include <kconfig.hpp>
#include <kconfig/store.hpp>
#include <ktrace.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    kconfig::Initialize();
    ktrace::RegisterChannel("store");
    ktrace::RegisterChannel("store.requests");
    ktrace::ProcessCLI(argc, argv, "trace");
    kconfig::cli::ParseArgs(argc, argv, "config");

    const std::filesystem::path repoRoot = std::filesystem::current_path();
    const std::filesystem::path runtimeRoot = repoRoot / "demo" / "exe" / "core" / "runtime";
    const std::filesystem::path defaultsPath = runtimeRoot / "defaults.json";
    const std::filesystem::path userPath = runtimeRoot / "user.json";
    const std::filesystem::path sessionPath = runtimeRoot / "session.json";

    KTRACE("store.requests",
           "core demo preparing config load defaults='{}' user='{}' session='{}'",
           defaultsPath.string(),
           userPath.string(),
           sessionPath.string());

    std::string ioError;

    if (!kconfig::store::fs::LoadReadOnly("defaults", defaultsPath, &ioError)) {
        std::cerr << "Failed to load defaults config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    if (!kconfig::store::fs::LoadMutable("user", userPath, &ioError)) {
        std::cerr << "Failed to load user config: " << ioError << "\n";
        return 1;
    }

    ioError.clear();
    if (!kconfig::store::fs::LoadReadOnly("session", sessionPath, &ioError)) {
        std::cerr << "Failed to load session config: " << ioError << "\n";
        return 1;
    }

    std::vector<std::string> mergeSources = {"defaults", "user", "session"};
    if (kconfig::store::Has("config")) {
        KTRACE("store.requests", "core demo detected CLI namespace 'config' -> append to merge sources");
        mergeSources.push_back("config");
    }
    if (!kconfig::store::Merge("merged", mergeSources)) {
        std::cerr << "Failed to merge config namespaces: defaults + user + session -> merged\n";
        return 1;
    }

    try {
        const bool enabled = kconfig::store::read::RequiredBool("merged.feature.enabled");
        const uint16_t port = kconfig::store::read::RequiredUint16("merged.network.port");
        const std::string name = kconfig::store::read::RequiredString("merged.text.name");

        std::cout << std::boolalpha
                  << "Core config demo: enabled=" << enabled
                  << ", port=" << port
                  << ", name=" << name << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Core config demo failed: " << ex.what() << "\n";
        return 1;
    }

    kconfig::demo::alpha::EmitDemoOutput();

    std::cout << "KConfig demo core compile/link/integration check passed\n";
    return 0;
}
