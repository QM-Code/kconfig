#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "karma/common/json.hpp"

namespace bz3::server {

class HeartbeatClient;
class ServerGame;

class CommunityHeartbeat {
 public:
    CommunityHeartbeat();
    ~CommunityHeartbeat();

    void configureFromConfig(const karma::json::Value& merged_config,
                             uint16_t listen_port,
                             const std::string& community_override);
    void update(const ServerGame& game);

 private:
    std::unique_ptr<HeartbeatClient> client_{};
    bool enabled_ = false;
    int interval_seconds_ = 0;
    std::string community_url_{};
    std::string server_address_{};
    int max_players_ = 0;
    std::chrono::steady_clock::time_point next_heartbeat_time_{};
};

} // namespace bz3::server
