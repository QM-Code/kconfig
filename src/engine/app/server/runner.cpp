#include "karma/app/server_runner.hpp"

#include "karma/app/bootstrap_scaffold.hpp"
#include "karma/app/server_bootstrap_runner.hpp"
#include "karma/cli/cli_parse_scaffold.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace karma::app {

int RunServer(int argc,
              char** argv,
              const ServerRunSpec& spec,
              const ServerRuntimeHook& runtime_hook) {
    const std::string parse_app_name =
        spec.parse_app_name.empty() ? std::string("app") : std::string(spec.parse_app_name);
    const std::string bootstrap_app_name = spec.bootstrap_app_name.empty()
        ? parse_app_name
        : std::string(spec.bootstrap_app_name);

    std::string app_name =
        karma::cli::ResolveExecutableName((argc > 0 && argv) ? argv[0] : nullptr, parse_app_name);
    try {
        karma::cli::ServerAppOptions options =
            karma::cli::ParseServerAppCliOptions(argc, argv, parse_app_name);
        app_name = options.app_name;
        RunServerBootstrap(options, argc, argv, bootstrap_app_name);
        options.app_name = ResolveConfiguredAppName(options.app_name);
        app_name = options.app_name;
        if (!runtime_hook) {
            throw std::runtime_error("Server runtime hook was not provided.");
        }
        return runtime_hook(options);
    } catch (const std::exception& ex) {
        spdlog::error("{}: {}", app_name, ex.what());
        return 1;
    }
}

} // namespace karma::app
