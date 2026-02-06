#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace karma::ui {

enum class UiBackendKind {
    None,
    ImGui,
    RmlUi
};

class UiDrawContext {
 public:
    using ImGuiDrawCallback = std::function<void()>;
    using RmlUiDrawCallback = std::function<void()>;

    struct TextPanel {
        std::string title;
        std::vector<std::string> lines;
        float x = 20.0f;
        float y = 20.0f;
        float bg_alpha = 0.78f;
        bool auto_size = true;
    };

    virtual ~UiDrawContext() = default;
    virtual UiBackendKind backendKind() const = 0;
    virtual void addImGuiDraw(ImGuiDrawCallback callback) = 0;
    virtual void addRmlUiDraw(RmlUiDrawCallback callback) = 0;
    virtual void addTextPanel(TextPanel panel) = 0;
    virtual size_t imguiDrawCount() const = 0;
    virtual size_t rmluiDrawCount() const = 0;
    virtual size_t textPanelCount() const = 0;
};

} // namespace karma::ui
