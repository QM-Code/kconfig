#include <kconfig/store.hpp>
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/i18n.hpp>
#include <kconfig/trace.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main() {
    kconfig::InitializeTraceLogging();

    kconfig::common::data::SetDataRootOverride(std::filesystem::current_path());
    kconfig::common::config::ConfigStore::Initialize({});
    const auto probePath = kconfig::common::data::Resolve("demo/compile");
    (void)probePath;
    const std::string preview = kconfig::common::i18n::Get().formatText(
        "KConfig {state}",
        {{"state", "ready"}}
    );
    (void)preview;

    std::cout << "KConfig SDK compile/link/load check passed\n";
    return 0;
}
