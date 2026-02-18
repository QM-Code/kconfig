#pragma once

#include "karma/network/server/auth/preauth.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace karma::network {

struct ServerSessionJoinDecision {
    bool accepted = true;
    std::string reject_reason{};
};

struct ServerSessionHooks {
    std::function<ServerSessionJoinDecision(const ServerPreAuthRequest&)> on_join{};
    std::function<bool(uint32_t)> has_client{};
    std::function<bool(uint32_t)> on_leave{};
    std::function<std::string()> last_join_reject_reason{};
};

enum class ServerSessionLeaveResult {
    Applied,
    UnknownClient,
    LeaveRejected,
    InvalidHooks
};

ServerSessionJoinDecision ResolveServerSessionJoinDecision(const ServerPreAuthConfig& pre_auth_config,
                                                           const ServerPreAuthRequest& request,
                                                           const ServerSessionHooks& hooks);

ServerSessionLeaveResult ApplyServerSessionLeave(uint32_t client_id,
                                                 const ServerSessionHooks& hooks);

} // namespace karma::network
