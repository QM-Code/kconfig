#pragma once

namespace karma::physics {

struct PhysicsMaterial {
    float friction = 0.0f;
    float restitution = 0.0f;
    float rolling_friction = 0.0f;
    float spinning_friction = 0.0f;
};

} // namespace karma::physics
