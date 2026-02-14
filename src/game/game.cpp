#include "game.hpp"

#include "client/domain/tank_drive_controller.hpp"
#include "client/net/client_connection.hpp"
#include "karma/audio/backend.hpp"
#include "karma/audio/audio_system.hpp"
#include "karma/common/config_helpers.hpp"
#include "karma/common/config_store.hpp"
#include "karma/common/logging.hpp"
#include "karma/ecs/world.hpp"
#include "karma/input/input_system.hpp"
#include "karma/renderer/layers.hpp"
#include "karma/renderer/render_system.hpp"
#include "karma/scene/components.hpp"
#include "karma/ui/ui_draw_context.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <string>
#include <utility>

namespace bz3 {

Game::Game(GameStartupOptions options) : startup_(std::move(options)) {}

Game::~Game() = default;

void Game::onStart() {
    syncInputMode();
    static_cast<void>(ensureLocalTankEntity());

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
    static_cast<void>(ensureLocalTankEntity());
    if (connection_) {
        connection_->poll();
        if (connection_->shouldExit()) {
            spdlog::error("Game: network join was rejected, closing client.");
            std::exit(1);
        }
    }

    if (!input) {
        chat_entry_focused_ = false;
        console_toggle_was_down_ = false;
        escape_was_down_ = false;
        chat_was_down_ = false;
        spawn_was_down_ = false;
        fire_was_down_ = false;
        return;
    }

    const bool console_down = input->global().actionDown("console");
    const bool escape_down = input->global().actionDown("quickMenu");
    const bool chat_down = input->game().actionDown("chat");
    const bool console_pressed =
        input->global().actionPressed("console") || (console_down && !console_toggle_was_down_);
    const bool escape_pressed =
        input->global().actionPressed("quickMenu") || (escape_down && !escape_was_down_);
    const bool chat_pressed =
        input->game().actionPressed("chat") || (chat_down && !chat_was_down_);

    if (console_pressed) {
        console_visible_ = !console_visible_;
        if (!console_visible_) {
            chat_entry_focused_ = false;
        }
    } else if (chat_pressed) {
        console_visible_ = true;
        chat_entry_focused_ = true;
    } else if (console_visible_ && escape_pressed) {
        console_visible_ = false;
        chat_entry_focused_ = false;
    }

    syncInputMode();

    const bool spawn_down = input->game().actionDown("spawn");
    const bool fire_down = input->game().actionDown("fire");

    const bool spawn_pressed =
        input->game().actionPressed("spawn") || (spawn_down && !spawn_was_down_);
    const bool fire_pressed =
        input->game().actionPressed("fire") || (fire_down && !fire_was_down_);

    const bool gameplay_input_enabled = !console_visible_ && !chat_entry_focused_;

    updateLocalTank(dt, gameplay_input_enabled);

    if (gameplay_input_enabled && spawn_pressed) {
        if (connection_ && connection_->isConnected()) {
            (void)connection_->sendRequestPlayerSpawn();
        }
    }

    if (gameplay_input_enabled && fire_pressed) {
        // Play local fire immediately for responsive feedback.
        onAudioEvent(client::net::AudioEvent::ShotFire);
        if (connection_ && connection_->isConnected()) {
            (void)connection_->sendCreateShot();
        }
    }

    console_toggle_was_down_ = console_down;
    escape_was_down_ = escape_down;
    chat_was_down_ = chat_down;
    spawn_was_down_ = spawn_down;
    fire_was_down_ = fire_down;
}

void Game::onUiUpdate(float dt, karma::ui::UiDrawContext& ui) {
    (void)dt;
    const bool connected = connection_ && connection_->isConnected();
    const bool hud_visible = connected || !console_visible_;

    if (hud_visible) {
        karma::ui::UiDrawContext::TextPanel hud{};
        hud.title = "BZ3 HUD";
        hud.lines.push_back("Player: " + startup_.player_name);
        if (startup_.connect_on_start) {
            hud.lines.push_back("Connect target: " + startup_.connect_addr + ":" +
                                std::to_string(startup_.connect_port));
            hud.lines.push_back(std::string("Connection: ") + (connected ? "connected" : "connecting"));
        } else {
            hud.lines.push_back("Connection: offline");
        }
        hud.lines.push_back(console_visible_ ? "Console: open" : "Console: closed");
        hud.lines.push_back(chat_entry_focused_ ? "Chat entry focus: active" : "Chat entry focus: idle");
        hud.lines.push_back("HUD rule: connected || !console");
        if (tank_drive_) {
            const auto& tank = tank_drive_->state();
            hud.lines.push_back("Tank: arrow keys to drive");
            hud.lines.push_back("Tank pos: (" + std::to_string(tank.position.x) + ", " +
                                std::to_string(tank.position.y) + ", " +
                                std::to_string(tank.position.z) + ")");
        } else {
            hud.lines.push_back("Tank: unavailable (player model failed to load)");
        }
        ui.addTextPanel(std::move(hud));
    }

    if (console_visible_) {
        karma::ui::UiDrawContext::TextPanel console{};
        console.title = "BZ3 Console";
        console.x = 72.0f;
        console.y = 84.0f;
        console.lines.push_back("Console parity slice (engine lifecycle + game state mapping).");
        console.lines.push_back("` toggle: open/close");
        console.lines.push_back("Chat action: focus chat entry + open console");
        console.lines.push_back("Esc: close console");
        console.lines.push_back(chat_entry_focused_ ? "Chat entry focus is active." : "Chat entry focus is idle.");
        console.lines.push_back("Gameplay input blocked while console/chat focus is active.");
        ui.addTextPanel(std::move(console));
    }
}

void Game::onShutdown() {
    if (connection_) {
        connection_->shutdown();
        connection_.reset();
    }
    destroyLocalTankEntity();
    console_visible_ = false;
    chat_entry_focused_ = false;
    syncInputMode();
}

void Game::syncInputMode() {
    if (!input) {
        return;
    }

    if (world && tank_entity_.isValid() && world->isAlive(tank_entity_)) {
        input->setMode(karma::input::InputMode::Game);
        return;
    }

    const bool ui_focus_active = console_visible_ || chat_entry_focused_;
    input->setMode(ui_focus_active ? karma::input::InputMode::Game
                                   : karma::input::InputMode::Roaming);
}

bool Game::ensureLocalTankEntity() {
    if (!world || !graphics || !render) {
        return false;
    }
    if (tank_entity_.isValid() && world->isAlive(tank_entity_)) {
        return true;
    }

    destroyLocalTankEntity();

    const auto model_path = karma::config::ConfigStore::ResolveAssetPath("assets.models.playerModel", {});
    if (model_path.empty()) {
        spdlog::error("Game: failed to resolve assets.models.playerModel");
        return false;
    }

    tank_mesh_id_ = graphics->createMeshFromFile(model_path);
    if (tank_mesh_id_ == karma::renderer::kInvalidMesh) {
        spdlog::error("Game: failed to create tank mesh from '{}'", model_path.string());
        return false;
    }

    karma::renderer::MaterialDesc tank_material{};
    tank_material.base_color = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
    tank_material_id_ = graphics->createMaterial(tank_material);
    if (tank_material_id_ == karma::renderer::kInvalidMaterial) {
        graphics->destroyMesh(tank_mesh_id_);
        tank_mesh_id_ = karma::renderer::kInvalidMesh;
        spdlog::error("Game: failed to create tank material");
        return false;
    }

    tank_drive_ = std::make_unique<client::domain::TankDriveController>();
    client::domain::TankDriveParams drive_params{};
    drive_params.forward_speed =
        std::max(0.0f, karma::config::ReadFloatConfig({"gameplay.tank.forwardSpeed"}, 8.0f));
    drive_params.reverse_speed =
        std::max(0.0f, karma::config::ReadFloatConfig({"gameplay.tank.reverseSpeed"}, 5.0f));
    drive_params.turn_speed =
        std::max(0.0f, karma::config::ReadFloatConfig({"gameplay.tank.turnSpeed"}, 2.0f));
    tank_drive_->setParams(drive_params);

    client::domain::TankDriveState initial_state{};
    initial_state.position = glm::vec3(
        karma::config::ReadFloatConfig({"gameplay.tank.startX"}, 0.0f),
        karma::config::ReadFloatConfig({"gameplay.tank.startY"}, 0.6f),
        karma::config::ReadFloatConfig({"gameplay.tank.startZ"}, 0.0f));
    initial_state.yaw_radians =
        karma::config::ReadFloatConfig({"gameplay.tank.startYawDegrees"}, 0.0f)
        * glm::pi<float>() / 180.0f;
    tank_drive_->setState(initial_state);

    tank_model_scale_ = std::max(0.01f, karma::config::ReadFloatConfig({"gameplay.tank.modelScale"}, 1.0f));
    tank_model_yaw_offset_radians_ =
        karma::config::ReadFloatConfig({"gameplay.tank.modelYawOffsetDegrees"}, 0.0f)
        * glm::pi<float>() / 180.0f;

    tank_camera_height_ =
        std::max(0.1f, karma::config::ReadFloatConfig({"gameplay.tank.cameraHeight"}, 4.0f));
    tank_camera_follow_distance_ =
        std::max(0.1f, karma::config::ReadFloatConfig({"gameplay.tank.cameraDistance"}, 10.0f));
    tank_camera_look_ahead_ =
        std::max(0.0f, karma::config::ReadFloatConfig({"gameplay.tank.cameraLookAhead"}, 4.0f));

    tank_entity_ = world->createEntity();

    karma::scene::TransformComponent transform{};
    world->add(tank_entity_, transform);

    karma::scene::RenderComponent render_component{};
    render_component.mesh = tank_mesh_id_;
    render_component.material = tank_material_id_;
    render_component.layer = karma::renderer::kLayerWorld;
    render_component.casts_shadow = true;
    world->add(tank_entity_, render_component);

    updateLocalTankTransform();
    updateTankFollowCamera();
    syncInputMode();

    KARMA_TRACE("game.client",
                "Game: local tank entity ready model='{}' entity={} forward_speed={:.2f} reverse_speed={:.2f} turn_speed={:.2f}",
                model_path.string(),
                tank_entity_.index,
                drive_params.forward_speed,
                drive_params.reverse_speed,
                drive_params.turn_speed);
    return true;
}

void Game::destroyLocalTankEntity() {
    if (world && tank_entity_.isValid() && world->isAlive(tank_entity_)) {
        world->destroyEntity(tank_entity_);
    }
    tank_entity_ = {};

    if (graphics && tank_material_id_ != karma::renderer::kInvalidMaterial) {
        graphics->destroyMaterial(tank_material_id_);
    }
    tank_material_id_ = karma::renderer::kInvalidMaterial;

    if (graphics && tank_mesh_id_ != karma::renderer::kInvalidMesh) {
        graphics->destroyMesh(tank_mesh_id_);
    }
    tank_mesh_id_ = karma::renderer::kInvalidMesh;
    tank_drive_.reset();
}

void Game::updateLocalTank(float dt, bool gameplay_input_enabled) {
    if (!tank_drive_) {
        return;
    }

    client::domain::TankDriveInput drive_input{};
    if (gameplay_input_enabled && input) {
        if (input->game().actionDown("moveForward")) {
            drive_input.throttle += 1.0f;
        }
        if (input->game().actionDown("moveBackward")) {
            drive_input.throttle -= 1.0f;
        }
        if (input->game().actionDown("moveLeft")) {
            drive_input.steering += 1.0f;
        }
        if (input->game().actionDown("moveRight")) {
            drive_input.steering -= 1.0f;
        }
    }

    tank_drive_->update(std::max(0.0f, dt), drive_input);
    updateLocalTankTransform();
    updateTankFollowCamera();
}

void Game::updateLocalTankTransform() {
    if (!world || !tank_entity_.isValid() || !world->isAlive(tank_entity_) || !tank_drive_) {
        return;
    }

    auto* transform = world->tryGet<karma::scene::TransformComponent>(tank_entity_);
    if (!transform) {
        return;
    }

    const auto& tank = tank_drive_->state();
    const float yaw = tank.yaw_radians + tank_model_yaw_offset_radians_;
    glm::mat4 local(1.0f);
    local = glm::translate(local, tank.position);
    local = glm::rotate(local, yaw, glm::vec3{0.0f, 1.0f, 0.0f});
    local = glm::scale(local, glm::vec3{tank_model_scale_, tank_model_scale_, tank_model_scale_});
    transform->local = local;
    transform->world = local;
}

void Game::updateTankFollowCamera() {
    if (!render || !tank_drive_) {
        return;
    }

    const auto& tank = tank_drive_->state();
    const glm::vec3 forward = client::domain::TankDriveController::ForwardFromYaw(tank.yaw_radians);

    auto camera = render->camera();
    camera.position =
        tank.position - (forward * tank_camera_follow_distance_) + glm::vec3{0.0f, tank_camera_height_, 0.0f};
    camera.target = tank.position + glm::vec3{0.0f, 1.2f, 0.0f} + (forward * tank_camera_look_ahead_);
    render->setCamera(camera);
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
