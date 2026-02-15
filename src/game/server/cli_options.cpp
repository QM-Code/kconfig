#include "server/cli_options.hpp"

#include "karma/cli/cli_parse_scaffold.hpp"
#include "karma/cli/server_runtime_options.hpp"
#include "karma/common/logging.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace bz3::server {

namespace {

std::vector<karma::cli::CliRegisteredOption> BuildGameCliOptions(CLIOptions& opts) {
    (void)opts;
    return {};
}

void PrintHelp(const std::string& app_name,
               const std::vector<karma::cli::CliRegisteredOption>& game_options) {
    std::cout
        << "Usage: " << app_name << " [options]\n"
        << "\n"
        << "Options:\n";
    karma::cli::AppendCommonCliHelp(std::cout, false);
    karma::cli::AppendCoreBackendCliHelp(std::cout);
    karma::cli::AppendServerRuntimeCliHelp(std::cout);
    karma::cli::AppendRegisteredCliHelp(std::cout, game_options);
}

[[noreturn]] void Fail(const std::string& message,
                       const std::string& app_name,
                       const std::vector<karma::cli::CliRegisteredOption>& game_options) {
    std::cerr << "Error: " << message << "\n\n";
    PrintHelp(app_name, game_options);
    std::exit(1);
}

} // namespace

CLIOptions ParseCLIOptions(int argc, char** argv) {
    karma::cli::RequireTraceList(argc, argv);

    CLIOptions opts{};
    opts.app_name = karma::cli::ResolveExecutableName(
        (argc > 0 && argv) ? argv[0] : nullptr,
        "bz3-server");
    karma::cli::CliCommonState common{};
    const auto game_options = BuildGameCliOptions(opts);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        const auto common_result = karma::cli::ConsumeCommonCliOption(arg, i, argc, argv, common);
        if (common_result.consumed) {
            if (!common_result.error.empty()) {
                Fail(common_result.error, opts.app_name, game_options);
            }
            if (common_result.help_requested) {
                PrintHelp(opts.app_name, game_options);
                std::exit(0);
            }
            continue;
        }

        const auto physics_result =
            karma::cli::ConsumePhysicsBackendCliOption(arg,
                                                       i,
                                                       argc,
                                                       argv,
                                                       opts.backend_physics,
                                                       opts.backend_physics_explicit);
        if (physics_result.consumed) {
            if (!physics_result.error.empty()) {
                Fail(physics_result.error, opts.app_name, game_options);
            }
            continue;
        }

        const auto audio_result =
            karma::cli::ConsumeAudioBackendCliOption(arg, i, argc, argv, opts.backend_audio, opts.backend_audio_explicit);
        if (audio_result.consumed) {
            if (!audio_result.error.empty()) {
                Fail(audio_result.error, opts.app_name, game_options);
            }
            continue;
        }

        const auto server_runtime_result = karma::cli::ConsumeServerRuntimeCliOption(
            arg,
            i,
            argc,
            argv,
            opts.server_config_path,
            opts.server_config_explicit,
            opts.listen_port,
            opts.listen_port_explicit,
            opts.community,
            opts.community_explicit);
        if (server_runtime_result.consumed) {
            if (!server_runtime_result.error.empty()) {
                Fail(server_runtime_result.error, opts.app_name, game_options);
            }
            continue;
        }

        const auto game_result = karma::cli::ConsumeRegisteredCliOption(arg, i, argc, argv, game_options);
        if (game_result.consumed) {
            if (!game_result.error.empty()) {
                Fail(game_result.error, opts.app_name, game_options);
            }
            continue;
        }

        Fail("Unknown option '" + arg + "'.", opts.app_name, game_options);
    }

    opts.trace_explicit = common.trace_explicit;
    opts.trace_channels = common.trace_channels;
    opts.timestamp_logging = common.timestamp_logging;
    opts.data_dir = common.data_dir;
    opts.data_dir_explicit = common.data_dir_explicit;
    opts.user_config_path = common.user_config_path;
    opts.user_config_explicit = common.user_config_explicit;
    opts.strict_config = common.strict_config;

    if (opts.trace_explicit && opts.trace_channels.empty()) {
        std::cerr << "Error: --trace/-t requires a comma-separated channel list.\n";
        std::cerr << "\nAvailable trace channels:\n"
                  << karma::logging::GetDefaultTraceChannelsHelp();
        std::exit(1);
    }

    return opts;
}

} // namespace bz3::server
