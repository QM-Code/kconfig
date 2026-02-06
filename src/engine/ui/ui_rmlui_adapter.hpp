#pragma once

#include "ui/ui_backend.hpp"

#include <memory>
#include <vector>

namespace karma::ui {

class UiRmlUiAdapter {
 public:
    virtual ~UiRmlUiAdapter() = default;
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame(float dt, const std::vector<platform::Event>& events) = 0;
    virtual void build(const std::vector<UiDrawContext::RmlUiDrawCallback>& draw_callbacks,
                       const std::vector<UiDrawContext::TextPanel>& text_panels,
                       UiOverlayFrame& out) = 0;
};

std::unique_ptr<UiRmlUiAdapter> CreateRmlUiAdapter();

} // namespace karma::ui
