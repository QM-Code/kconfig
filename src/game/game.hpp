#pragma once

#include "karma/app/game_interface.hpp"

namespace bz3 {

class Game final : public karma::app::GameInterface {
 public:
    Game() = default;
    void onStart() override;
    void onUpdate(float dt) override;
    void onShutdown() override;
};

} // namespace bz3
