#pragma once

#include "karma/renderer/backend.hpp"
#include "karma/ui/ui_system.hpp"

#include <optional>
#include <string>

namespace karma::app::client {

renderer_backend::BackendKind ResolveRenderBackendFromOption(
    const std::string& option_value,
    bool option_explicit);
std::string CompiledPlatformBackendName();
void ValidatePlatformBackendFromOption(
    const std::string& option_value,
    bool option_explicit);
std::string ReadPreferredVideoDriverFromConfig();

std::optional<ui::Backend> ResolveUiBackendOverrideFromOption(
    const std::string& option_value,
    bool option_explicit);

} // namespace karma::app::client
