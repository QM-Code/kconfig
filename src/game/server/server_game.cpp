#include "server/server_game.hpp"

#include "karma/common/logging.hpp"
#include "karma/ecs/world.hpp"

namespace bz3::server {

namespace {

struct ServerStateComponent {
    uint64_t ticks = 0;
    float uptime_seconds = 0.0f;
};

} // namespace

ServerGame::ServerGame(std::string worldName)
    : world_name_(std::move(worldName)) {}

void ServerGame::onStart() {
    if (!world) {
        return;
    }

    state_entity_ = world->createEntity();
    world->add<ServerStateComponent>(state_entity_, ServerStateComponent{});

    simulated_entities_.clear();
    simulated_entities_.reserve(8);
    for (int i = 0; i < 8; ++i) {
        simulated_entities_.push_back(world->createEntity());
    }

    KARMA_TRACE("engine.server",
                "ServerGame: onStart world='{}' state_entity={} simulated_entities={} world_entities={}",
                world_name_,
                state_entity_.index,
                simulated_entities_.size(),
                world->entities().size());
}

void ServerGame::onTick(float dt) {
    if (!world || !state_entity_.isValid()) {
        return;
    }

    ServerStateComponent* state = world->tryGet<ServerStateComponent>(state_entity_);
    if (!state) {
        return;
    }

    state->ticks += 1;
    state->uptime_seconds += dt;
    status_log_accumulator_ += dt;

    if (status_log_accumulator_ >= 1.0f) {
        status_log_accumulator_ = 0.0f;
        KARMA_TRACE("engine.server",
                    "ServerGame: tick={} uptime={:.2f}s world='{}' world_entities={}",
                    state->ticks,
                    state->uptime_seconds,
                    world_name_,
                    world->entities().size());
    }
}

void ServerGame::onShutdown() {
    if (!world) {
        return;
    }

    for (const auto entity : simulated_entities_) {
        world->destroyEntity(entity);
    }
    simulated_entities_.clear();

    if (state_entity_.isValid()) {
        world->destroyEntity(state_entity_);
        state_entity_ = {};
    }

    KARMA_TRACE("engine.server",
                "ServerGame: onShutdown world='{}' world_entities={}",
                world_name_,
                world->entities().size());
}

} // namespace bz3::server
