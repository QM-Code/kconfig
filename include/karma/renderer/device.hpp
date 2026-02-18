#pragma once

#include <filesystem>

#include "karma/renderer/backend.hpp"
#include "karma/renderer/types.hpp"

namespace karma::renderer {

class GraphicsDevice {
 public:
    explicit GraphicsDevice(karma::window::Window& window,
                            renderer::backend::BackendKind preferred_backend = renderer::backend::BackendKind::Auto);
    ~GraphicsDevice();

    void beginFrame(int width, int height, float dt);
    void endFrame();

    MeshId createMesh(const MeshData& mesh);
    MeshId createMeshFromFile(const std::filesystem::path& path);
    void destroyMesh(MeshId mesh);

    MaterialId createMaterial(const MaterialDesc& material);
    void destroyMaterial(MaterialId material);

    void submit(const DrawItem& item);
    void submitDebugLine(const DebugLineItem& line);
    void renderFrame();
    void renderLayer(LayerId layer);
    void setCamera(const CameraData& camera);
    void setDirectionalLight(const DirectionalLightData& light);
    void setLights(const std::vector<LightData>& lights);
    void setEnvironmentLighting(const EnvironmentLightingData& environment);
    bool isValid() const;
    renderer::backend::BackendKind backendKind() const { return backend_kind_; }
    const char* backendName() const { return renderer::backend::BackendKindName(backend_kind_); }

 private:
    std::unique_ptr<renderer::backend::Backend> backend_;
    renderer::backend::BackendKind backend_kind_ = renderer::backend::BackendKind::Auto;
};

} // namespace karma::renderer
