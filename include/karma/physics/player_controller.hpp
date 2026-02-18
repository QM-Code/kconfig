#pragma once

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace karma::physics {

class World;

class PlayerController {
 public:
    PlayerController();
    ~PlayerController();

    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;
    PlayerController(PlayerController&& other) noexcept;
    PlayerController& operator=(PlayerController&& other) noexcept;

    bool isValid() const;

    glm::vec3 getPosition() const;
    glm::quat getRotation() const;
    glm::vec3 getVelocity() const;
    glm::vec3 getForwardVector() const;
    glm::vec3 getCenter() const;
    glm::vec3 getHalfExtents() const;

    void setHalfExtents(const glm::vec3& half_extents);
    void setCenter(const glm::vec3& center);

    void update(float dt);

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setVelocity(const glm::vec3& velocity);

    bool isGrounded() const;

    void destroy();

 private:
    class Impl;
    explicit PlayerController(std::unique_ptr<Impl> impl);
    static PlayerController CreateFacadeHandle(std::shared_ptr<void> world_state,
                                               uint64_t generation,
                                               uint64_t body,
                                               const glm::vec3& half_extents);

    std::unique_ptr<Impl> impl_{};

    friend class World;
};

} // namespace karma::physics
