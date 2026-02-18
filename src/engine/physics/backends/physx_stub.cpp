#include "physics/backends/factory_internal.hpp"

#include "karma/common/logging/logging.hpp"

#if !defined(KARMA_HAS_PHYSICS_PHYSX)

namespace karma::physics::backend {
namespace {

class PhysXBackendStub final : public Backend {
 public:
    const char* name() const override { return "physx"; }

    bool init() override {
        KARMA_TRACE("physics.physx", "PhysicsBackend[physx]: unavailable (not compiled)");
        return false;
    }

    void shutdown() override {}
    void beginFrame(float) override {}
    void simulateFixedStep(float) override {}
    void endFrame() override {}
    BodyId createBody(const BodyDesc&) override { return kInvalidBodyId; }
    void destroyBody(BodyId) override {}
    bool setBodyTransform(BodyId, const BodyTransform&) override { return false; }
    bool getBodyTransform(BodyId, BodyTransform&) const override { return false; }
    bool setBodyGravityEnabled(BodyId, bool) override { return false; }
    bool getBodyGravityEnabled(BodyId, bool&) const override { return false; }
    bool setBodyKinematic(BodyId, bool) override { return false; }
    bool getBodyKinematic(BodyId, bool&) const override { return false; }
    bool setBodyAwake(BodyId, bool) override { return false; }
    bool getBodyAwake(BodyId, bool&) const override { return false; }
    bool addBodyForce(BodyId, const glm::vec3&) override { return false; }
    bool addBodyLinearImpulse(BodyId, const glm::vec3&) override { return false; }
    bool addBodyTorque(BodyId, const glm::vec3&) override { return false; }
    bool addBodyAngularImpulse(BodyId, const glm::vec3&) override { return false; }
    bool setBodyLinearVelocity(BodyId, const glm::vec3&) override { return false; }
    bool getBodyLinearVelocity(BodyId, glm::vec3&) const override { return false; }
    bool setBodyAngularVelocity(BodyId, const glm::vec3&) override { return false; }
    bool getBodyAngularVelocity(BodyId, glm::vec3&) const override { return false; }
    bool setBodyLinearDamping(BodyId, float) override { return false; }
    bool getBodyLinearDamping(BodyId, float&) const override { return false; }
    bool setBodyAngularDamping(BodyId, float) override { return false; }
    bool getBodyAngularDamping(BodyId, float&) const override { return false; }
    bool setBodyRotationLocked(BodyId, bool) override { return false; }
    bool getBodyRotationLocked(BodyId, bool&) const override { return false; }
    bool setBodyTranslationLocked(BodyId, bool) override { return false; }
    bool getBodyTranslationLocked(BodyId, bool&) const override { return false; }
    bool setBodyTrigger(BodyId, bool) override { return false; }
    bool getBodyTrigger(BodyId, bool&) const override { return false; }
    bool setBodyCollisionMask(BodyId, const CollisionMask&) override { return false; }
    bool getBodyCollisionMask(BodyId, CollisionMask&) const override { return false; }
    bool setBodyFriction(BodyId, float) override { return false; }
    bool getBodyFriction(BodyId, float&) const override { return false; }
    bool setBodyRestitution(BodyId, float) override { return false; }
    bool getBodyRestitution(BodyId, float&) const override { return false; }
    bool raycastClosest(const glm::vec3&, const glm::vec3&, float, RaycastHit&) const override { return false; }
};

} // namespace

std::unique_ptr<Backend> CreatePhysXBackend() {
    return std::make_unique<PhysXBackendStub>();
}

} // namespace karma::physics::backend

#endif // !defined(KARMA_HAS_PHYSICS_PHYSX)
