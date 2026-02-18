#include "client/runtime.hpp"

#include "client/runtime/internal.hpp"
#include "game.hpp"

#include "karma/app/client/engine.hpp"

namespace bz3::client {

int RunRuntime(const karma::cli::ClientAppOptions& options) {
    const karma::app::client::EngineConfig config = runtime_detail::BuildEngineConfig(options);
    const bz3::GameStartupOptions startup = runtime_detail::ResolveGameStartupOptions(options);

    karma::app::client::Engine app;
    bz3::Game game{startup};
    app.start(game, config);
    while (app.isRunning()) {
        app.tick();
    }
    return 0;
}

} // namespace bz3::client
