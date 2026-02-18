#include "renderer/backends/factory_internal.hpp"

#if !defined(KARMA_HAS_RENDER_BGFX)
namespace karma::renderer::backend {

std::unique_ptr<Backend> CreateBgfxBackend(karma::window::Window& window) {
    (void)window;
    return nullptr;
}

} // namespace karma::renderer::backend
#endif
