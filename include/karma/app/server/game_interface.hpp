#pragma once

namespace karma::ecs { class World; }

namespace karma::app::server {

class GameInterface {
 public:
    virtual ~GameInterface() = default;
    virtual void onStart() = 0;
    virtual void onTick(float dt) = 0;
    virtual void onShutdown() = 0;

 protected:
    ecs::World* world = nullptr;

 private:
    friend class Engine;
    void bind(ecs::World& world_ref) { world = &world_ref; }
};

} // namespace karma::app::server
