#include "karma/network/server_session_hooks.hpp"

namespace karma::network {

ServerSessionJoinDecision ResolveServerSessionJoinDecision(const ServerPreAuthConfig& pre_auth_config,
                                                           const ServerPreAuthRequest& request,
                                                           const ServerSessionHooks& hooks) {
    if (!hooks.on_join) {
        return ServerSessionJoinDecision{
            .accepted = false,
            .reject_reason = "Join rejected by server."};
    }

    const ServerPreAuthDecision pre_auth_decision = EvaluateServerPreAuth(pre_auth_config, request);
    if (!pre_auth_decision.accepted) {
        return ServerSessionJoinDecision{
            .accepted = false,
            .reject_reason = pre_auth_decision.reject_reason.empty()
                ? std::string("Authentication failed.")
                : pre_auth_decision.reject_reason};
    }

    ServerSessionJoinDecision decision = hooks.on_join(request);
    if (decision.accepted) {
        return decision;
    }
    if (!decision.reject_reason.empty()) {
        return decision;
    }

    if (hooks.last_join_reject_reason) {
        const std::string reason = hooks.last_join_reject_reason();
        if (!reason.empty()) {
            decision.reject_reason = reason;
            return decision;
        }
    }

    decision.reject_reason = "Join rejected by server.";
    return decision;
}

ServerSessionLeaveResult ApplyServerSessionLeave(uint32_t client_id,
                                                 const ServerSessionHooks& hooks) {
    if (!hooks.has_client || !hooks.on_leave) {
        return ServerSessionLeaveResult::InvalidHooks;
    }
    if (!hooks.has_client(client_id)) {
        return ServerSessionLeaveResult::UnknownClient;
    }
    if (!hooks.on_leave(client_id)) {
        return ServerSessionLeaveResult::LeaveRejected;
    }
    return ServerSessionLeaveResult::Applied;
}

} // namespace karma::network
