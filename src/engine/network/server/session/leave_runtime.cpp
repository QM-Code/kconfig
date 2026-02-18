#include "karma/network/server/session/leave_runtime.hpp"

namespace karma::network {

ServerSessionLeaveEventResult ApplyServerSessionLeaveEvent(uint32_t client_id,
                                                           const ServerSessionHooks& hooks,
                                                           const ServerSessionLeaveAppliedFn& on_leave_applied) {
    const ServerSessionLeaveResult leave_result = ApplyServerSessionLeave(client_id, hooks);
    switch (leave_result) {
        case ServerSessionLeaveResult::Applied:
            if (on_leave_applied) {
                on_leave_applied(client_id);
            }
            return ServerSessionLeaveEventResult::Applied;
        case ServerSessionLeaveResult::UnknownClient:
            return ServerSessionLeaveEventResult::IgnoredUnknownClient;
        case ServerSessionLeaveResult::LeaveRejected:
            return ServerSessionLeaveEventResult::IgnoredLeaveRejected;
        case ServerSessionLeaveResult::InvalidHooks:
        default:
            return ServerSessionLeaveEventResult::InvalidHooks;
    }
}

const char* DescribeServerSessionLeaveEventResult(ServerSessionLeaveEventResult result) {
    switch (result) {
        case ServerSessionLeaveEventResult::Applied:
            return "applied";
        case ServerSessionLeaveEventResult::IgnoredUnknownClient:
            return "ignored unknown client";
        case ServerSessionLeaveEventResult::IgnoredLeaveRejected:
            return "ignored leave rejected";
        case ServerSessionLeaveEventResult::InvalidHooks:
        default:
            return "ignored invalid hooks";
    }
}

} // namespace karma::network
