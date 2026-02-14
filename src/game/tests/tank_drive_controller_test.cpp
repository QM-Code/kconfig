#include "client/domain/tank_drive_controller.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

bool Fail(const std::string& message) {
    std::cerr << message << "\n";
    return false;
}

bool ExpectNear(float actual, float expected, float epsilon, const std::string& message) {
    if (std::fabs(actual - expected) > epsilon) {
        return Fail(message + " (actual=" + std::to_string(actual) +
                    " expected=" + std::to_string(expected) + ")");
    }
    return true;
}

bool TestForwardMotion() {
    bz3::client::domain::TankDriveController controller{};
    bz3::client::domain::TankDriveParams params{};
    params.forward_speed = 8.0f;
    params.reverse_speed = 5.0f;
    params.turn_speed = 2.0f;
    controller.setParams(params);
    controller.setState(bz3::client::domain::TankDriveState{});

    controller.update(1.0f, bz3::client::domain::TankDriveInput{1.0f, 0.0f});
    const auto& state = controller.state();
    return ExpectNear(state.position.x, 0.0f, 1e-4f, "forward: position.x mismatch")
        && ExpectNear(state.position.z, 8.0f, 1e-4f, "forward: position.z mismatch")
        && ExpectNear(state.speed, 8.0f, 1e-4f, "forward: speed mismatch");
}

bool TestReverseMotionUsesReverseSpeed() {
    bz3::client::domain::TankDriveController controller{};
    bz3::client::domain::TankDriveParams params{};
    params.forward_speed = 8.0f;
    params.reverse_speed = 3.0f;
    params.turn_speed = 2.0f;
    controller.setParams(params);
    controller.setState(bz3::client::domain::TankDriveState{});

    controller.update(1.0f, bz3::client::domain::TankDriveInput{-1.0f, 0.0f});
    const auto& state = controller.state();
    return ExpectNear(state.position.z, -3.0f, 1e-4f, "reverse: position.z mismatch")
        && ExpectNear(state.speed, -3.0f, 1e-4f, "reverse: speed mismatch");
}

bool TestTurnAffectsHeading() {
    bz3::client::domain::TankDriveController controller{};
    bz3::client::domain::TankDriveParams params{};
    params.forward_speed = 10.0f;
    params.reverse_speed = 5.0f;
    params.turn_speed = 1.5f;
    controller.setParams(params);
    controller.setState(bz3::client::domain::TankDriveState{});

    controller.update(1.0f, bz3::client::domain::TankDriveInput{1.0f, 1.0f});
    const auto& state = controller.state();
    return ExpectNear(state.yaw_radians, 1.5f, 1e-4f, "turn: yaw mismatch")
        && ExpectNear(state.position.x, std::sin(1.5f) * 10.0f, 1e-3f, "turn: position.x mismatch")
        && ExpectNear(state.position.z, std::cos(1.5f) * 10.0f, 1e-3f, "turn: position.z mismatch");
}

} // namespace

int main() {
    if (!TestForwardMotion()) {
        return 1;
    }
    if (!TestReverseMotionUsesReverseSpeed()) {
        return 1;
    }
    if (!TestTurnAffectsHeading()) {
        return 1;
    }
    return 0;
}
