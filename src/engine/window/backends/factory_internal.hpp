#pragma once

#include "karma/window/window.hpp"

namespace karma::window::backend {

std::unique_ptr<Window> CreateSdl3WindowBackend(const WindowConfig& config);

} // namespace karma::window::backend
