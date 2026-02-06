#include "ui/ui_backend.hpp"

#include "ui/ui_rmlui_adapter.hpp"

#include <memory>

namespace karma::ui {
namespace {

class RmlUiBackend final : public UiBackend {
 public:
    RmlUiBackend() : adapter_(CreateRmlUiAdapter()) {}

    const char* name() const override {
        return "rmlui";
    }

    bool init() override {
        return adapter_ ? adapter_->init() : false;
    }

    void shutdown() override {
        if (adapter_) {
            adapter_->shutdown();
        }
    }

    void beginFrame(float dt, const std::vector<platform::Event>& events) override {
        if (adapter_) {
            adapter_->beginFrame(dt, events);
        }
    }

    void build(const std::vector<UiDrawContext::ImGuiDrawCallback>& imgui_draw_callbacks,
               const std::vector<UiDrawContext::RmlUiDrawCallback>& rmlui_draw_callbacks,
               const std::vector<UiDrawContext::TextPanel>& text_panels,
               UiOverlayFrame& out) override {
        (void)imgui_draw_callbacks;
        if (adapter_) {
            adapter_->build(rmlui_draw_callbacks, text_panels, out);
        }
    }

 private:
    std::unique_ptr<UiRmlUiAdapter> adapter_{};
};

} // namespace

std::unique_ptr<UiBackend> CreateRmlUiBackend() {
    return std::make_unique<RmlUiBackend>();
}

} // namespace karma::ui
