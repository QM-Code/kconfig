#include "client/domain/tank_drive_controller.hpp"

#include <algorithm>
#include <cmath>

namespace bz3::client::domain {

namespace {

float ClampUnit(float value) {
    return std::clamp(value, -1.0f, 1.0f);
}

} // namespace

void TankDriveController::setParams(const TankDriveParams& params) {
    params_ = params;
    params_.forward_speed = std::max(0.0f, params_.forward_speed);
    params_.reverse_speed = std::max(0.0f, params_.reverse_speed);
    params_.turn_speed = std::max(0.0f, params_.turn_speed);
}

void TankDriveController::setState(const TankDriveState& state) {
    state_ = state;
}

void TankDriveController::update(float dt_seconds, const TankDriveInput& input) {
    if (dt_seconds <= 0.0f) {
        state_.speed = 0.0f;
        return;
    }

    const float throttle = ClampUnit(input.throttle);
    const float steering = ClampUnit(input.steering);

    state_.yaw_radians += steering * params_.turn_speed * dt_seconds;

    const float speed = (throttle >= 0.0f)
        ? (throttle * params_.forward_speed)
        : (throttle * params_.reverse_speed);
    state_.speed = speed;
    state_.position += ForwardFromYaw(state_.yaw_radians) * speed * dt_seconds;
}

glm::vec3 TankDriveController::ForwardFromYaw(float yaw_radians) {
    return glm::vec3{std::sin(yaw_radians), 0.0f, std::cos(yaw_radians)};
}

} // namespace bz3::client::domain
