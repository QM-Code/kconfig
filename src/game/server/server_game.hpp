#pragma once

#include <string>
#include <vector>

#include "karma/app/server_game_interface.hpp"
#include "karma/ecs/entity.hpp"

namespace bz3::server {

class ServerGame final : public karma::app::ServerGameInterface {
 public:
    explicit ServerGame(std::string worldName);

    void onStart() override;
    void onTick(float dt) override;
    void onShutdown() override;

 private:
    std::string world_name_;
    karma::ecs::Entity state_entity_{};
    std::vector<karma::ecs::Entity> simulated_entities_{};
    float status_log_accumulator_ = 0.0f;
};

} // namespace bz3::server
