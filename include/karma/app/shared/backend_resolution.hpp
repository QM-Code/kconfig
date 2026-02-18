#pragma once

#include "karma/audio/backend.hpp"
#include "karma/physics/backend.hpp"

#include <string>

namespace karma::app::shared {

physics::backend::BackendKind ResolvePhysicsBackendFromOption(
    const std::string& option_value,
    bool option_explicit);
audio::backend::BackendKind ResolveAudioBackendFromOption(
    const std::string& option_value,
    bool option_explicit);

} // namespace karma::app::shared
