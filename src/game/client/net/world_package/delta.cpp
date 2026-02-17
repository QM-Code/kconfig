#include "client/net/world_package/internal.hpp"

#include "karma/common/content/package_apply.hpp"
#include "karma/common/logging.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <string>
#include <vector>

namespace bz3::client::net {

namespace {

std::vector<karma::content::ManifestEntry> ToContentManifest(
    const std::vector<bz3::net::WorldManifestEntry>& manifest) {
    std::vector<karma::content::ManifestEntry> converted{};
    converted.reserve(manifest.size());
    for (const auto& entry : manifest) {
        converted.push_back(karma::content::ManifestEntry{
            .path = entry.path,
            .size = entry.size,
            .hash = entry.hash});
    }
    return converted;
}

} // namespace

bool ApplyDeltaArchiveOverCachedBase(const std::filesystem::path& server_cache_dir,
                                     std::string_view world_name,
                                     std::string_view world_id,
                                     std::string_view world_revision,
                                     std::string_view world_hash,
                                     std::string_view world_content_hash,
                                     std::string_view world_manifest_hash,
                                     uint32_t world_manifest_file_count,
                                     const std::vector<bz3::net::WorldManifestEntry>& world_manifest,
                                     std::string_view base_world_id,
                                     std::string_view base_world_revision,
                                     std::string_view base_world_hash,
                                     std::string_view base_world_content_hash,
                                     const std::vector<std::byte>& delta_archive) {
    if (base_world_id.empty() || base_world_revision.empty()) {
        spdlog::error("ClientConnection: delta transfer missing base world identity metadata");
        return false;
    }
    if (base_world_id != world_id) {
        spdlog::error("ClientConnection: delta transfer base world id mismatch target='{}' base='{}'",
                      world_id,
                      base_world_id);
        return false;
    }

    const std::string target_package_cache_key = ResolveWorldPackageCacheKey(world_content_hash, world_hash);
    const std::filesystem::path target_root =
        PackageRootForIdentity(server_cache_dir, world_id, world_revision, target_package_cache_key);
    const std::string base_package_cache_key =
        ResolveWorldPackageCacheKey(base_world_content_hash, base_world_hash);
    const std::filesystem::path base_root = PackageRootForIdentity(server_cache_dir,
                                                                   base_world_id,
                                                                   base_world_revision,
                                                                   base_package_cache_key);
    if (!std::filesystem::exists(base_root) || !std::filesystem::is_directory(base_root)) {
        spdlog::error("ClientConnection: delta base world package is missing '{}' for id='{}' rev='{}'",
                      base_root.string(),
                      base_world_id,
                      base_world_revision);
        return false;
    }

    size_t removed_paths = 0;
    if (!karma::content::ApplyDeltaArchiveOverBasePackage(target_root,
                                                          base_root,
                                                          world_name,
                                                          world_content_hash,
                                                          world_manifest_hash,
                                                          world_manifest_file_count,
                                                          ToContentManifest(world_manifest),
                                                          delta_archive,
                                                          "ClientConnection",
                                                          &removed_paths)) {
        return false;
    }

    KARMA_TRACE("net.client",
                "ClientConnection: applied world delta archive target='{}' base='{}' removed_paths={}",
                target_root.string(),
                base_root.string(),
                removed_paths);
    return true;
}


} // namespace bz3::client::net
