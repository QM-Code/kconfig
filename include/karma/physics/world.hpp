#pragma once

#include "karma/physics/backend.hpp"
#include "karma/physics/player_controller.hpp"
#include "karma/physics/rigid_body.hpp"
#include "karma/physics/static_body.hpp"
#include "karma/physics/types.hpp"

#include <memory>
#include <string>

#include <glm/glm.hpp>

namespace karma::physics {

class PhysicsSystem;

class World {
 public:
    World();
    explicit World(PhysicsSystem& system);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&& other) noexcept;
    World& operator=(World&& other) noexcept;

    void setBackend(physics_backend::BackendKind backend);
    physics_backend::BackendKind requestedBackend() const;
    physics_backend::BackendKind selectedBackend() const;
    const char* selectedBackendName() const;
    bool isInitialized() const;

    void init();
    void shutdown();
    void beginFrame(float dt);
    void simulateFixedStep(float fixed_dt);
    void endFrame();
    void update(float dt);

    void setGravity(float gravity);
    float gravity() const;

    RigidBody createBoxBody(const glm::vec3& half_extents,
                            float mass,
                            const glm::vec3& position,
                            const PhysicsMaterial& material = {});
    StaticBody createStaticMesh(const std::string& mesh_path);

    PlayerController& createPlayer();
    PlayerController& createPlayer(const glm::vec3& size);
    PlayerController* playerController();
    const PlayerController* playerController() const;

    bool raycast(const glm::vec3& from,
                 const glm::vec3& to,
                 glm::vec3& hit_point,
                 // Phase 2a contract: hit normals are not yet surfaced by the BodyId substrate.
                 // On hit this is set to (0,0,0).
                 glm::vec3& hit_normal) const;

 private:
    class Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace karma::physics
