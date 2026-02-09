#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "server/cli_options.hpp"

namespace bz3::server::domain {

struct WorldManifestEntry {
    std::string path{};
    uint64_t size = 0;
    std::string hash{};
};

struct WorldSessionContext {
    std::string world_name{};
    std::filesystem::path world_dir{};
    std::filesystem::path world_config_path{};
    bool world_package_enabled = false;
    std::vector<std::byte> world_package{};
    std::string world_package_hash{};
    uint64_t world_package_size = 0;
    std::string world_content_hash{};
    std::string world_manifest_hash{};
    uint32_t world_manifest_file_count = 0;
    std::vector<WorldManifestEntry> world_manifest{};
    std::string world_id{};
    std::string world_revision{};
};

std::optional<WorldSessionContext> LoadWorldSessionContext(const CLIOptions& options);

} // namespace bz3::server::domain
