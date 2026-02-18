#pragma once

#include "karma/physics/backend.hpp"

#include <memory>
#include <string>

namespace karma::physics {

class PhysicsSystem {
 public:
    void setBackend(physics::backend::BackendKind backend) { requested_backend_ = backend; }
    physics::backend::BackendKind requestedBackend() const { return requested_backend_; }
    physics::backend::BackendKind selectedBackend() const { return selected_backend_; }
    const char* selectedBackendName() const;
    bool isInitialized() const { return initialized_; }

    void init();
    void shutdown();
    void beginFrame(float dt);
    void simulateFixedStep(float fixed_dt);
    void endFrame();

    // Forwards the full backend-neutral body descriptor, including collider shape/material metadata
    // (for example mesh asset path ingestion for static mesh colliders).
    physics::backend::BodyId createBody(const physics::backend::BodyDesc& desc);
    void destroyBody(physics::backend::BodyId body);
    bool setBodyTransform(physics::backend::BodyId body, const physics::backend::BodyTransform& transform);
    bool getBodyTransform(physics::backend::BodyId body, physics::backend::BodyTransform& out_transform) const;
    bool setBodyGravityEnabled(physics::backend::BodyId body, bool enabled);
    bool getBodyGravityEnabled(physics::backend::BodyId body, bool& out_enabled) const;
    bool setBodyKinematic(physics::backend::BodyId body, bool enabled);
    bool getBodyKinematic(physics::backend::BodyId body, bool& out_enabled) const;
    bool setBodyAwake(physics::backend::BodyId body, bool enabled);
    bool getBodyAwake(physics::backend::BodyId body, bool& out_enabled) const;
    bool addBodyForce(physics::backend::BodyId body, const glm::vec3& force);
    bool addBodyLinearImpulse(physics::backend::BodyId body, const glm::vec3& impulse);
    bool addBodyTorque(physics::backend::BodyId body, const glm::vec3& torque);
    bool addBodyAngularImpulse(physics::backend::BodyId body, const glm::vec3& impulse);
    bool setBodyLinearVelocity(physics::backend::BodyId body, const glm::vec3& velocity);
    bool getBodyLinearVelocity(physics::backend::BodyId body, glm::vec3& out_velocity) const;
    bool setBodyAngularVelocity(physics::backend::BodyId body, const glm::vec3& velocity);
    bool getBodyAngularVelocity(physics::backend::BodyId body, glm::vec3& out_velocity) const;
    bool setBodyLinearDamping(physics::backend::BodyId body, float damping);
    bool getBodyLinearDamping(physics::backend::BodyId body, float& out_damping) const;
    bool setBodyAngularDamping(physics::backend::BodyId body, float damping);
    bool getBodyAngularDamping(physics::backend::BodyId body, float& out_damping) const;
    bool setBodyRotationLocked(physics::backend::BodyId body, bool locked);
    bool getBodyRotationLocked(physics::backend::BodyId body, bool& out_locked) const;
    bool setBodyTranslationLocked(physics::backend::BodyId body, bool locked);
    bool getBodyTranslationLocked(physics::backend::BodyId body, bool& out_locked) const;
    bool setBodyTrigger(physics::backend::BodyId body, bool enabled);
    bool getBodyTrigger(physics::backend::BodyId body, bool& out_enabled) const;
    bool setBodyCollisionMask(physics::backend::BodyId body, const physics::backend::CollisionMask& mask);
    bool getBodyCollisionMask(physics::backend::BodyId body, physics::backend::CollisionMask& out_mask) const;
    bool setBodyFriction(physics::backend::BodyId body, float friction);
    bool getBodyFriction(physics::backend::BodyId body, float& out_friction) const;
    bool setBodyRestitution(physics::backend::BodyId body, float restitution);
    bool getBodyRestitution(physics::backend::BodyId body, float& out_restitution) const;
    bool raycastClosest(const glm::vec3& origin,
                        const glm::vec3& direction,
                        float max_distance,
                        physics::backend::RaycastHit& out_hit) const;

 private:
    physics::backend::BackendKind requested_backend_ = physics::backend::BackendKind::Auto;
    physics::backend::BackendKind selected_backend_ = physics::backend::BackendKind::Auto;
    std::unique_ptr<physics::backend::Backend> backend_{};
    bool initialized_ = false;
};

} // namespace karma::physics
