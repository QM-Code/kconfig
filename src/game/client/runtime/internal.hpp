#pragma once

#include "game.hpp"
#include "karma/app/client/engine.hpp"
#include "karma/cli/client/app_options.hpp"
#include "karma/cli/client/runtime_options.hpp"

#include <glm/glm.hpp>

#include <string>

namespace bz3::client::runtime_detail {

glm::vec3 ReadRequiredVec3(const char* path);
glm::vec4 ReadRequiredColor(const char* path);

bz3::GameStartupOptions ResolveGameStartupOptions(const karma::cli::client::AppOptions& options);
karma::app::client::EngineConfig BuildEngineConfig(const karma::cli::client::AppOptions& options);

} // namespace bz3::client::runtime_detail
