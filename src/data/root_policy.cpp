#include "data/root_policy.hpp"
#include "data/path_utils.hpp"
#include "data/path_resolver.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

bool HasValue(const std::optional<std::filesystem::path>& value) {
    return value.has_value() && !value->empty();
}

std::filesystem::path ValidateRootPath(const std::filesystem::path& root,
                                       const std::optional<std::filesystem::path>& required_marker) {
    const std::filesystem::path canonical_root = kconfig::common::data::path_utils::Canonicalize(root);
    std::error_code ec;
    if (!std::filesystem::exists(canonical_root, ec) || !std::filesystem::is_directory(canonical_root, ec)) {
        throw std::runtime_error("root_policy: invalid root directory: " + canonical_root.string());
    }

    if (required_marker.has_value() && !required_marker->empty()) {
        const std::filesystem::path marker_path = canonical_root / *required_marker;
        if (!std::filesystem::exists(marker_path, ec) || !std::filesystem::is_regular_file(marker_path, ec)) {
            throw std::runtime_error("root_policy: missing required marker: " + marker_path.string());
        }
    }

    return canonical_root;
}

std::optional<std::filesystem::path> RootPathFromEnvironment(const std::string& env_var) {
    if (env_var.empty()) {
        return std::nullopt;
    }

    const char* value = std::getenv(env_var.c_str());
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }

    return std::filesystem::path(value);
}

} // namespace

namespace kconfig::common::data {

std::optional<std::filesystem::path> ResolveRootPathPolicy(const RootPathPolicy& policy) {
    if (HasValue(policy.cli_root)) {
        return ValidateRootPath(*policy.cli_root, policy.required_marker);
    }

    if (const auto env_root = RootPathFromEnvironment(policy.env_var)) {
        return ValidateRootPath(*env_root, policy.required_marker);
    }

    if (HasValue(policy.fallback_root)) {
        return ValidateRootPath(*policy.fallback_root, policy.required_marker);
    }

    return std::nullopt;
}

void ConfigureDataRootPolicy(const DataPathSpec& spec, const std::optional<std::filesystem::path>& cli_root) {
    SetDataPathSpec(spec);

    RootPathPolicy policy{};
    policy.cli_root = cli_root;
    policy.env_var = spec.dataDirEnvVar;
    if (!spec.requiredDataMarker.empty()) {
        policy.required_marker = spec.requiredDataMarker;
    }

    if (const auto resolved_root = ResolveRootPathPolicy(policy)) {
        SetDataRootOverride(*resolved_root);
    }
}

} // namespace kconfig::common::data
