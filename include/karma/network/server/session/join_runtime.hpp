#pragma once

#include "karma/network/server/session/hooks.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace karma::network {

struct ServerSessionSnapshotEntry {
    uint32_t session_id = 0;
    std::string session_name{};
};

struct ServerWorldManifestEntry {
    std::string path{};
    uint64_t size = 0;
    std::string hash{};
};

struct ServerJoinWorldState {
    std::string world_name{};
    std::string world_id{};
    std::string world_revision{};
    std::string world_package_hash{};
    std::string world_content_hash{};
    std::string world_manifest_hash{};
    uint32_t world_manifest_file_count = 0;
    uint64_t world_package_size = 0;
    std::filesystem::path world_dir{};
    const std::vector<ServerWorldManifestEntry>* world_manifest = nullptr;
    const std::vector<std::byte>* world_package = nullptr;
};

struct ServerJoinResultPayload {
    uint32_t client_id = 0;
    bool accepted = false;
    std::string reason{};
    std::vector<ServerSessionSnapshotEntry> sessions{};
    const ServerJoinWorldState* world = nullptr;
};

using ServerSessionSnapshotProvider = std::function<std::vector<ServerSessionSnapshotEntry>()>;
using ServerJoinResultEmitter = std::function<void(const ServerJoinResultPayload&)>;

ServerJoinResultPayload BuildServerJoinResultPayload(uint32_t client_id,
                                                     const ServerSessionJoinDecision& join_decision,
                                                     const ServerJoinWorldState& world_state,
                                                     const ServerSessionSnapshotProvider& snapshot_provider);

void EmitServerJoinResult(const ServerJoinResultPayload& payload,
                          const ServerJoinResultEmitter& emit_fn);

} // namespace karma::network
