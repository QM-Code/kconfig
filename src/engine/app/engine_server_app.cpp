#include "karma/app/engine_server_app.hpp"

#include "karma/common/logging.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

namespace karma::app {

EngineServerApp::~EngineServerApp() {
    shutdown();
}

void EngineServerApp::start(ServerGameInterface& game, const EngineServerConfig& config) {
    if (running_) {
        return;
    }

    game_ = &game;
    config_ = config;
    game_->bind(world_);
    last_tick_time_ = std::chrono::steady_clock::now();
    running_ = true;

    KARMA_TRACE("engine.server",
                "EngineServerApp: start target_hz={:.2f} max_dt={:.3f}",
                config_.target_tick_hz,
                config_.max_delta_time);
    game_->onStart();
}

void EngineServerApp::requestStop() {
    running_ = false;
}

void EngineServerApp::tick() {
    if (!running_ || !game_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    const float min_dt = (config_.target_tick_hz > 0.0f) ? (1.0f / config_.target_tick_hz) : 0.0f;
    if (min_dt > 0.0f) {
        const float elapsed = std::chrono::duration<float>(now - last_tick_time_).count();
        if (elapsed < min_dt) {
            std::this_thread::sleep_for(std::chrono::duration<float>(min_dt - elapsed));
            now = std::chrono::steady_clock::now();
        }
    }

    float dt = std::chrono::duration<float>(now - last_tick_time_).count();
    last_tick_time_ = now;
    if (config_.max_delta_time > 0.0f) {
        dt = std::min(dt, config_.max_delta_time);
    }

    KARMA_TRACE_CHANGED("ecs.world",
                        std::to_string(world_.entities().size()),
                        "EngineServerApp: world entities={}",
                        world_.entities().size());

    game_->onTick(dt);

    if (!running_) {
        shutdown();
    }
}

void EngineServerApp::shutdown() {
    if (!game_) {
        running_ = false;
        return;
    }

    game_->onShutdown();
    KARMA_TRACE("engine.server",
                "EngineServerApp: shutdown world_entities={}",
                world_.entities().size());

    game_ = nullptr;
    running_ = false;
}

} // namespace karma::app
