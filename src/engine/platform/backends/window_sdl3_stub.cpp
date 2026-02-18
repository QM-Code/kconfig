#include "platform/backends/factory_internal.hpp"

#if !defined(KARMA_WINDOW_BACKEND_SDL3)
namespace karma::platform::backend {

std::unique_ptr<Window> CreateSdl3WindowBackend(const WindowConfig& config) {
    (void)config;
    return nullptr;
}

} // namespace karma::platform::backend
#endif
