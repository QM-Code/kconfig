#include <kconfig/data/directory_override.hpp>

#include "data/path_utils.hpp"
#include "data/path_resolver.hpp"
#include <kconfig/json.hpp>
#include <ktrace/trace.hpp>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <stdexcept>

namespace {

struct DataDirFromConfigResult {
    std::optional<std::filesystem::path> path{};
    std::string reason{"not evaluated"};
};

std::filesystem::path EnsureConfigFileAtPath(const std::filesystem::path &path, const std::filesystem::path &defaultRelative) {
    const auto target = path.empty()
        ? kconfig::data::path_resolver::UserConfigDirectory() / defaultRelative
        : path;

    std::string error;
    if (!kconfig::data::path_utils::EnsureJsonObjectFile(target, &error)) {
        throw std::runtime_error("Failed to prepare user config file " + target.string() + ": " + error);
    }

    return kconfig::data::path_utils::Canonicalize(target);
}

DataDirFromConfigResult ExtractDataDirFromConfig(const std::filesystem::path &configPath) {
    const auto readResult = kconfig::data::path_utils::ReadJsonFile(configPath);
    if (!readResult.json.has_value()) {
        // If the file cannot be opened/read, fall back to other mechanisms.
        if (readResult.error == kconfig::data::path_utils::JsonReadError::ParseFailed) {
            throw std::runtime_error("Failed to parse user config at " + configPath.string()
                                     + ": "
                                     + (readResult.message.empty() ? std::string("invalid JSON") : readResult.message));
        }
        return {std::nullopt, "config file not readable"};
    }

    try {
        const auto &configJson = *readResult.json;
        if (!configJson.is_object()) {
            return {std::nullopt, "config is not a JSON object"};
        }

        const auto dataDirIt = configJson.find("DataDir");
        if (dataDirIt == configJson.end()) {
            return {std::nullopt, "DataDir key missing"};
        }
        if (!dataDirIt->is_string()) {
            return {std::nullopt, "DataDir is not a string"};
        }

        const auto value = dataDirIt->get<std::string>();
        if (value.empty()) {
            return {std::nullopt, "DataDir is empty"};
        }

        std::filesystem::path dataDirPath(value);
        if (dataDirPath.is_relative()) {
            dataDirPath = configPath.parent_path() / dataDirPath;
        }
        return {kconfig::data::path_utils::Canonicalize(dataDirPath), "DataDir resolved from config"};
    } catch (const std::exception &ex) {
        throw std::runtime_error("Failed to parse user config at " + configPath.string() + ": " + ex.what());
    }
}

bool IsValidDir(const std::filesystem::path &path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

void ValidateDataDirOrExit(const std::filesystem::path &path, const std::string &sourceLabel, const std::optional<std::filesystem::path> &configPath = std::nullopt) {
    if (!IsValidDir(path)) {
        KTRACE("cli", "rejected data directory from {} -> '{}' (path missing or not a directory)",
                    sourceLabel,
                    path.string());
        std::cerr << "Invalid data directory specified: \"" << sourceLabel << "\"\n";
        std::cerr << "" << path.string() << " does not exist or is not a directory.\n";
        if (configPath) {
            std::cerr << "User config path: " << configPath->string() << "\n";
        }
        std::exit(1);
    }

    std::error_code ec;
    const auto spec = kconfig::data::path_resolver::GetDataPathSpec();
    if (!spec.requiredDataMarker.empty()) {
        const auto markerPath = path / spec.requiredDataMarker;
        if (!std::filesystem::exists(markerPath, ec) || !std::filesystem::is_regular_file(markerPath, ec)) {
            KTRACE("cli", "rejected data directory from {} -> '{}' (missing required marker '{}')",
                        sourceLabel,
                        path.string(),
                        markerPath.string());
            std::cerr << "Invalid data directory specified: \"" << sourceLabel << "\"\n";
            std::cerr << markerPath.string() << " does not exist." << std::endl;
            if (configPath) {
                std::cerr << "User config path: " << configPath->string() << std::endl;
            }
            std::exit(1);
        }
    }
}

} // namespace

namespace kconfig::data::directory_override {

DataDirectoryOverrideResult ApplyDataDirectoryOverride(
    const std::optional<std::filesystem::path>& cli_user_config_path,
    const std::optional<std::filesystem::path>& cli_data_dir,
    const std::filesystem::path& defaultConfigRelative,
    bool enableUserConfig,
    bool allowDataDirFromUserConfigWhenUserConfigDisabled) {
    try {
        std::filesystem::path configPath{};
        std::optional<std::filesystem::path> configDataDir{};
        std::string configDataDirReason = "not evaluated";

        KTRACE("cli", "begin resolution (enable_user_config={}, allow_user_config_data_dir_when_disabled={}, default_config_relative='{}')",
                    enableUserConfig,
                    allowDataDirFromUserConfigWhenUserConfigDisabled,
                    defaultConfigRelative.string());
        KTRACE("cli", "cli inputs (--user-config={}, --data={})",
                    cli_user_config_path ? cli_user_config_path->string() : std::string("<unset>"),
                    cli_data_dir ? cli_data_dir->string() : std::string("<unset>"));

        if (enableUserConfig) {
            configPath = EnsureConfigFileAtPath(cli_user_config_path ? *cli_user_config_path : std::filesystem::path{},
                                                defaultConfigRelative);
            KTRACE("cli", "resolved writable user config path '{}'", configPath.string());
            const auto configRoot = configPath.parent_path();
            if (!configRoot.empty()) {
                kconfig::data::path_resolver::SetUserConfigRootOverride(configRoot);
                KTRACE("cli", "user config root override '{}'", configRoot.string());
            }
            if (cli_data_dir) {
                configDataDirReason = "skipped user-config DataDir because --data was provided";
            } else {
                const DataDirFromConfigResult extracted = ExtractDataDirFromConfig(configPath);
                configDataDir = extracted.path;
                configDataDirReason = extracted.reason;
                KTRACE("cli", "user config DataDir probe at '{}' -> {} ({})",
                            configPath.string(),
                            configDataDir ? configDataDir->string() : std::string("<none>"),
                            configDataDirReason);
            }
        } else if (cli_user_config_path) {
            KTRACE("cli", "--user-config provided but this executable has user-config disabled");
            std::cerr << "The --user-config option is not supported for this executable.\n";
            std::exit(1);
        } else if (allowDataDirFromUserConfigWhenUserConfigDisabled) {
            const auto readOnlyConfigPath = kconfig::data::path_utils::Canonicalize(kconfig::data::path_resolver::UserConfigDirectory() / defaultConfigRelative);
            configPath = readOnlyConfigPath;
            KTRACE("cli", "using read-only user config probe path '{}'",
                        readOnlyConfigPath.string());
            if (cli_data_dir) {
                configDataDirReason = "skipped user-config DataDir because --data was provided";
            } else {
                const DataDirFromConfigResult extracted = ExtractDataDirFromConfig(readOnlyConfigPath);
                configDataDir = extracted.path;
                configDataDirReason = extracted.reason;
                KTRACE("cli", "read-only config DataDir probe at '{}' -> {} ({})",
                            readOnlyConfigPath.string(),
                            configDataDir ? configDataDir->string() : std::string("<none>"),
                            configDataDirReason);
            }
        }

        if (cli_data_dir) {
            KTRACE("cli", "selecting --data source '{}'", cli_data_dir->string());
            ValidateDataDirOrExit(*cli_data_dir, std::string("--data ") + cli_data_dir->string());
            kconfig::data::path_resolver::SetDataRootOverride(*cli_data_dir);
            KTRACE("cli", "accepted --data source '{}'", cli_data_dir->string());
            KTRACE("config", "Using data directory from CLI override: {}", cli_data_dir->string());
            return {configPath, *cli_data_dir};
        }

        if (configDataDir) {
            KTRACE("cli", "selecting config DataDir source '{}'", configDataDir->string());
            ValidateDataDirOrExit(*configDataDir, std::string("user config"), configPath);
            kconfig::data::path_resolver::SetDataRootOverride(*configDataDir);
            KTRACE("cli", "accepted config DataDir source '{}'", configDataDir->string());
            KTRACE("config", "Using data directory from user config: {}", configDataDir->string());
            return {configPath, *configDataDir};
        }

        const auto spec = kconfig::data::path_resolver::GetDataPathSpec();
        const char *envDataDir = std::getenv(spec.dataDirEnvVar.c_str());

        // Fall back to env var if present; otherwise fail with a friendly message.
        if (envDataDir && *envDataDir) {
            const std::filesystem::path envPath(envDataDir);
            KTRACE("cli", "selecting env source {}='{}'",
                        spec.dataDirEnvVar,
                        envPath.string());
            ValidateDataDirOrExit(envPath, std::string(spec.dataDirEnvVar) + ": " + envDataDir);
            kconfig::data::path_resolver::SetDataRootOverride(envPath);
            KTRACE("cli", "accepted env source {}='{}'", spec.dataDirEnvVar, envPath.string());
            KTRACE("config", "Using data directory from {}: {}", spec.dataDirEnvVar, envPath.string());
            return {configPath, envPath};
        }

        KTRACE("cli", "no usable source (--data=<unset>, config_data_dir={}, config_probe_reason='{}', env {}={})",
                    configDataDir ? configDataDir->string() : std::string("<none>"),
                    configDataDirReason,
                    spec.dataDirEnvVar,
                    (envDataDir && *envDataDir) ? std::string(envDataDir) : std::string("<unset>"));
        if (!configPath.empty()) {
            KTRACE("cli", "probed config path '{}'", configPath.string());
        }

        std::cerr << "\n";
        std::cerr << "The data directory could not be found.\n";
        std::cerr << "\n";
        std::cerr << "This should not happen and may indicate a problem with installation.\n\n";
        std::cerr << "This directory can be specified";
        if (enableUserConfig || allowDataDirFromUserConfigWhenUserConfigDisabled) {
            std::cerr << " in three ways:\n";
        } else {
            std::cerr << " in two ways:\n";
        }
        std::cerr << "  1. Set the " << spec.dataDirEnvVar << " environment variable.\n";
        std::cerr << "  2. Use the command-line option \"--data <datadir>\".\n";
        if (enableUserConfig || allowDataDirFromUserConfigWhenUserConfigDisabled) {
            std::cerr << "  3. Add the following to your user config file:\n";
            std::cerr << "     " << configPath.string() << "\n";
            std::cerr << "     {\n";
            std::cerr << "         \"DataDir\" : \"<datadir>\"\n";
            std::cerr << "     }\n";
        }
        std::cerr << "\n";
        std::exit(1);
    } catch (const std::exception &ex) {
        KTRACE("cli", "fatal exception during resolution: {}", ex.what());
        std::cerr << ex.what() << std::endl;
        std::exit(1);
    }
}

} // namespace kconfig::data::directory_override
