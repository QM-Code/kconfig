#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace kconfig::common::data {

struct DataPathSpec;

struct RootPathPolicy {
    std::optional<std::filesystem::path> cli_root{};
    std::string env_var{};
    std::optional<std::filesystem::path> fallback_root{};
    std::optional<std::filesystem::path> required_marker{};
};

std::optional<std::filesystem::path> ResolveRootPathPolicy(const RootPathPolicy& policy);

void ConfigureDataRootPolicy(const DataPathSpec& spec,
                             const std::optional<std::filesystem::path>& cli_root = std::nullopt);

} // namespace kconfig::common::data
