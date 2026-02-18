#pragma once

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace karma::physics {

class World;

class StaticBody {
 public:
    StaticBody();
    ~StaticBody();

    StaticBody(const StaticBody&) = delete;
    StaticBody& operator=(const StaticBody&) = delete;
    StaticBody(StaticBody&& other) noexcept;
    StaticBody& operator=(StaticBody&& other) noexcept;

    bool isValid() const;

    glm::vec3 getPosition() const;
    glm::quat getRotation() const;

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setTransform(const glm::vec3& position, const glm::quat& rotation);

    void destroy();

 private:
    class Impl;
    explicit StaticBody(std::unique_ptr<Impl> impl);
    static StaticBody CreateFacadeHandle(std::shared_ptr<void> world_state,
                                         uint64_t generation,
                                         uint64_t body);

    std::unique_ptr<Impl> impl_{};

    friend class World;
};

} // namespace karma::physics
