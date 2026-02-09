#pragma once

#include "karma/app/game_interface.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace bz3::client::net {
class ClientConnection;
enum class AudioEvent;
}

namespace bz3 {

struct GameStartupOptions {
    std::string player_name{};
    std::string connect_addr{};
    uint16_t connect_port = 0;
    bool connect_on_start = false;
};

class Game final : public karma::app::GameInterface {
 public:
    explicit Game(GameStartupOptions options = {});
    ~Game() override;
    void onStart() override;
    void onUpdate(float dt) override;
    void onUiUpdate(float dt, karma::ui::UiDrawContext& ui) override;
    void onShutdown() override;

 private:
    void onAudioEvent(client::net::AudioEvent event);
    void playOneShotAsset(const char* asset_key, float gain = 1.0f, float pitch = 1.0f);

    GameStartupOptions startup_{};
    std::unique_ptr<client::net::ClientConnection> connection_{};
    bool spawn_was_down_ = false;
    bool fire_was_down_ = false;
};

} // namespace bz3
