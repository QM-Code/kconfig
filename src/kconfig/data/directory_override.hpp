#pragma once

#include <filesystem>
#include <optional>

namespace kconfig::data::directory_override {

struct DataDirectoryOverrideResult {
    std::filesystem::path userConfigPath;
    std::optional<std::filesystem::path> dataDir;
};

DataDirectoryOverrideResult ApplyDataDirectoryOverride(
    const std::optional<std::filesystem::path>& cli_user_config_path,
    const std::optional<std::filesystem::path>& cli_data_dir,
    const std::filesystem::path& defaultConfigRelative = std::filesystem::path("config.json"),
    bool enableUserConfig = true,
    bool allowDataDirFromUserConfigWhenUserConfigDisabled = false);

} // namespace kconfig::data::directory_override
