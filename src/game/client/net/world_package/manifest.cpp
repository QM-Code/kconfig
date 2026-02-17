#include "client/net/world_package/internal.hpp"

#include "karma/common/content/cache_store.hpp"
#include "karma/common/content/manifest.hpp"
#include "karma/common/logging.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
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

std::vector<bz3::net::WorldManifestEntry> FromContentManifest(
    const std::vector<karma::content::ManifestEntry>& manifest) {
    std::vector<bz3::net::WorldManifestEntry> converted{};
    converted.reserve(manifest.size());
    for (const auto& entry : manifest) {
        converted.push_back(bz3::net::WorldManifestEntry{
            .path = entry.path,
            .size = entry.size,
            .hash = entry.hash});
    }
    return converted;
}

} // namespace

std::string ComputeManifestHash(const std::vector<bz3::net::WorldManifestEntry>& manifest) {
    return karma::content::ComputeManifestHash(ToContentManifest(manifest));
}

bool VerifyExtractedWorldPackage(const std::filesystem::path& package_root,
                                 std::string_view world_name,
                                 std::string_view expected_world_content_hash,
                                 std::string_view expected_world_manifest_hash,
                                 uint32_t expected_world_manifest_file_count,
                                 const std::vector<bz3::net::WorldManifestEntry>& expected_world_manifest,
                                 std::string_view stage_name) {
    const auto summary = karma::content::ComputeDirectoryManifestSummary(package_root);
    if (!summary.has_value()) {
        spdlog::error("ClientConnection: failed to verify {} world package '{}' at '{}'",
                      stage_name,
                      world_name,
                      package_root.string());
        return false;
    }

    if (!expected_world_content_hash.empty() &&
        summary->content_hash != expected_world_content_hash) {
        spdlog::error("ClientConnection: {} content hash mismatch for world '{}' (expected='{}' got='{}')",
                      stage_name,
                      world_name,
                      expected_world_content_hash,
                      summary->content_hash);
        return false;
    }

    if (!expected_world_manifest_hash.empty() &&
        summary->manifest_hash != expected_world_manifest_hash) {
        spdlog::error("ClientConnection: {} manifest hash mismatch for world '{}' (expected='{}' got='{}')",
                      stage_name,
                      world_name,
                      expected_world_manifest_hash,
                      summary->manifest_hash);
        return false;
    }

    if (expected_world_manifest_file_count > 0 &&
        summary->entries.size() != expected_world_manifest_file_count) {
        spdlog::error("ClientConnection: {} manifest file count mismatch for world '{}' (expected={} got={})",
                      stage_name,
                      world_name,
                      expected_world_manifest_file_count,
                      summary->entries.size());
        return false;
    }

    if (!expected_world_manifest.empty()) {
        const auto expected = karma::content::SortManifestEntries(ToContentManifest(expected_world_manifest));
        const auto actual = karma::content::SortManifestEntries(summary->entries);
        if (!karma::content::ManifestEntriesEqual(expected, actual)) {
            spdlog::error("ClientConnection: {} manifest entries mismatch for world '{}' (expected_entries={} got_entries={})",
                          stage_name,
                          world_name,
                          expected.size(),
                          actual.size());
            return false;
        }
    }

    KARMA_TRACE("net.client",
                "ClientConnection: verified {} world package world='{}' content_hash='{}' manifest_hash='{}' files={}",
                stage_name,
                world_name,
                summary->content_hash,
                summary->manifest_hash,
                summary->entries.size());
    return true;
}

std::vector<bz3::net::WorldManifestEntry> ReadCachedWorldManifest(
    const std::filesystem::path& server_cache_dir) {
    bool malformed = false;
    const auto manifest = karma::content::ReadCachedManifestFile(ActiveWorldManifestPath(server_cache_dir),
                                                                 &malformed);
    if (malformed) {
        spdlog::warn("ClientConnection: cached world manifest '{}' is malformed; ignoring",
                     ActiveWorldManifestPath(server_cache_dir).string());
    }
    return FromContentManifest(manifest);
}

bool PersistCachedWorldManifest(const std::filesystem::path& server_cache_dir,
                                const std::vector<bz3::net::WorldManifestEntry>& manifest) {
    return karma::content::PersistCachedManifestFile(ActiveWorldManifestPath(server_cache_dir),
                                                     ToContentManifest(manifest));
}

void LogManifestDiffPlan(std::string_view world_name,
                         const std::vector<bz3::net::WorldManifestEntry>& cached_manifest,
                         const std::vector<bz3::net::WorldManifestEntry>& incoming_manifest) {
    if (incoming_manifest.empty()) {
        KARMA_TRACE("net.client",
                    "ClientConnection: manifest diff plan skipped world='{}' (incoming manifest unavailable, cached_entries={})",
                    world_name,
                    cached_manifest.size());
        return;
    }

    const auto plan = karma::content::BuildManifestDiffPlan(ToContentManifest(cached_manifest),
                                                             ToContentManifest(incoming_manifest));
    if (cached_manifest.empty()) {
        KARMA_TRACE("net.client",
                    "ClientConnection: manifest diff plan world='{}' cached_entries=0 incoming_entries={} unchanged=0 added={} modified=0 removed=0 potential_transfer_bytes={} reused_bytes=0",
                    world_name,
                    incoming_manifest.size(),
                    incoming_manifest.size(),
                    plan.potential_transfer_bytes);
        return;
    }

    KARMA_TRACE("net.client",
                "ClientConnection: manifest diff plan world='{}' cached_entries={} incoming_entries={} unchanged={} added={} modified={} removed={} potential_transfer_bytes={} reused_bytes={}",
                world_name,
                cached_manifest.size(),
                incoming_manifest.size(),
                plan.unchanged_entries,
                plan.added_entries,
                plan.modified_entries,
                plan.removed_entries,
                plan.potential_transfer_bytes,
                plan.reused_bytes);
}

} // namespace bz3::client::net
