#pragma once

#include "karma/renderer/backend.hpp"

namespace karma::renderer::backend {

std::unique_ptr<Backend> CreateBgfxBackend(karma::window::Window& window);
std::unique_ptr<Backend> CreateDiligentBackend(karma::window::Window& window);

} // namespace karma::renderer::backend
