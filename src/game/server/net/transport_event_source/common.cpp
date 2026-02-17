#include "server/net/transport_event_source/internal.hpp"

#include "karma/common/content/delta_builder.hpp"
#include "karma/common/content/primitives.hpp"
#include "karma/common/logging.hpp"
#include "net/protocol_codec.hpp"

namespace bz3::server::net::detail {

namespace {

std::vector<karma::content::ManifestEntry> ToContentManifest(
    const std::vector<WorldManifestEntry>& manifest) {
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

std::string DefaultPlayerName(uint32_t client_id) {
    return "player-" + std::to_string(client_id);
}

std::string ResolvePlayerName(const bz3::net::ClientMessage& message, uint32_t client_id) {
    if (!message.player_name.empty()) {
        return message.player_name;
    }
    return DefaultPlayerName(client_id);
}

bool NormalizeRelativePath(std::string_view raw_path, std::filesystem::path* out) {
    return karma::content::NormalizeRelativePath(raw_path, out);
}

ManifestDiffPlan BuildServerManifestDiffPlan(const std::vector<WorldManifestEntry>& cached_manifest,
                                             const std::vector<WorldManifestEntry>& incoming_manifest) {
    return karma::content::BuildManifestDiffPlan(ToContentManifest(cached_manifest),
                                                 ToContentManifest(incoming_manifest));
}

void LogServerManifestDiffPlan(uint32_t client_id,
                               std::string_view world_name,
                               const ManifestDiffPlan& plan) {
    if (!plan.incoming_manifest_available) {
        KARMA_TRACE("net.server",
                    "ServerEventSource: manifest diff plan skipped client_id={} world='{}' (incoming manifest unavailable, cached_entries={})",
                    client_id,
                    world_name,
                    plan.cached_entries);
        return;
    }

    KARMA_TRACE("net.server",
                "ServerEventSource: manifest diff plan client_id={} world='{}' cached_entries={} incoming_entries={} unchanged={} added={} modified={} removed={} potential_transfer_bytes={} reused_bytes={} delta_transfer_bytes={}",
                client_id,
                world_name,
                plan.cached_entries,
                plan.incoming_entries,
                plan.unchanged_entries,
                plan.added_entries,
                plan.modified_entries,
                plan.removed_entries,
                plan.potential_transfer_bytes,
                plan.reused_bytes,
                plan.delta_transfer_bytes);
}

std::optional<std::vector<std::byte>> BuildWorldDeltaArchive(const std::filesystem::path& world_dir,
                                                             const ManifestDiffPlan& diff_plan,
                                                             std::string_view world_id,
                                                             std::string_view target_world_revision,
                                                             std::string_view base_world_revision) {
    return karma::content::BuildDeltaArchiveFromManifestDiff(world_dir,
                                                             diff_plan,
                                                             world_id,
                                                             target_world_revision,
                                                             base_world_revision,
                                                             "ServerEventSource");
}

} // namespace bz3::server::net::detail
