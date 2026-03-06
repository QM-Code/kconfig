#include <alpha/sdk.hpp>
#include <beta/sdk.hpp>
#include <gamma/sdk.hpp>
#include <kconfig.hpp>
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/i18n.hpp>
#include <kconfig/store.hpp>
#include <ktrace.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

int main(int argc, char** argv) {
    kconfig::Initialize();
    ktrace::ProcessCLI(argc, argv, "trace");
    kconfig::ParseCLI(argc, argv, "config");

    const std::filesystem::path repoRoot = std::filesystem::current_path();
    const std::filesystem::path runtimeRoot = repoRoot / "demo" / "executable" / "runtime";
    const std::filesystem::path defaultsPath = runtimeRoot / "defaults.json";
    const std::filesystem::path userPath = runtimeRoot / "user.json";
    const std::filesystem::path sessionPath = runtimeRoot / "session.json";

    KTRACE("store.requests",
           "demo executable preparing config load defaults='{}' user='{}' session='{}'",
           defaultsPath.string(),
           userPath.string(),
           sessionPath.string());

    KTRACE("store.requests", "LoadReadOnly('defaults', '{}')", defaultsPath.string());
    if (!kconfig::store::LoadReadOnly("defaults", defaultsPath)) {
        std::cerr << "Failed to load defaults config: " << defaultsPath << "\n";
        return 1;
    }
    if (const char* userConfigDirname = std::getenv("KCONFIG_DEMO_USER_CONFIG_DIRNAME");
        userConfigDirname != nullptr && *userConfigDirname != '\0') {
        KTRACE("store.requests", "SetUserConfigDirname('{}')", userConfigDirname);
        if (!kconfig::store::SetUserConfigDirname(userConfigDirname)) {
            std::cerr << "Failed to set user config dirname: " << userConfigDirname << "\n";
            return 1;
        }

        if (!kconfig::store::HasUserConfigFile()) {
            std::string initError;
            KTRACE("store.requests", "InitializeUserConfigFile(object)");
            if (!kconfig::store::InitializeUserConfigFile(kconfig::json::Object(), &initError)) {
                std::cerr << "Failed to initialize user config file: " << initError << "\n";
                return 1;
            }
        }

        kconfig::store::LoadUserConfigFileOptions userLoadOptions{};
        userLoadOptions.mode = kconfig::store::UserConfigLoadMode::Mutable;
        std::string userLoadError;
        KTRACE("store.requests", "LoadUserConfigFile('user', mode=Mutable)");
        if (!kconfig::store::LoadUserConfigFile("user", userLoadOptions, &userLoadError)) {
            std::cerr << "Failed to load user config file via user-config API: " << userLoadError << "\n";
            return 1;
        }
    } else {
        KTRACE("store.requests", "LoadMutable('user', '{}')", userPath.string());
        if (!kconfig::store::LoadMutable("user", userPath)) {
            std::cerr << "Failed to load user config: " << userPath << "\n";
            return 1;
        }
    }
    KTRACE("store.requests", "LoadReadOnly('session', '{}')", sessionPath.string());
    if (!kconfig::store::LoadReadOnly("session", sessionPath)) {
        std::cerr << "Failed to load session config: " << sessionPath << "\n";
        return 1;
    }
    std::vector<std::string> mergeSources = {"defaults", "user", "session"};
    if (kconfig::store::Get("config", ".")) {
        KTRACE("store.requests",
               "demo executable detected CLI namespace 'config' -> append to merge sources");
        mergeSources.push_back("config");
    }
    KTRACE("store.requests", "Merge('merged', dynamic-sources)");
    if (!kconfig::store::Merge("merged", mergeSources)) {
        std::cerr << "Failed to merge config namespaces: defaults + user + session -> merged\n";
        return 1;
    }

    const auto shouldRunBackingRoundTrip = []() {
        const char* env = std::getenv("KCONFIG_DEMO_TEST_BACKING");
        if (env == nullptr || *env == '\0') {
            return false;
        }
        std::string token(env);
        std::transform(token.begin(),
                       token.end(),
                       token.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return token != "0" && token != "false" && token != "no" && token != "off";
    };

    if (shouldRunBackingRoundTrip()) {
        KTRACE("store.requests", "Backing roundtrip test enabled via KCONFIG_DEMO_TEST_BACKING");

        std::ifstream originalInput(userPath, std::ios::binary);
        if (!originalInput) {
            std::cerr << "Backing roundtrip failed: unable to read original user config file\n";
            return 1;
        }
        std::ostringstream originalBuffer;
        originalBuffer << originalInput.rdbuf();
        const std::string originalUserFileText = originalBuffer.str();

        const auto restoreUserFile = [&]() {
            std::ofstream restoreOut(userPath, std::ios::binary | std::ios::trunc);
            if (!restoreOut) {
                return false;
            }
            restoreOut << originalUserFileText;
            return static_cast<bool>(restoreOut);
        };

        auto failBacking = [&](const std::string& message) {
            std::cerr << "Backing roundtrip failed: " << message << "\n";
            if (!restoreUserFile()) {
                std::cerr << "Backing roundtrip failed: unable to restore original user config file\n";
            }
            std::string reloadError;
            (void)kconfig::store::ReloadBackingFile("user", &reloadError);
            return 1;
        };

        std::string ioError;
        const std::string sentinel = "backing-write-sentinel";
        KTRACE("store.requests", "Set('user', 'text.name', '{}')", sentinel);
        if (!kconfig::store::Set("user", "text.name", sentinel)) {
            return failBacking("Set('user', 'text.name', sentinel) returned false");
        }
        KTRACE("store.requests", "SetSaveIntervalSeconds('user', 0.0)");
        if (!kconfig::store::SetSaveIntervalSeconds("user", 0.0)) {
            return failBacking("SetSaveIntervalSeconds('user', 0.0) returned false");
        }
        KTRACE("store.requests", "FlushWrites()");
        if (!kconfig::store::FlushWrites(&ioError)) {
            return failBacking("FlushWrites() returned false: " + ioError);
        }
        KTRACE("store.requests", "Set('user', 'text.name', 'memory-only-temp')");
        if (!kconfig::store::Set("user", "text.name", "memory-only-temp")) {
            return failBacking("Set('user', 'text.name', memory-only-temp) returned false");
        }
        ioError.clear();
        KTRACE("store.requests", "ReloadBackingFile('user')");
        if (!kconfig::store::ReloadBackingFile("user", &ioError)) {
            return failBacking("ReloadBackingFile('user') returned false: " + ioError);
        }

        const auto roundTripName = kconfig::store::Get("user", "text.name");
        if (!roundTripName || !roundTripName->is_string() || roundTripName->get<std::string>() != sentinel) {
            return failBacking("reloaded user.text.name did not match sentinel");
        }

        if (!restoreUserFile()) {
            std::cerr << "Backing roundtrip failed: unable to restore original user config file\n";
            return 1;
        }
        ioError.clear();
        KTRACE("store.requests", "ReloadBackingFile('user') after restore");
        if (!kconfig::store::ReloadBackingFile("user", &ioError)) {
            std::cerr << "Backing roundtrip failed: ReloadBackingFile after restore returned false: "
                      << ioError << "\n";
            return 1;
        }

        std::cout << "Backing file roundtrip check passed\n";
    }

    try {
        KTRACE("store.requests", "ReadRequiredBool('merged.feature.enabled')");
        const bool enabled = kconfig::store::ReadRequiredBool("merged.feature.enabled");
        KTRACE("store.requests", "ReadRequiredUInt16('merged.network.port')");
        const uint16_t port = kconfig::store::ReadRequiredUInt16("merged.network.port");
        KTRACE("store.requests", "ReadRequiredPositiveUInt16('merged.positive.u16')");
        const uint16_t retries = kconfig::store::ReadRequiredPositiveUInt16("merged.positive.u16");
        KTRACE("store.requests", "ReadRequiredUInt32('merged.numbers.u32')");
        const uint32_t maxSessions = kconfig::store::ReadRequiredUInt32("merged.numbers.u32");
        KTRACE("store.requests", "ReadRequiredFloat('merged.limits.timeout')");
        const float timeout = kconfig::store::ReadRequiredFloat("merged.limits.timeout");
        KTRACE("store.requests", "ReadRequiredPositiveFiniteFloat('merged.positive.float')");
        const float positiveScale = kconfig::store::ReadRequiredPositiveFiniteFloat("merged.positive.float");
        KTRACE("store.requests", "ReadRequiredNonEmptyString('merged.client.Language')");
        const std::string language = kconfig::store::ReadRequiredNonEmptyString("merged.client.Language");
        KTRACE("store.requests", "ReadRequiredString('merged.text.name')");
        const std::string name = kconfig::store::ReadRequiredString("merged.text.name");
        KTRACE("store.requests", "ReadRequiredFloatArray('merged.values.weights')");
        const std::vector<float> weights = kconfig::store::ReadRequiredFloatArray("merged.values.weights");

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
    const auto probePath = kconfig::data::path_resolver::Resolve("demo/executable");
    (void)probePath;
    const std::string preview = kconfig::i18n::Get().formatText(
        "KConfig {state}",
        {{"state", "ready"}}
    );
    (void)preview;

    kconfig::demo::alpha::EmitDemoOutput();
    kconfig::demo::beta::EmitDemoOutput();
    kconfig::demo::gamma::EmitDemoOutput();

    std::cout << "KConfig demo executable compile/link/integration check passed\n";
    return 0;
}
