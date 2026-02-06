#include "server/bootstrap.hpp"

#include "karma/common/config_store.hpp"
#include "karma/common/data_dir_override.hpp"
#include "karma/common/data_path_resolver.hpp"
#include "karma/common/logging.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <vector>

namespace bz3::server {

void ConfigureLogging(const CLIOptions& options) {
    karma::logging::ConfigureLogPatterns(options.timestamp_logging);
    spdlog::set_level(options.verbose ? spdlog::level::debug : spdlog::level::info);
    if (options.trace_explicit) {
        karma::logging::EnableTraceChannels(options.trace_channels);
    }
}

void ConfigureDataAndConfig(int argc, char** argv) {
    karma::data::DataPathSpec spec;
    spec.appName = "bz3";
    spec.dataDirEnvVar = "BZ3_DATA_DIR";
    spec.requiredDataMarker = "common/config.json";
    karma::data::SetDataPathSpec(spec);

    const auto dataDirResult =
        karma::data::ApplyDataDirOverrideFromArgs(argc, argv, std::filesystem::path("server/config.json"));
    const std::vector<karma::config::ConfigFileSpec> configSpecs = {
        {"common/config.json", "data/common/config.json", spdlog::level::err, true, true},
        {"server/config.json", "data/server/config.json", spdlog::level::err, true, true}
    };
    karma::config::ConfigStore::Initialize(configSpecs, dataDirResult.userConfigPath);
}

} // namespace bz3::server
