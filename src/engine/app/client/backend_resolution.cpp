#include "karma/app/client/backend_resolution.hpp"

#include "karma/common/config/helpers.hpp"
#include "karma/common/config/store.hpp"
#include "karma/window/window.hpp"
#include "ui/backend.hpp"

#include <algorithm>
#include <stdexcept>

namespace karma::app::client {

renderer::backend::BackendKind ResolveRenderBackendFromOption(const std::string& option_value,
                                                             bool option_explicit) {
    const std::string configured = option_explicit
        ? option_value
        : common::config::ReadStringConfig("render.backend", "auto");
    const auto parsed = renderer::backend::ParseBackendKind(configured);
    if (!parsed) {
        const char* source = option_explicit ? "--backend-render" : "config 'render.backend'";
        throw std::runtime_error(std::string("Invalid value for ") + source + ": '" + configured
                                 + "' (expected: auto|bgfx|diligent)");
    }
    if (*parsed != renderer::backend::BackendKind::Auto) {
        const auto compiled = renderer::backend::CompiledBackends();
        const bool supported = std::any_of(compiled.begin(),
                                           compiled.end(),
                                           [parsed](renderer::backend::BackendKind kind) {
                                               return kind == *parsed;
                                           });
        if (!supported) {
            throw std::runtime_error(
                std::string("Configured render backend '") + configured + "' is not compiled into this binary.");
        }
    }
    return *parsed;
}

std::string CompiledPlatformBackendName() {
    const auto compiled = window::backend::CompiledBackends();
    if (compiled.empty()) {
        return "unknown";
    }
    return window::backend::BackendKindName(compiled.front());
}

void ValidatePlatformBackendFromOption(const std::string& option_value, bool option_explicit) {
    if (!option_explicit) {
        return;
    }

    const auto parsed = window::backend::ParseBackendKind(option_value);
    if (!parsed || *parsed == window::backend::BackendKind::Auto) {
        throw std::runtime_error("Invalid CLI value for --backend-platform: '" + option_value
                                 + "' (expected compiled backend name).");
    }

    const auto compiled = window::backend::CompiledBackends();
    const bool supported = std::any_of(compiled.begin(),
                                       compiled.end(),
                                       [parsed](window::backend::BackendKind kind) {
                                           return kind == *parsed;
                                       });
    if (!supported) {
        throw std::runtime_error("Requested CLI platform backend '" + option_value
                                 + "' is not compiled into this build.");
    }
}

std::string ReadPreferredVideoDriverFromConfig() {
    if (const auto* value = common::config::ConfigStore::Get("platform.VideoDriver")) {
        if (value->is_string()) {
            return value->get<std::string>();
        }
        throw std::runtime_error("Missing required string config: platform.VideoDriver");
    }
    return common::config::ReadRequiredStringConfig("platform.SdlVideoDriver");
}

std::optional<ui::Backend> ResolveUiBackendOverrideFromOption(const std::string& option_value,
                                                              bool option_explicit) {
    if (!option_explicit) {
        return std::nullopt;
    }
    const auto parsed = ui::backend::ParseBackendKind(option_value);
    if (!parsed || *parsed == ui::backend::BackendKind::Auto || *parsed == ui::backend::BackendKind::Software) {
        throw std::runtime_error(std::string("Invalid CLI value for --backend-ui: '")
                                 + option_value + "' (expected: imgui|rmlui)");
    }
    return *parsed == ui::backend::BackendKind::ImGui ? ui::Backend::ImGui : ui::Backend::RmlUi;
}

} // namespace karma::app::client
