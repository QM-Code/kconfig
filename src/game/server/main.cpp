#include "server/bootstrap.hpp"
#include "server/cli_options.hpp"
#include "server/runtime.hpp"

#include <spdlog/spdlog.h>

#include <exception>

int main(int argc, char** argv) {
    try {
        const bz3::server::CLIOptions options = bz3::server::ParseCLIOptions(argc, argv);
        bz3::server::ConfigureLogging(options);
        bz3::server::ConfigureDataAndConfig(argc, argv);
        return bz3::server::RunRuntime(options);
    } catch (const std::exception& ex) {
        spdlog::error("bz3-server: {}", ex.what());
        return 1;
    }
}
