#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include "karma/renderer/types.hpp"

namespace karma::renderer::assets {

struct SceneMesh {
    MeshData mesh{};
    MaterialDesc material{};
    uint32_t material_index = 0;
    glm::mat4 transform{1.0f};
};

bool LoadMesh(const std::filesystem::path& path, MeshData& out);
bool LoadScene(const std::filesystem::path& path, std::vector<SceneMesh>& out);

} // namespace karma::renderer::assets
