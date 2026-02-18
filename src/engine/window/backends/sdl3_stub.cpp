#include "window/backends/factory_internal.hpp"

#if !defined(KARMA_WINDOW_BACKEND_SDL3)
namespace karma::window::backend {

std::unique_ptr<Window> CreateSdl3WindowBackend(const WindowConfig& config) {
    (void)config;
    return nullptr;
}

} // namespace karma::window::backend
#endif
