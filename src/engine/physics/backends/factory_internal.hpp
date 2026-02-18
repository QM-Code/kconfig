#pragma once

#include "karma/physics/backend.hpp"

namespace karma::physics::backend {

std::unique_ptr<Backend> CreateJoltBackend();
std::unique_ptr<Backend> CreatePhysXBackend();

} // namespace karma::physics::backend

