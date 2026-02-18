#include "karma/network/server/session/join_runtime.hpp"

namespace karma::network {

ServerJoinResultPayload BuildServerJoinResultPayload(uint32_t client_id,
                                                     const ServerSessionJoinDecision& join_decision,
                                                     const ServerJoinWorldState& world_state,
                                                     const ServerSessionSnapshotProvider& snapshot_provider) {
    ServerJoinResultPayload payload{};
    payload.client_id = client_id;
    payload.accepted = join_decision.accepted;
    payload.reason = join_decision.reject_reason;
    payload.world = &world_state;

    if (payload.accepted && snapshot_provider) {
        payload.sessions = snapshot_provider();
    } else if (!payload.accepted && payload.reason.empty()) {
        payload.reason = "Join rejected by server.";
    }

    return payload;
}

void EmitServerJoinResult(const ServerJoinResultPayload& payload,
                          const ServerJoinResultEmitter& emit_fn) {
    if (!emit_fn) {
        return;
    }
    emit_fn(payload);
}

} // namespace karma::network
