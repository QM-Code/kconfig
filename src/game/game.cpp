#include "game.hpp"

#include "karma/input/input_system.hpp"

namespace bz3 {

void Game::onStart() {
    if (input) {
        input->setMode(karma::input::InputMode::Roaming);
    }
}

void Game::onUpdate(float dt) {
    (void)dt;
}

void Game::onShutdown() {}

} // namespace bz3
