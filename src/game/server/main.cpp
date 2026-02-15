#include "server/bootstrap.hpp"
#include "server/cli_options.hpp"
#include "server/runtime.hpp"

#include "karma/app/bootstrap_scaffold.hpp"
#include "karma/cli/cli_parse_scaffold.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <string>

int main(int argc, char** argv) {
    std::string app_name =
        karma::cli::ResolveExecutableName((argc > 0 && argv) ? argv[0] : nullptr, "bz3-server");
    try {
        bz3::server::CLIOptions options = bz3::server::ParseCLIOptions(argc, argv);
        app_name = options.app_name;
        bz3::server::ConfigureLogging(options);
        bz3::server::ConfigureDataAndConfig(options, argc, argv);
        options.app_name = karma::app::ResolveConfiguredAppName(options.app_name);
        app_name = options.app_name;
        return bz3::server::RunRuntime(options);
    } catch (const std::exception& ex) {
        spdlog::error("{}: {}", app_name, ex.what());
        return 1;
    }
}
