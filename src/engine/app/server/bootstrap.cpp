#include "karma/app/server_bootstrap_runner.hpp"

#include "karma/app/bootstrap_scaffold.hpp"
#include "karma/cli/server_runtime_options.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace karma::app {

void RunServerBootstrap(const karma::cli::ServerAppOptions& options,
                        int argc,
                        char** argv,
                        std::string_view app_name) {
    ConfigureLoggingFromOptions(options.timestamp_logging,
                                options.trace_explicit,
                                options.trace_channels);

    BootstrapConfigSpec spec{};
    spec.app_name = app_name.empty() ? std::string("app") : std::string(app_name);
    spec.data_dir_env_var = "BZ3_DATA_DIR";
    spec.required_data_marker = "server/config.json";
    spec.enable_user_config = false;
    spec.allow_user_config_data_dir_when_user_config_disabled = true;
    spec.config_specs = {
        {"server/config.json", "data/server/config.json", spdlog::level::err, true, true}
    };
    ConfigureDataAndConfigFromSpec(spec, argc, argv);

    (void)karma::cli::ApplyServerConfigOverlay(options.server_config_path, options.server_config_explicit);
}

} // namespace karma::app
