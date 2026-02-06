#include "karma/scene/scene_bootstrap.hpp"

#include "geometry/mesh_loader.hpp"
#include "karma/common/config_store.hpp"
#include "karma/common/logging.hpp"
#include "karma/common/data_path_resolver.hpp"
#include "karma/ecs/world.hpp"
#include "karma/renderer/device.hpp"
#include "karma/renderer/layers.hpp"
#include "karma/scene/components.hpp"

#include <spdlog/spdlog.h>

namespace karma::scene {
namespace {

std::string DescribeJsonValue(const karma::json::Value* value) {
    if (!value) {
        return "<missing>";
    }
    if (value->is_string()) {
        return "'" + value->get<std::string>() + "'";
    }
    return value->dump();
}

} // namespace

bool PopulateStartupWorld(renderer::GraphicsDevice& graphics,
                          ecs::World& world,
                          StartupSceneResources& out_resources) {
    out_resources.entities.clear();
    out_resources.meshes.clear();
    out_resources.materials.clear();

    const std::filesystem::path world_path =
        config::ConfigStore::ResolveAssetPath("assets.models.world", {});
    if (world_path.empty()) {
        const auto alt_models_world = config::ConfigStore::ResolveAssetPath("models.world", {});
        const auto alt_world = config::ConfigStore::ResolveAssetPath("world", {});
        const auto* config_value = config::ConfigStore::Get("assets.models.world");
        spdlog::error(
            "EngineApp: failed to resolve startup world asset key 'assets.models.world' "
            "(config value: {}, data_root: '{}', diag models.world: '{}', diag world: '{}')",
            DescribeJsonValue(config_value),
            data::DataRoot().string(),
            alt_models_world.empty() ? "<missing>" : alt_models_world.string(),
            alt_world.empty() ? "<missing>" : alt_world.string());
        return false;
    }

    KARMA_TRACE("render.mesh", "EngineApp: loading startup world '{}'", world_path.string());
    std::vector<geometry::SceneMesh> scene_meshes;
    if (!geometry::LoadScene(world_path, scene_meshes)) {
        spdlog::error("EngineApp: failed to load startup world from {}", world_path.string());
        return false;
    }

    auto cleanup_and_fail = [&](const char* message) {
        spdlog::error("EngineApp: {}", message);
        for (const ecs::Entity entity : out_resources.entities) {
            world.destroyEntity(entity);
        }
        for (const renderer::MeshId mesh_id : out_resources.meshes) {
            if (mesh_id != renderer::kInvalidMesh) {
                graphics.destroyMesh(mesh_id);
            }
        }
        for (const renderer::MaterialId material_id : out_resources.materials) {
            if (material_id != renderer::kInvalidMaterial) {
                graphics.destroyMaterial(material_id);
            }
        }
        out_resources.entities.clear();
        out_resources.meshes.clear();
        out_resources.materials.clear();
        return false;
    };

    renderer::MaterialDesc material_desc{};
    material_desc.base_color = {0.8f, 0.8f, 0.8f, 1.0f};
    const renderer::MaterialId material_id = graphics.createMaterial(material_desc);
    if (material_id == renderer::kInvalidMaterial) {
        return cleanup_and_fail("failed to create startup material");
    }
    out_resources.materials.push_back(material_id);

    for (const auto& entry : scene_meshes) {
        const renderer::MeshId mesh_id = graphics.createMesh(entry.mesh);
        if (mesh_id == renderer::kInvalidMesh) {
            spdlog::error("EngineApp: failed to create mesh for startup world {}", world_path.string());
            continue;
        }

        const ecs::Entity entity = world.createEntity();
        TransformComponent transform{};
        transform.local = entry.transform;
        transform.world = entry.transform;
        world.add(entity, transform);

        RenderComponent render_component{};
        render_component.mesh = mesh_id;
        render_component.material = material_id;
        render_component.layer = renderer::kLayerWorld;
        world.add(entity, render_component);

        out_resources.meshes.push_back(mesh_id);
        out_resources.entities.push_back(entity);
    }

    if (out_resources.entities.empty()) {
        return cleanup_and_fail("startup world produced zero renderable entities");
    }

    KARMA_TRACE("ecs.world",
                "EngineApp: startup world entities={} world_entities={}",
                out_resources.entities.size(),
                world.entities().size());
    return true;
}

void ReleaseStartupSceneResources(renderer::GraphicsDevice& graphics,
                                  ecs::World& world,
                                  StartupSceneResources& resources) {
    for (const ecs::Entity entity : resources.entities) {
        world.destroyEntity(entity);
    }
    for (const renderer::MeshId mesh_id : resources.meshes) {
        if (mesh_id != renderer::kInvalidMesh) {
            graphics.destroyMesh(mesh_id);
        }
    }
    for (const renderer::MaterialId material_id : resources.materials) {
        if (material_id != renderer::kInvalidMaterial) {
            graphics.destroyMaterial(material_id);
        }
    }
    resources.entities.clear();
    resources.meshes.clear();
    resources.materials.clear();
    KARMA_TRACE("ecs.world", "EngineApp: startup world released world_entities={}", world.entities().size());
}

} // namespace karma::scene
