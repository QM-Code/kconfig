#pragma once

#include "karma/network/server/session/hooks.hpp"

#include <cstdint>
#include <functional>

namespace karma::network {

enum class ServerSessionLeaveEventResult {
    Applied,
    IgnoredUnknownClient,
    IgnoredLeaveRejected,
    InvalidHooks
};

using ServerSessionLeaveAppliedFn = std::function<void(uint32_t)>;

ServerSessionLeaveEventResult ApplyServerSessionLeaveEvent(uint32_t client_id,
                                                           const ServerSessionHooks& hooks,
                                                           const ServerSessionLeaveAppliedFn& on_leave_applied);

const char* DescribeServerSessionLeaveEventResult(ServerSessionLeaveEventResult result);

} // namespace karma::network
