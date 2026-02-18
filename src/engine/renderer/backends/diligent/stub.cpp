#include "renderer/backends/factory_internal.hpp"

#if !defined(KARMA_HAS_RENDER_DILIGENT)
namespace karma::renderer::backend {

std::unique_ptr<Backend> CreateDiligentBackend(karma::window::Window& window) {
    (void)window;
    return nullptr;
}

} // namespace karma::renderer::backend
#endif
