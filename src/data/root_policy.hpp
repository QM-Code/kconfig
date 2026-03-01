#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace kconfig::common::data {

struct DataPathSpec;

// Root resolution precedence:
// 1) cli_root
// 2) env_var (when set and non-empty)
// 3) fallback_root
//
// Returns std::nullopt when no source is available.
// Throws std::runtime_error when a selected root is invalid or missing marker.
struct RootPathPolicy {
    std::optional<std::filesystem::path> cli_root{};
    std::string env_var{};
    std::optional<std::filesystem::path> fallback_root{};
    std::optional<std::filesystem::path> required_marker{};
};

std::optional<std::filesystem::path> ResolveRootPathPolicy(const RootPathPolicy& policy);

// Shared data-root setup contract used by tools/tests:
// - configure DataPathSpec first,
// - resolve root by policy precedence (cli -> env),
// - install resolver override only when a root source is available.
void ConfigureDataRootPolicy(const DataPathSpec& spec,
                             const std::optional<std::filesystem::path>& cli_root = std::nullopt);

} // namespace kconfig::common::data
