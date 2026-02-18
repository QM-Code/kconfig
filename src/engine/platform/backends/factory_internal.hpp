#pragma once

#include "karma/platform/window.hpp"

namespace karma::platform::backend {

std::unique_ptr<Window> CreateSdl3WindowBackend(const WindowConfig& config);

} // namespace karma::platform::backend
