#include "karma/app/client/runner.hpp"

#include "karma/app/client/bootstrap.hpp"
#include "karma/app/shared/bootstrap.hpp"
#include "karma/cli/cli_parse_scaffold.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace karma::app::client {

int Run(int argc,
        char** argv,
        const RunSpec& spec,
        const RuntimeHook& runtime_hook) {
    const std::string parse_app_name =
        spec.parse_app_name.empty() ? std::string("app") : std::string(spec.parse_app_name);
    const std::string bootstrap_app_name = spec.bootstrap_app_name.empty()
        ? parse_app_name
        : std::string(spec.bootstrap_app_name);

    std::string app_name =
        karma::cli::ResolveExecutableName((argc > 0 && argv) ? argv[0] : nullptr, parse_app_name);
    try {
        karma::cli::ClientAppOptions options =
            karma::cli::ParseClientAppCliOptions(argc, argv, parse_app_name);
        app_name = options.app_name;
        RunBootstrap(options, argc, argv, bootstrap_app_name);
        options.app_name = shared::ResolveConfiguredAppName(options.app_name);
        app_name = options.app_name;
        if (!runtime_hook) {
            throw std::runtime_error("Client runtime hook was not provided.");
        }
        return runtime_hook(options);
    } catch (const std::exception& ex) {
        spdlog::error("{}: {}", app_name, ex.what());
        return 1;
    }
}

} // namespace karma::app::client
