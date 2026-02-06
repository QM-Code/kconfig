#include "server/runtime.hpp"
#include "server/server_game.hpp"

#include "karma/app/engine_server_app.hpp"
#include "karma/common/config_helpers.hpp"
#include "karma/common/config_store.hpp"
#include "karma/common/config_validation.hpp"
#include "karma/common/data_path_resolver.hpp"
#include "karma/common/logging.hpp"

#include <spdlog/spdlog.h>

#include <atomic>
#include <csignal>
#include <filesystem>
#include <string>

namespace bz3::server {

namespace {

std::atomic<bool> g_running{true};

void OnSignal(int signum) {
    KARMA_TRACE("engine.server", "bz3-server: received signal {}, requesting stop", signum);
    g_running.store(false);
}

std::string ResolveWorldDirectory(const CLIOptions& options) {
    if (options.world_specified) {
        return options.world_dir;
    }
    if (options.use_default_world) {
        return karma::config::ReadStringConfig("defaultWorld", "");
    }
    return karma::config::ReadStringConfig("defaultWorld", "");
}

} // namespace

int RunRuntime(const CLIOptions& options) {
    const auto issues = karma::config::ValidateRequiredKeys(karma::config::ServerRequiredKeys());
    if (!issues.empty()) {
        for (const auto& issue : issues) {
            if (options.strict_config) {
                spdlog::error("config validation: {}: {}", issue.path, issue.message);
            } else {
                spdlog::warn("config validation: {}: {}", issue.path, issue.message);
            }
        }
        if (options.strict_config) {
            return 1;
        }
    }

    const std::string worldDir = ResolveWorldDirectory(options);
    if (worldDir.empty()) {
        spdlog::error("bz3-server: no world directory specified and 'defaultWorld' is missing.");
        return 1;
    }

    const std::filesystem::path worldDirPath = karma::data::Resolve(worldDir);
    if (!std::filesystem::is_directory(worldDirPath)) {
        spdlog::error("bz3-server: world directory not found: {}", worldDirPath.string());
        return 1;
    }

    const std::filesystem::path worldConfigPath = worldDirPath / "config.json";
    const auto worldConfigOpt =
        karma::data::LoadJsonFile(worldConfigPath, "world config", spdlog::level::err);
    if (!worldConfigOpt || !worldConfigOpt->is_object()) {
        spdlog::error("bz3-server: failed to load world config object from {}", worldConfigPath.string());
        return 1;
    }

    if (!karma::config::ConfigStore::AddRuntimeLayer("world config", *worldConfigOpt, worldConfigPath.parent_path())) {
        spdlog::error("bz3-server: failed to add world config runtime layer.");
        return 1;
    }

    const auto* layer = karma::config::ConfigStore::LayerByLabel("world config");
    if (!layer || !layer->is_object()) {
        spdlog::error("bz3-server: runtime layer lookup failed for world config.");
        return 1;
    }

    const std::string worldName =
        karma::config::ReadStringConfig("worldName", worldDirPath.filename().string());
    KARMA_TRACE("engine.server",
                "bz3-server: world '{}' loaded from '{}'",
                worldName,
                worldDirPath.string());
    if (options.host_port_explicit) {
        KARMA_TRACE("engine.server",
                    "CLI option --port parsed (not wired yet): {}",
                    options.host_port);
    }
    if (options.community_explicit) {
        KARMA_TRACE("engine.server",
                    "CLI option --community parsed (not wired yet): '{}'",
                    options.community);
    }

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    g_running.store(true);
    karma::app::EngineServerConfig engineConfig{};
    ServerGame game{worldName};
    karma::app::EngineServerApp app{};
    app.start(game, engineConfig);

    while (app.isRunning()) {
        app.tick();
        if (!g_running.load()) {
            app.requestStop();
        }
    }

    return 0;
}

} // namespace bz3::server
