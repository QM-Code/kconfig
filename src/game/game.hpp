#pragma once

#include "karma/app/game_interface.hpp"
#include "karma/ecs/entity.hpp"
#include "karma/renderer/types.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace bz3::client::net {
class ClientConnection;
enum class AudioEvent;
}

namespace bz3::client::domain {
class TankDriveController;
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
    bool ensureLocalTankEntity();
    void destroyLocalTankEntity();
    void updateLocalTank(float dt, bool gameplay_input_enabled);
    void updateLocalTankTransform();
    void updateTankFollowCamera();
    void syncInputMode();
    void onAudioEvent(client::net::AudioEvent event);
    void playOneShotAsset(const char* asset_key, float gain = 1.0f, float pitch = 1.0f);

    GameStartupOptions startup_{};
    std::unique_ptr<client::net::ClientConnection> connection_{};
    std::unique_ptr<client::domain::TankDriveController> tank_drive_{};
    karma::ecs::Entity tank_entity_{};
    karma::renderer::MeshId tank_mesh_id_ = karma::renderer::kInvalidMesh;
    karma::renderer::MaterialId tank_material_id_ = karma::renderer::kInvalidMaterial;
    float tank_model_scale_ = 1.0f;
    float tank_model_yaw_offset_radians_ = 0.0f;
    float tank_camera_height_ = 4.0f;
    float tank_camera_follow_distance_ = 10.0f;
    float tank_camera_look_ahead_ = 4.0f;
    bool console_visible_ = false;
    bool chat_entry_focused_ = false;
    bool console_toggle_was_down_ = false;
    bool escape_was_down_ = false;
    bool chat_was_down_ = false;
    bool spawn_was_down_ = false;
    bool fire_was_down_ = false;
};

} // namespace bz3
