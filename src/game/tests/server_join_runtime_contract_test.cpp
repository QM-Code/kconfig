#include "karma/network/server_join_runtime.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool Fail(const std::string& message) {
    std::cerr << message << "\n";
    return false;
}

bool Expect(bool condition, const std::string& message) {
    if (!condition) {
        return Fail(message);
    }
    return true;
}

bool TestAcceptedJoinBuildsPayloadAndEmits() {
    std::vector<karma::network::ServerWorldManifestEntry> manifest{
        karma::network::ServerWorldManifestEntry{
            .path = "maps/default.bzw",
            .size = 128,
            .hash = "hash-1"}};
    std::vector<std::byte> world_package{
        std::byte{0x1},
        std::byte{0x2}};
    const karma::network::ServerJoinWorldState world{
        .world_name = "Test World",
        .world_id = "world-id",
        .world_revision = "rev-1",
        .world_package_hash = "pkg-hash",
        .world_content_hash = "content-hash",
        .world_manifest_hash = "manifest-hash",
        .world_manifest_file_count = 1,
        .world_package_size = 2,
        .world_manifest = &manifest,
        .world_package = &world_package};
    const karma::network::ServerSessionJoinDecision decision{
        .accepted = true,
        .reject_reason = {}};

    bool snapshot_called = false;
    const auto payload = karma::network::BuildServerJoinResultPayload(
        7,
        decision,
        world,
        [&snapshot_called]() {
            snapshot_called = true;
            return std::vector<karma::network::ServerSessionSnapshotEntry>{
                karma::network::ServerSessionSnapshotEntry{.session_id = 7, .session_name = "alpha"}};
        });

    if (!Expect(snapshot_called, "accepted join should call snapshot provider")) {
        return false;
    }
    if (!Expect(payload.client_id == 7, "accepted join should preserve client_id")
        || !Expect(payload.accepted, "accepted join should set accepted=true")
        || !Expect(payload.reason.empty(), "accepted join should not populate reject reason")
        || !Expect(payload.sessions.size() == 1, "accepted join should carry snapshot sessions")
        || !Expect(payload.world == &world, "accepted join should keep world reference")) {
        return false;
    }

    bool emitted = false;
    karma::network::EmitServerJoinResult(
        payload,
        [&emitted](const karma::network::ServerJoinResultPayload& out) {
            emitted = true;
            if (!Expect(out.client_id == 7, "emitted payload client_id mismatch")
                || !Expect(out.sessions.size() == 1, "emitted payload session count mismatch")
                || !Expect(out.world != nullptr, "emitted payload world pointer missing")) {
                emitted = false;
            }
        });
    return Expect(emitted, "emit callback should be invoked");
}

bool TestRejectedJoinSkipsSnapshotAndDefaultsReason() {
    const karma::network::ServerJoinWorldState world{};
    const karma::network::ServerSessionJoinDecision decision{
        .accepted = false,
        .reject_reason = {}};

    bool snapshot_called = false;
    const auto payload = karma::network::BuildServerJoinResultPayload(
        8,
        decision,
        world,
        [&snapshot_called]() {
            snapshot_called = true;
            return std::vector<karma::network::ServerSessionSnapshotEntry>{};
        });

    if (!Expect(!snapshot_called, "rejected join should skip snapshot provider")) {
        return false;
    }
    return Expect(payload.client_id == 8, "rejected join should preserve client_id")
           && Expect(!payload.accepted, "rejected join should set accepted=false")
           && Expect(payload.sessions.empty(), "rejected join should not include sessions")
           && Expect(payload.reason == "Join rejected by server.",
                     "rejected join without reason should use default reason");
}

bool TestEmitJoinResultNoopWithoutEmitter() {
    const karma::network::ServerJoinResultPayload payload{};
    karma::network::EmitServerJoinResult(payload, {});
    return true;
}

} // namespace

int main() {
    if (!TestAcceptedJoinBuildsPayloadAndEmits()) {
        return 1;
    }
    if (!TestRejectedJoinSkipsSnapshotAndDefaultsReason()) {
        return 1;
    }
    if (!TestEmitJoinResultNoopWithoutEmitter()) {
        return 1;
    }
    return 0;
}
