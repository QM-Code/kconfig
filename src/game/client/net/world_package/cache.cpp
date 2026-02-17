#include "client/net/world_package/internal.hpp"

#include "karma/common/content/cache_store.hpp"
#include "karma/common/data_path_resolver.hpp"
#include "karma/common/logging.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace bz3::client::net {

namespace {

karma::content::CachedContentIdentity ToContentIdentity(const CachedWorldIdentity& identity) {
    return karma::content::CachedContentIdentity{
        .world_hash = identity.world_hash,
        .world_content_hash = identity.world_content_hash,
        .world_id = identity.world_id,
        .world_revision = identity.world_revision};
}

CachedWorldIdentity FromContentIdentity(const karma::content::CachedContentIdentity& identity) {
    return CachedWorldIdentity{
        .world_hash = identity.world_hash,
        .world_content_hash = identity.world_content_hash,
        .world_id = identity.world_id,
        .world_revision = identity.world_revision};
}

} // namespace

bool HasCachedWorldPackageForServer(const std::string& host,
                                    uint16_t port,
                                    std::string_view world_id,
                                    std::string_view world_revision,
                                    std::string_view world_content_hash,
                                    std::string_view world_hash) {
    if (world_id.empty() || world_revision.empty()) {
        return false;
    }

    try {
        const std::filesystem::path server_cache_dir =
            karma::data::EnsureUserWorldDirectoryForServer(host, port);
        const std::string world_package_cache_key =
            ResolveWorldPackageCacheKey(world_content_hash, world_hash);
        const std::filesystem::path package_root = PackageRootForIdentity(server_cache_dir,
                                                                          world_id,
                                                                          world_revision,
                                                                          world_package_cache_key);
        return std::filesystem::exists(package_root) && std::filesystem::is_directory(package_root);
    } catch (const std::exception& ex) {
        spdlog::warn("ClientConnection: failed to query cached package for {}:{} id='{}' rev='{}': {}",
                     host,
                     port,
                     world_id,
                     world_revision,
                     ex.what());
        return false;
    }
}

void PruneWorldPackageCache(const std::filesystem::path& server_cache_dir,
                            std::string_view active_world_id,
                            std::string_view active_world_revision,
                            std::string_view active_world_package_key) {
    // Keep retention policy engine-owned here so server-delivered world config cannot influence
    // cache pruning behavior via runtime layers.
    const auto result = karma::content::PruneWorldPackageCache(WorldPackagesByWorldRoot(server_cache_dir),
                                                               active_world_id,
                                                               active_world_revision,
                                                               active_world_package_key,
                                                               kDefaultMaxRevisionsPerWorld,
                                                               kDefaultMaxPackagesPerRevision,
                                                               kMaxCachePathComponentLen);

    for (const auto& warning : result.warnings) {
        switch (warning.kind) {
        case karma::content::CachePruneWarningKind::PrunePackage:
            spdlog::warn("ClientConnection: failed to prune cached world package '{}': {}",
                         warning.path.string(),
                         warning.message);
            break;
        case karma::content::CachePruneWarningKind::RemoveEmptyRevision:
            spdlog::warn("ClientConnection: failed to remove empty cached revision '{}': {}",
                         warning.path.string(),
                         warning.message);
            break;
        case karma::content::CachePruneWarningKind::PruneRevision:
            spdlog::warn("ClientConnection: failed to prune cached world revision '{}': {}",
                         warning.path.string(),
                         warning.message);
            break;
        case karma::content::CachePruneWarningKind::RemoveEmptyWorldDir:
            spdlog::warn("ClientConnection: failed to remove empty cached world dir '{}': {}",
                         warning.path.string(),
                         warning.message);
            break;
        }
    }

    for (const auto& path : result.pruned_package_paths) {
        KARMA_TRACE("net.client",
                    "ClientConnection: pruned cached world package '{}'",
                    path.string());
    }
    for (const auto& path : result.pruned_revision_paths) {
        KARMA_TRACE("net.client",
                    "ClientConnection: pruned cached world revision '{}'",
                    path.string());
    }

    KARMA_TRACE("net.client",
                "ClientConnection: cache prune summary worlds={} revisions={} packages={} pruned_revisions={} pruned_packages={}",
                result.scanned_world_dirs,
                result.scanned_revision_dirs,
                result.scanned_package_dirs,
                result.pruned_revision_dirs,
                result.pruned_package_dirs);
}

bool HasPackageIdentity(const CachedWorldIdentity& identity) {
    return karma::content::HasPackageIdentity(ToContentIdentity(identity));
}

bool HasRequiredIdentityFields(const CachedWorldIdentity& identity) {
    return karma::content::HasRequiredIdentityFields(ToContentIdentity(identity));
}

std::optional<CachedWorldIdentity> ReadCachedWorldIdentityFile(const std::filesystem::path& identity_file) {
    const auto identity = karma::content::ReadCachedIdentityFile(identity_file);
    if (!identity.has_value()) {
        return std::nullopt;
    }
    return FromContentIdentity(*identity);
}

std::optional<CachedWorldIdentity> ReadCachedWorldIdentityForServer(const std::string& host, uint16_t port) {
    try {
        const auto server_cache_dir = karma::data::EnsureUserWorldDirectoryForServer(host, port);
        const auto identity_file = ActiveWorldIdentityPath(server_cache_dir);
        const auto identity = ReadCachedWorldIdentityFile(identity_file);
        if (!identity.has_value()) {
            std::error_code ec;
            std::filesystem::remove(identity_file, ec);
            KARMA_TRACE("net.client",
                        "ClientConnection: cached world identity ignored for {}:{} (missing/incomplete identity metadata)",
                        host,
                        port);
            return std::nullopt;
        }
        return identity.value();
    } catch (const std::exception& ex) {
        spdlog::warn("ClientConnection: failed to read cached world identity for {}:{}: {}",
                     host,
                     port,
                     ex.what());
        return std::nullopt;
    }
}

bool PersistCachedWorldIdentity(const std::filesystem::path& server_cache_dir,
                                std::string_view world_hash,
                                std::string_view world_content_hash,
                                std::string_view world_id,
                                std::string_view world_revision) {
    return karma::content::PersistCachedIdentityFile(ActiveWorldIdentityPath(server_cache_dir),
                                                     world_hash,
                                                     world_content_hash,
                                                     world_id,
                                                     world_revision);
}

std::optional<CachedWorldIdentity> ReadCachedWorldIdentity(const std::filesystem::path& server_cache_dir) {
    return ReadCachedWorldIdentityFile(ActiveWorldIdentityPath(server_cache_dir));
}

bool ValidateCachedWorldIdentity(const std::filesystem::path& server_cache_dir,
                                 std::string_view world_name,
                                 std::string_view expected_world_hash,
                                 std::string_view expected_world_content_hash,
                                 std::string_view expected_world_id,
                                 std::string_view expected_world_revision,
                                 std::string_view expected_world_manifest_hash,
                                 uint32_t expected_world_manifest_file_count,
                                 bool require_exact_revision) {
    const auto identity = ReadCachedWorldIdentity(server_cache_dir);
    if (!identity.has_value()) {
        spdlog::error("ClientConnection: cache identity metadata is missing for world '{}'", world_name);
        return false;
    }

    const bool id_match = identity->world_id == expected_world_id;
    const bool revision_match = identity->world_revision == expected_world_revision;
    const bool hash_match = !expected_world_hash.empty() && identity->world_hash == expected_world_hash;
    const bool content_hash_match = !expected_world_content_hash.empty() &&
                                    identity->world_content_hash == expected_world_content_hash;
    bool manifest_match = false;
    uint32_t cached_manifest_file_count = 0;
    std::string cached_manifest_hash{};
    if (!expected_world_manifest_hash.empty()) {
        const auto cached_manifest = ReadCachedWorldManifest(server_cache_dir);
        cached_manifest_file_count = static_cast<uint32_t>(cached_manifest.size());
        cached_manifest_hash = ComputeManifestHash(cached_manifest);
        manifest_match = cached_manifest_hash == expected_world_manifest_hash &&
                         cached_manifest_file_count == expected_world_manifest_file_count;
    }

    const bool package_match = hash_match || content_hash_match || manifest_match;
    if (!id_match || (require_exact_revision && !revision_match) || !package_match) {
        spdlog::error("ClientConnection: cache identity mismatch for world '{}' (expected hash='{}' content_hash='{}' id='{}' rev='{}' manifest_hash='{}' manifest_files={} require_exact_revision={}, got hash='{}' content_hash='{}' id='{}' rev='{}' manifest_hash='{}' manifest_files={})",
                      world_name,
                      expected_world_hash,
                      expected_world_content_hash,
                      expected_world_id,
                      expected_world_revision,
                      expected_world_manifest_hash,
                      expected_world_manifest_file_count,
                      require_exact_revision ? 1 : 0,
                      identity->world_hash,
                      identity->world_content_hash,
                      identity->world_id,
                      identity->world_revision,
                      cached_manifest_hash,
                      cached_manifest_file_count);
        return false;
    }
    return true;
}

void ClearCachedWorldIdentity(const std::filesystem::path& server_cache_dir) {
    static_cast<void>(PersistCachedWorldIdentity(server_cache_dir, "", "", "", ""));
    static_cast<void>(PersistCachedWorldManifest(server_cache_dir, {}));
}

} // namespace bz3::client::net
