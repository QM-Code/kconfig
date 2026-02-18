#include "karma/physics/physics_system.hpp"

#include "karma/common/logging/logging.hpp"
#include "karma/physics/backend.hpp"

#include <sstream>

namespace karma::physics {
namespace {

std::string CompiledBackendList() {
    const auto compiled = physics::backend::CompiledBackends();
    if (compiled.empty()) {
        return "(none)";
    }

    std::ostringstream out;
    for (size_t i = 0; i < compiled.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << physics::backend::BackendKindName(compiled[i]);
    }
    return out.str();
}

} // namespace

const char* PhysicsSystem::selectedBackendName() const {
    return physics::backend::BackendKindName(selected_backend_);
}

void PhysicsSystem::init() {
    if (initialized_) {
        return;
    }

    KARMA_TRACE("physics.system",
                "PhysicsSystem: creating backend requested='{}' compiled='{}'",
                physics::backend::BackendKindName(requested_backend_),
                CompiledBackendList());
    backend_ = physics::backend::CreateBackend(requested_backend_, &selected_backend_);
    if (!backend_) {
        selected_backend_ = physics::backend::BackendKind::Auto;
        return;
    }

    if (!backend_->init()) {
        KARMA_TRACE("physics.system",
                    "PhysicsSystem: backend '{}' init failed",
                    backend_->name());
        backend_.reset();
        selected_backend_ = physics::backend::BackendKind::Auto;
        return;
    }

    initialized_ = true;
    KARMA_TRACE("physics.system",
                "PhysicsSystem: backend ready selected='{}'",
                backend_->name());
}

void PhysicsSystem::shutdown() {
    if (!backend_) {
        initialized_ = false;
        selected_backend_ = physics::backend::BackendKind::Auto;
        return;
    }

    backend_->shutdown();
    backend_.reset();
    initialized_ = false;
    selected_backend_ = physics::backend::BackendKind::Auto;
}

void PhysicsSystem::beginFrame(float dt) {
    if (!backend_) {
        return;
    }
    backend_->beginFrame(dt);
}

void PhysicsSystem::simulateFixedStep(float fixed_dt) {
    if (!backend_) {
        return;
    }
    backend_->simulateFixedStep(fixed_dt);
}

void PhysicsSystem::endFrame() {
    if (!backend_) {
        return;
    }
    backend_->endFrame();
}

physics::backend::BodyId PhysicsSystem::createBody(const physics::backend::BodyDesc& desc) {
    if (!backend_) {
        return physics::backend::kInvalidBodyId;
    }
    // BodyDesc is passed through as-is so backend shape-local offsets and mesh-ingestion intent stay substrate-owned.
    return backend_->createBody(desc);
}

void PhysicsSystem::destroyBody(physics::backend::BodyId body) {
    if (!backend_) {
        return;
    }
    backend_->destroyBody(body);
}

bool PhysicsSystem::setBodyTransform(physics::backend::BodyId body,
                                     const physics::backend::BodyTransform& transform) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyTransform(body, transform);
}

bool PhysicsSystem::getBodyTransform(physics::backend::BodyId body,
                                     physics::backend::BodyTransform& out_transform) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyTransform(body, out_transform);
}

bool PhysicsSystem::setBodyGravityEnabled(physics::backend::BodyId body, bool enabled) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyGravityEnabled(body, enabled);
}

bool PhysicsSystem::getBodyGravityEnabled(physics::backend::BodyId body, bool& out_enabled) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyGravityEnabled(body, out_enabled);
}

bool PhysicsSystem::setBodyKinematic(physics::backend::BodyId body, bool enabled) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyKinematic(body, enabled);
}

bool PhysicsSystem::getBodyKinematic(physics::backend::BodyId body, bool& out_enabled) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyKinematic(body, out_enabled);
}

bool PhysicsSystem::setBodyAwake(physics::backend::BodyId body, bool enabled) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyAwake(body, enabled);
}

bool PhysicsSystem::getBodyAwake(physics::backend::BodyId body, bool& out_enabled) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyAwake(body, out_enabled);
}

bool PhysicsSystem::addBodyForce(physics::backend::BodyId body, const glm::vec3& force) {
    if (!backend_) {
        return false;
    }
    return backend_->addBodyForce(body, force);
}

bool PhysicsSystem::addBodyLinearImpulse(physics::backend::BodyId body, const glm::vec3& impulse) {
    if (!backend_) {
        return false;
    }
    return backend_->addBodyLinearImpulse(body, impulse);
}

bool PhysicsSystem::addBodyTorque(physics::backend::BodyId body, const glm::vec3& torque) {
    if (!backend_) {
        return false;
    }
    return backend_->addBodyTorque(body, torque);
}

bool PhysicsSystem::addBodyAngularImpulse(physics::backend::BodyId body, const glm::vec3& impulse) {
    if (!backend_) {
        return false;
    }
    return backend_->addBodyAngularImpulse(body, impulse);
}

bool PhysicsSystem::setBodyLinearVelocity(physics::backend::BodyId body, const glm::vec3& velocity) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyLinearVelocity(body, velocity);
}

bool PhysicsSystem::getBodyLinearVelocity(physics::backend::BodyId body, glm::vec3& out_velocity) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyLinearVelocity(body, out_velocity);
}

bool PhysicsSystem::setBodyAngularVelocity(physics::backend::BodyId body, const glm::vec3& velocity) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyAngularVelocity(body, velocity);
}

bool PhysicsSystem::getBodyAngularVelocity(physics::backend::BodyId body, glm::vec3& out_velocity) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyAngularVelocity(body, out_velocity);
}

bool PhysicsSystem::setBodyLinearDamping(physics::backend::BodyId body, float damping) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyLinearDamping(body, damping);
}

bool PhysicsSystem::getBodyLinearDamping(physics::backend::BodyId body, float& out_damping) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyLinearDamping(body, out_damping);
}

bool PhysicsSystem::setBodyAngularDamping(physics::backend::BodyId body, float damping) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyAngularDamping(body, damping);
}

bool PhysicsSystem::getBodyAngularDamping(physics::backend::BodyId body, float& out_damping) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyAngularDamping(body, out_damping);
}

bool PhysicsSystem::setBodyRotationLocked(physics::backend::BodyId body, bool locked) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyRotationLocked(body, locked);
}

bool PhysicsSystem::getBodyRotationLocked(physics::backend::BodyId body, bool& out_locked) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyRotationLocked(body, out_locked);
}

bool PhysicsSystem::setBodyTranslationLocked(physics::backend::BodyId body, bool locked) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyTranslationLocked(body, locked);
}

bool PhysicsSystem::getBodyTranslationLocked(physics::backend::BodyId body, bool& out_locked) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyTranslationLocked(body, out_locked);
}

bool PhysicsSystem::setBodyTrigger(physics::backend::BodyId body, bool enabled) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyTrigger(body, enabled);
}

bool PhysicsSystem::getBodyTrigger(physics::backend::BodyId body, bool& out_enabled) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyTrigger(body, out_enabled);
}

bool PhysicsSystem::setBodyCollisionMask(physics::backend::BodyId body, const physics::backend::CollisionMask& mask) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyCollisionMask(body, mask);
}

bool PhysicsSystem::getBodyCollisionMask(physics::backend::BodyId body, physics::backend::CollisionMask& out_mask) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyCollisionMask(body, out_mask);
}

bool PhysicsSystem::setBodyFriction(physics::backend::BodyId body, float friction) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyFriction(body, friction);
}

bool PhysicsSystem::getBodyFriction(physics::backend::BodyId body, float& out_friction) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyFriction(body, out_friction);
}

bool PhysicsSystem::setBodyRestitution(physics::backend::BodyId body, float restitution) {
    if (!backend_) {
        return false;
    }
    return backend_->setBodyRestitution(body, restitution);
}

bool PhysicsSystem::getBodyRestitution(physics::backend::BodyId body, float& out_restitution) const {
    if (!backend_) {
        return false;
    }
    return backend_->getBodyRestitution(body, out_restitution);
}

bool PhysicsSystem::raycastClosest(const glm::vec3& origin,
                                   const glm::vec3& direction,
                                   float max_distance,
                                   physics::backend::RaycastHit& out_hit) const {
    if (!backend_) {
        return false;
    }
    return backend_->raycastClosest(origin, direction, max_distance, out_hit);
}

} // namespace karma::physics
