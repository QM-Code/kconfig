#include "game.hpp"

#include "client/net/client_connection.hpp"
#include "karma/audio/backend.hpp"
#include "karma/audio/audio_system.hpp"
#include "karma/common/logging.hpp"
#include "karma/input/input_system.hpp"
#include "karma/ui/ui_draw_context.hpp"

#include <spdlog/spdlog.h>
#include <cstdlib>
#include <string>
#include <utility>

namespace bz3 {

Game::Game(GameStartupOptions options) : startup_(std::move(options)) {}

Game::~Game() = default;

void Game::onStart() {
    if (input) {
        input->setMode(karma::input::InputMode::Roaming);
    }

    if (!startup_.connect_on_start) {
        return;
    }

    connection_ = std::make_unique<client::net::ClientConnection>(
        startup_.connect_addr,
        startup_.connect_port,
        startup_.player_name,
        [this](client::net::AudioEvent event) { onAudioEvent(event); });
    if (!connection_->start()) {
        KARMA_TRACE("net.client",
                    "Game: startup connect failed addr='{}' port={} name='{}'",
                    startup_.connect_addr,
                    startup_.connect_port,
                    startup_.player_name);
        connection_.reset();
    }
}

void Game::onUpdate(float dt) {
    (void)dt;
    if (connection_) {
        connection_->poll();
        if (connection_->shouldExit()) {
            spdlog::error("Game: network join was rejected, closing client.");
            std::exit(1);
        }
    }

    if (!input) {
        spawn_was_down_ = false;
        fire_was_down_ = false;
        return;
    }

    const bool spawn_down = input->game().actionDown("spawn");
    const bool fire_down = input->game().actionDown("fire");

    const bool spawn_pressed =
        input->game().actionPressed("spawn") || (spawn_down && !spawn_was_down_);
    const bool fire_pressed =
        input->game().actionPressed("fire") || (fire_down && !fire_was_down_);

    if (spawn_pressed) {
        if (connection_ && connection_->isConnected()) {
            (void)connection_->sendRequestPlayerSpawn();
        }
    }

    if (fire_pressed) {
        // Play local fire immediately for responsive feedback.
        onAudioEvent(client::net::AudioEvent::ShotFire);
        if (connection_ && connection_->isConnected()) {
            (void)connection_->sendCreateShot();
        }
    }

    spawn_was_down_ = spawn_down;
    fire_was_down_ = fire_down;
}

void Game::onUiUpdate(float dt, karma::ui::UiDrawContext& ui) {
    (void)dt;
    karma::ui::UiDrawContext::TextPanel panel{};
    panel.title = "BZ3 Client";
    panel.lines.push_back("Engine-owned UI lifecycle (game-submitted content)");
    panel.lines.push_back("");
    panel.lines.push_back("Player: " + startup_.player_name);
    if (startup_.connect_on_start) {
        panel.lines.push_back("Connect target: " + startup_.connect_addr + ":" +
                              std::to_string(startup_.connect_port));
        const bool connected = connection_ && connection_->isConnected();
        panel.lines.push_back(std::string("Connection: ") + (connected ? "connected" : "connecting"));
    } else {
        panel.lines.push_back("Connection: offline");
    }
    ui.addTextPanel(std::move(panel));
}

void Game::onShutdown() {
    if (connection_) {
        connection_->shutdown();
        connection_.reset();
    }
}

void Game::onAudioEvent(client::net::AudioEvent event) {
    switch (event) {
        case client::net::AudioEvent::PlayerSpawn:
            playOneShotAsset("assets.audio.player.Spawn", 1.0f, 1.0f);
            break;
        case client::net::AudioEvent::PlayerDeath:
            playOneShotAsset("assets.audio.player.Die", 1.0f, 1.0f);
            break;
        case client::net::AudioEvent::ShotFire:
            playOneShotAsset("assets.audio.shot.Fire", 0.65f, 1.0f);
            break;
        default:
            break;
    }
}

void Game::playOneShotAsset(const char* asset_key, float gain, float pitch) {
    if (!audio || !asset_key || *asset_key == '\0') {
        return;
    }

    karma::audio_backend::PlayRequest request{};
    request.asset_path = asset_key;
    request.gain = gain;
    request.pitch = pitch;
    request.loop = false;
    audio->playOneShot(request);
}

} // namespace bz3
