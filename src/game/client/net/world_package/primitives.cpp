#include "client/net/world_package/internal.hpp"

#include "karma/common/content/cache_store.hpp"
#include "karma/common/content/package_apply.hpp"
#include "karma/common/content/primitives.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace bz3::client::net {

std::filesystem::path ActiveWorldIdentityPath(const std::filesystem::path& server_cache_dir) {
    return server_cache_dir / kWorldIdentityFile;
}

std::filesystem::path ActiveWorldManifestPath(const std::filesystem::path& server_cache_dir) {
    return server_cache_dir / kWorldManifestFile;
}

std::filesystem::path WorldPackagesByWorldRoot(const std::filesystem::path& server_cache_dir) {
    return server_cache_dir / kWorldPackagesDir / kWorldPackagesByWorldDir;
}

uint64_t HashStringFNV1a(std::string_view value) {
    return karma::content::HashStringFNV1a(value);
}

void HashStringFNV1a(uint64_t& hash, std::string_view value) {
    karma::content::HashStringFNV1a(hash, value);
}

void HashBytesFNV1a(uint64_t& hash, const std::byte* bytes, size_t count) {
    karma::content::HashBytesFNV1a(hash, bytes, count);
}

void HashBytesFNV1a(uint64_t& hash, std::string_view value) {
    karma::content::HashBytesFNV1a(hash, value);
}

void HashSeparatorFNV1a(uint64_t& hash) {
    karma::content::HashSeparatorFNV1a(hash);
}

std::string Hash64Hex(uint64_t hash) {
    return karma::content::Hash64Hex(hash);
}

void HashChunkChainFNV1a(uint64_t& hash, uint32_t chunk_index, const std::vector<std::byte>& chunk_data) {
    karma::content::HashChunkChainFNV1a(hash, chunk_index, chunk_data);
}

bool InitIncludesWorldMetadata(const bz3::net::ServerMessage& message) {
    return !message.world_data.empty() ||
           message.world_size > 0 ||
           !message.world_hash.empty() ||
           !message.world_content_hash.empty() ||
           !message.world_manifest_hash.empty() ||
           message.world_manifest_file_count > 0 ||
           !message.world_manifest.empty();
}

bool IsChunkInTransferBounds(uint64_t total_bytes,
                             uint32_t chunk_size,
                             uint32_t chunk_index,
                             size_t chunk_bytes) {
    return karma::content::IsChunkInTransferBounds(total_bytes, chunk_size, chunk_index, chunk_bytes);
}

bool ChunkMatchesBufferedPayload(const std::vector<std::byte>& payload,
                                 size_t chunk_offset,
                                 const std::vector<std::byte>& chunk_data) {
    return karma::content::ChunkMatchesBufferedPayload(payload, chunk_offset, chunk_data);
}

std::string SanitizeCachePathComponent(std::string_view input, std::string_view fallback_prefix) {
    return karma::content::SanitizeCachePathComponent(input,
                                                      fallback_prefix,
                                                      kMaxCachePathComponentLen);
}

std::filesystem::path PackageRootForIdentity(const std::filesystem::path& server_cache_dir,
                                             std::string_view world_id,
                                             std::string_view world_revision,
                                             std::string_view world_package_cache_key) {
    return karma::content::PackageRootForIdentity(WorldPackagesByWorldRoot(server_cache_dir),
                                                  world_id,
                                                  world_revision,
                                                  world_package_cache_key,
                                                  kMaxCachePathComponentLen);
}

std::string ResolveWorldPackageCacheKey(std::string_view world_content_hash, std::string_view world_hash) {
    return karma::content::ResolveWorldPackageCacheKey(world_content_hash, world_hash);
}


std::string WorldCacheDirName(std::string_view world_id) {
    return karma::content::WorldCacheDirName(world_id, kMaxCachePathComponentLen);
}

std::string RevisionCacheDirName(std::string_view world_revision) {
    return karma::content::RevisionCacheDirName(world_revision, kMaxCachePathComponentLen);
}

std::filesystem::path BuildPackageStagingRoot(const std::filesystem::path& package_root) {
    return karma::content::BuildPackageStagingRoot(package_root);
}

std::filesystem::path BuildPackageBackupRoot(const std::filesystem::path& package_root) {
    return karma::content::BuildPackageBackupRoot(package_root);
}

void CleanupStaleTemporaryDirectories(const std::filesystem::path& package_root) {
    karma::content::CleanupStaleTemporaryDirectories(package_root, "ClientConnection");
}

bool ActivateStagedPackageRootAtomically(const std::filesystem::path& package_root,
                                         const std::filesystem::path& staging_root) {
    return karma::content::ActivateStagedPackageRootAtomically(package_root,
                                                               staging_root,
                                                               "ClientConnection");
}


void TouchPathIfPresent(const std::filesystem::path& path) {
    karma::content::TouchPathIfPresent(path);
}

std::filesystem::file_time_type LastWriteTimeOrMin(const std::filesystem::path& path) {
    return karma::content::LastWriteTimeOrMin(path);
}


std::string ComputeWorldPackageHash(const std::vector<std::byte>& bytes) {
    return karma::content::ComputeWorldPackageHash(bytes);
}


bool NormalizeRelativePath(std::string_view raw_path, std::filesystem::path* out) {
    return karma::content::NormalizeRelativePath(raw_path, out);
}


} // namespace bz3::client::net
