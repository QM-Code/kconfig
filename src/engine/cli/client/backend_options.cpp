#include "karma/cli/client/backend_options.hpp"

#include "karma/renderer/backend.hpp"

#include <optional>
#include <vector>

namespace karma::cli::client {
namespace {

std::vector<std::string> RenderBackendChoices(bool include_auto) {
    std::vector<std::string> choices{};
    const auto compiled = renderer_backend::CompiledBackends();
    if (include_auto && compiled.size() > 1) {
        choices.emplace_back("auto");
    }
    for (const auto backend : compiled) {
        choices.emplace_back(renderer_backend::BackendKindName(backend));
    }
    return choices;
}

std::vector<std::string> UiBackendChoices() {
    std::vector<std::string> choices{};
#if defined(KARMA_HAS_IMGUI)
    choices.emplace_back("imgui");
#endif
#if defined(KARMA_HAS_RMLUI)
    choices.emplace_back("rmlui");
#endif
    return choices;
}

std::vector<std::string> PlatformBackendChoices() {
    std::vector<std::string> choices{};
#if defined(KARMA_WINDOW_BACKEND_SDL3)
    choices.emplace_back("sdl3");
#endif
#if defined(KARMA_WINDOW_BACKEND_SDL2)
    choices.emplace_back("sdl2");
#endif
#if defined(KARMA_WINDOW_BACKEND_GLFW)
    choices.emplace_back("glfw");
#endif
    return choices;
}

bool ShouldExposeRenderBackendOption() {
    return renderer_backend::CompiledBackends().size() > 1;
}

bool ShouldExposeUiBackendOption() {
    return UiBackendChoices().size() > 1;
}

bool ShouldExposePlatformBackendOption() {
    return PlatformBackendChoices().size() > 1;
}

} // namespace

shared::ConsumeResult ConsumeRenderBackendOption(const std::string& arg,
                                                 int& index,
                                                 int argc,
                                                 char** argv,
                                                 std::string& backend_out,
                                                 bool& explicit_out) {
    shared::ConsumeResult out{};
    if (!ShouldExposeRenderBackendOption()) {
        return out;
    }

    std::optional<std::string> raw_value{};
    if (arg == "--backend-render") {
        std::string error{};
        raw_value = shared::RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!raw_value) {
            out.error = error;
            return out;
        }
    } else if (shared::StartsWith(arg, "--backend-render=")) {
        raw_value = shared::ValueAfterEquals(arg, "--backend-render=");
        out.consumed = true;
    } else {
        return out;
    }

    const auto choices = RenderBackendChoices(true);
    const auto parsed = shared::ParseChoiceLower(*raw_value, choices);
    if (!parsed) {
        out.error = "Invalid value '" + *raw_value + "' for --backend-render. Expected: "
            + shared::JoinChoices(choices) + ".";
        return out;
    }
    backend_out = *parsed;
    explicit_out = true;
    return out;
}

shared::ConsumeResult ConsumeUiBackendOption(const std::string& arg,
                                             int& index,
                                             int argc,
                                             char** argv,
                                             std::string& backend_out,
                                             bool& explicit_out) {
    shared::ConsumeResult out{};
    if (!ShouldExposeUiBackendOption()) {
        return out;
    }

    std::optional<std::string> raw_value{};
    if (arg == "--backend-ui") {
        std::string error{};
        raw_value = shared::RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!raw_value) {
            out.error = error;
            return out;
        }
    } else if (shared::StartsWith(arg, "--backend-ui=")) {
        raw_value = shared::ValueAfterEquals(arg, "--backend-ui=");
        out.consumed = true;
    } else {
        return out;
    }

    const auto choices = UiBackendChoices();
    const auto parsed = shared::ParseChoiceLower(*raw_value, choices);
    if (!parsed) {
        out.error = "Invalid value '" + *raw_value + "' for --backend-ui. Expected: "
            + shared::JoinChoices(choices) + ".";
        return out;
    }
    backend_out = *parsed;
    explicit_out = true;
    return out;
}

shared::ConsumeResult ConsumePlatformBackendOption(const std::string& arg,
                                                   int& index,
                                                   int argc,
                                                   char** argv,
                                                   std::string& backend_out,
                                                   bool& explicit_out) {
    shared::ConsumeResult out{};
    if (!ShouldExposePlatformBackendOption()) {
        return out;
    }

    std::optional<std::string> raw_value{};
    if (arg == "--backend-platform") {
        std::string error{};
        raw_value = shared::RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!raw_value) {
            out.error = error;
            return out;
        }
    } else if (shared::StartsWith(arg, "--backend-platform=")) {
        raw_value = shared::ValueAfterEquals(arg, "--backend-platform=");
        out.consumed = true;
    } else {
        return out;
    }

    const auto choices = PlatformBackendChoices();
    const auto parsed = shared::ParseChoiceLower(*raw_value, choices);
    if (!parsed) {
        out.error = "Invalid value '" + *raw_value + "' for --backend-platform. Expected: "
            + shared::JoinChoices(choices) + ".";
        return out;
    }
    backend_out = *parsed;
    explicit_out = true;
    return out;
}

void AppendBackendHelp(std::ostream& out) {
    if (ShouldExposeRenderBackendOption()) {
        const auto choices = RenderBackendChoices(true);
        out << "      --backend-render <name>     Render backend override ("
            << shared::JoinChoices(choices) << ")\n";
    }
    if (ShouldExposeUiBackendOption()) {
        const auto choices = UiBackendChoices();
        out << "      --backend-ui <name>         UI backend override ("
            << shared::JoinChoices(choices) << ")\n";
    }
    if (ShouldExposePlatformBackendOption()) {
        const auto choices = PlatformBackendChoices();
        out << "      --backend-platform <name>   Platform backend override ("
            << shared::JoinChoices(choices) << ")\n";
    }
}

} // namespace karma::cli::client
