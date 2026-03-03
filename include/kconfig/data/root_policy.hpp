#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace kconfig::data::path_resolver {
struct DataPathSpec;
}

namespace kconfig::data::root_policy {

struct RootPathPolicy {
    std::optional<std::filesystem::path> cli_root{};
    std::string env_var{};
    std::optional<std::filesystem::path> fallback_root{};
    std::optional<std::filesystem::path> required_marker{};
};

std::optional<std::filesystem::path> ResolveRootPathPolicy(const RootPathPolicy& policy);

void ConfigureDataRootPolicy(const path_resolver::DataPathSpec& spec,
                             const std::optional<std::filesystem::path>& cli_root = std::nullopt);

} // namespace kconfig::data::root_policy
