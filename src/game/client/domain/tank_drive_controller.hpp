#pragma once

#include <glm/glm.hpp>

namespace bz3::client::domain {

struct TankDriveInput {
    float throttle = 0.0f; // [-1, 1], forward/backward
    float steering = 0.0f; // [-1, 1], right/left
};

struct TankDriveState {
    glm::vec3 position{0.0f, 0.6f, 0.0f};
    float yaw_radians = 0.0f;
    float speed = 0.0f;
};

struct TankDriveParams {
    float forward_speed = 8.0f;
    float reverse_speed = 5.0f;
    float turn_speed = 2.0f;
};

class TankDriveController {
 public:
    void setParams(const TankDriveParams& params);
    void setState(const TankDriveState& state);

    const TankDriveParams& params() const { return params_; }
    const TankDriveState& state() const { return state_; }

    void update(float dt_seconds, const TankDriveInput& input);
    static glm::vec3 ForwardFromYaw(float yaw_radians);

 private:
    TankDriveParams params_{};
    TankDriveState state_{};
};

} // namespace bz3::client::domain
