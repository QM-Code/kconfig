#pragma once

namespace karma::window { class Window; }
namespace karma::renderer { class GraphicsDevice; class RenderSystem; }
namespace karma::input { class InputContext; }
namespace karma::ecs { class World; }
namespace karma::scene { class Scene; }
namespace karma::ui { class UiDrawContext; }
namespace karma::audio { class AudioSystem; }

namespace karma::app::client {

class GameInterface {
 public:
    virtual ~GameInterface() = default;
    virtual void onStart() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onUiStart() {}
    virtual void onUiUpdate(float dt, ui::UiDrawContext& ui) {
        (void)dt;
        (void)ui;
    }
    virtual void onUiShutdown() {}
    virtual void onShutdown() = 0;

 protected:
    window::Window* window = nullptr;
    renderer::GraphicsDevice* graphics = nullptr;
    renderer::RenderSystem* render = nullptr;
    ecs::World* world = nullptr;
    scene::Scene* scene = nullptr;
    input::InputContext* input = nullptr;
    audio::AudioSystem* audio = nullptr;

 private:
    friend class Engine;
    void bind(window::Window& w, renderer::GraphicsDevice& g, renderer::RenderSystem& r, ecs::World& world_ref, scene::Scene& s,
              input::InputContext& in, audio::AudioSystem& audio_system) {
        window = &w;
        graphics = &g;
        render = &r;
        world = &world_ref;
        scene = &s;
        input = &in;
        audio = &audio_system;
    }
};

} // namespace karma::app::client
