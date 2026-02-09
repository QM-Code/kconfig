#pragma once

#include "server/net/event_source.hpp"

#include <cstdint>
#include <functional>

namespace bz3::server {

enum class RuntimeEventRuleResult {
    Applied,
    IgnoredUnknownClient,
    IgnoredLeaveFailed,
    InvalidHandlers,
    UnsupportedEvent
};

struct RuntimeEventRuleHandlers {
    std::function<bool(uint32_t)> has_client{};
    std::function<bool(uint32_t)> on_client_leave{};
    std::function<void(uint32_t)> on_player_death{};
    std::function<void(uint32_t)> on_player_spawn{};
    std::function<void(uint32_t, uint32_t, float, float, float, float, float, float)> on_create_shot{};
};

RuntimeEventRuleResult ApplyRuntimeEventRules(const net::ServerInputEvent& event,
                                              RuntimeEventRuleHandlers& handlers,
                                              uint32_t* next_global_shot_id);

} // namespace bz3::server
