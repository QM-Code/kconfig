#include <kconfig.hpp>
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/i18n.hpp>
#include <kconfig/store.hpp>
#include <ktrace.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    kconfig::Initialize();
    ktrace::ProcessCLI(argc, argv, "trace");
    kconfig::ParseCLI(argc, argv, "config");

    const std::filesystem::path repoRoot = std::filesystem::current_path();
    const std::filesystem::path runtimeRoot = repoRoot / "demo" / "compile" / "runtime";
    const std::filesystem::path defaultsPath = runtimeRoot / "defaults.json";
    const std::filesystem::path userPath = runtimeRoot / "user.json";

    if (!kconfig::store::LoadReadOnly("defaults", defaultsPath)) {
        std::cerr << "Failed to load defaults config: " << defaultsPath << "\n";
        return 1;
    }
    if (!kconfig::store::LoadMutable("user", userPath)) {
        std::cerr << "Failed to load user config: " << userPath << "\n";
        return 1;
    }

    try {
        const bool enabled = kconfig::store::ReadRequiredBool("feature.enabled");
        const uint16_t port = kconfig::store::ReadRequiredUInt16("network.port");
        const uint16_t retries = kconfig::store::ReadRequiredPositiveUInt16("positive.u16");
        const uint32_t maxSessions = kconfig::store::ReadRequiredUInt32("numbers.u32");
        const float timeout = kconfig::store::ReadRequiredFloat("limits.timeout");
        const float positiveScale = kconfig::store::ReadRequiredPositiveFiniteFloat("positive.float");
        const std::string language = kconfig::store::ReadRequiredNonEmptyString("client.Language");
        const std::string name = kconfig::store::ReadRequiredString("text.name");
        const std::vector<float> weights = kconfig::store::ReadRequiredFloatArray("values.weights");

        std::cout << std::boolalpha
                  << "ReadRequired demo: enabled=" << enabled
                  << ", port=" << port
                  << ", retries=" << retries
                  << ", maxSessions=" << maxSessions
                  << ", timeout=" << timeout
                  << ", scale=" << positiveScale
                  << ", language=" << language
                  << ", name=" << name
                  << ", weights=[";
        for (std::size_t i = 0; i < weights.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << weights[i];
        }
        std::cout << "]\n";
    } catch (const std::exception& ex) {
        std::cerr << "ReadRequired demo failed: " << ex.what() << "\n";
        return 1;
    }

    kconfig::data::path_resolver::SetDataRootOverride(repoRoot);
    const auto probePath = kconfig::data::path_resolver::Resolve("demo/compile");
    (void)probePath;
    const std::string preview = kconfig::i18n::Get().formatText(
        "KConfig {state}",
        {{"state", "ready"}}
    );
    (void)preview;

    std::cout << "KConfig SDK compile/link/load check passed\n";
    return 0;
}
