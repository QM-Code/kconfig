#pragma once

#include <vector>

#include "karma/ecs/entity.hpp"
#include "karma/renderer/types.hpp"

namespace karma::ecs {
class World;
}

namespace karma::renderer {
class GraphicsDevice;
}

namespace karma::scene {

struct StartupSceneResources {
    std::vector<ecs::Entity> entities{};
    std::vector<renderer::MeshId> meshes{};
    std::vector<renderer::MaterialId> materials{};
};

bool PopulateStartupWorld(renderer::GraphicsDevice& graphics,
                          ecs::World& world,
                          StartupSceneResources& out_resources);

void ReleaseStartupSceneResources(renderer::GraphicsDevice& graphics,
                                  ecs::World& world,
                                  StartupSceneResources& resources);

} // namespace karma::scene
