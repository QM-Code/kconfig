#include "karma/network/server/session/leave_runtime.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>

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

bool TestAppliedLeaveCallsCallback() {
    std::unordered_set<uint32_t> connected{3};
    int callback_calls = 0;

    karma::network::ServerSessionHooks hooks{};
    hooks.has_client = [&connected](uint32_t client_id) {
        return connected.contains(client_id);
    };
    hooks.on_leave = [&connected](uint32_t client_id) {
        connected.erase(client_id);
        return true;
    };

    const auto result = karma::network::ApplyServerSessionLeaveEvent(
        3,
        hooks,
        [&callback_calls](uint32_t) {
            ++callback_calls;
        });

    return Expect(result == karma::network::ServerSessionLeaveEventResult::Applied,
                  "applied leave should return Applied")
           && Expect(callback_calls == 1, "applied leave should call callback once")
           && Expect(!connected.contains(3), "applied leave should remove client");
}

bool TestUnknownClientLeaveSkipped() {
    std::unordered_set<uint32_t> connected{};
    int callback_calls = 0;

    karma::network::ServerSessionHooks hooks{};
    hooks.has_client = [&connected](uint32_t client_id) {
        return connected.contains(client_id);
    };
    hooks.on_leave = [](uint32_t) {
        return true;
    };

    const auto result = karma::network::ApplyServerSessionLeaveEvent(
        9,
        hooks,
        [&callback_calls](uint32_t) {
            ++callback_calls;
        });

    return Expect(result == karma::network::ServerSessionLeaveEventResult::IgnoredUnknownClient,
                  "unknown leave should be ignored")
           && Expect(callback_calls == 0, "unknown leave should not call callback");
}

bool TestRejectedLeaveSkipped() {
    std::unordered_set<uint32_t> connected{11};
    int callback_calls = 0;

    karma::network::ServerSessionHooks hooks{};
    hooks.has_client = [&connected](uint32_t client_id) {
        return connected.contains(client_id);
    };
    hooks.on_leave = [](uint32_t) {
        return false;
    };

    const auto result = karma::network::ApplyServerSessionLeaveEvent(
        11,
        hooks,
        [&callback_calls](uint32_t) {
            ++callback_calls;
        });

    return Expect(result == karma::network::ServerSessionLeaveEventResult::IgnoredLeaveRejected,
                  "rejected leave should map to IgnoredLeaveRejected")
           && Expect(callback_calls == 0, "rejected leave should not call callback")
           && Expect(connected.contains(11), "rejected leave should keep client connected");
}

bool TestInvalidHooks() {
    int callback_calls = 0;
    karma::network::ServerSessionHooks hooks{};

    const auto result = karma::network::ApplyServerSessionLeaveEvent(
        1,
        hooks,
        [&callback_calls](uint32_t) {
            ++callback_calls;
        });

    return Expect(result == karma::network::ServerSessionLeaveEventResult::InvalidHooks,
                  "invalid hooks should return InvalidHooks")
           && Expect(callback_calls == 0, "invalid hooks should not call callback");
}

bool TestDescribeResultStrings() {
    return Expect(std::string(karma::network::DescribeServerSessionLeaveEventResult(
                                  karma::network::ServerSessionLeaveEventResult::Applied))
                      == "applied",
                  "Applied description mismatch")
           && Expect(std::string(karma::network::DescribeServerSessionLeaveEventResult(
                                     karma::network::ServerSessionLeaveEventResult::IgnoredUnknownClient))
                         == "ignored unknown client",
                     "IgnoredUnknownClient description mismatch")
           && Expect(std::string(karma::network::DescribeServerSessionLeaveEventResult(
                                     karma::network::ServerSessionLeaveEventResult::IgnoredLeaveRejected))
                         == "ignored leave rejected",
                     "IgnoredLeaveRejected description mismatch")
           && Expect(std::string(karma::network::DescribeServerSessionLeaveEventResult(
                                     karma::network::ServerSessionLeaveEventResult::InvalidHooks))
                         == "ignored invalid hooks",
                     "InvalidHooks description mismatch");
}

} // namespace

int main() {
    if (!TestAppliedLeaveCallsCallback()) {
        return 1;
    }
    if (!TestUnknownClientLeaveSkipped()) {
        return 1;
    }
    if (!TestRejectedLeaveSkipped()) {
        return 1;
    }
    if (!TestInvalidHooks()) {
        return 1;
    }
    if (!TestDescribeResultStrings()) {
        return 1;
    }
    return 0;
}
