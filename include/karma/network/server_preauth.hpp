#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace karma::network {

struct ServerPreAuthConfig {
    std::string required_password{};
    std::string reject_reason{"Authentication failed."};
    std::string community_url{};
    std::string world_name{};
};

struct ServerPreAuthRequest {
    uint32_t client_id = 0;
    std::string_view player_name{};
    std::string_view auth_payload{};
    std::string_view peer_ip{};
    uint16_t peer_port = 0;
};

struct ServerPreAuthDecision {
    bool accepted = true;
    std::string reject_reason{};
};

ServerPreAuthConfig ReadServerPreAuthConfig();

ServerPreAuthDecision EvaluateServerPreAuth(const ServerPreAuthConfig& config,
                                            const ServerPreAuthRequest& request);

} // namespace karma::network
