#pragma once

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace karma::physics {

class World;

class RigidBody {
 public:
    RigidBody();
    ~RigidBody();

    RigidBody(const RigidBody&) = delete;
    RigidBody& operator=(const RigidBody&) = delete;
    RigidBody(RigidBody&& other) noexcept;
    RigidBody& operator=(RigidBody&& other) noexcept;

    bool isValid() const;

    glm::vec3 getPosition() const;
    glm::quat getRotation() const;

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setTransform(const glm::vec3& position, const glm::quat& rotation);

    bool setGravityEnabled(bool enabled);
    bool getGravityEnabled(bool& out_enabled) const;

    void destroy();

 private:
    class Impl;
    explicit RigidBody(std::unique_ptr<Impl> impl);
    static RigidBody CreateFacadeHandle(std::shared_ptr<void> world_state,
                                        uint64_t generation,
                                        uint64_t body);

    std::unique_ptr<Impl> impl_{};

    friend class World;
};

} // namespace karma::physics
