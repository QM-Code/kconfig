#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <kconfig/json.hpp>
#include <spdlog/spdlog.h>

namespace kconfig::common::data {

std::filesystem::path Resolve(const std::filesystem::path& relativePath);
void SetDataRootOverride(const std::filesystem::path& path);
void SetUserConfigRootOverride(const std::filesystem::path& path);

std::optional<kconfig::common::serialization::Value> LoadJsonFile(const std::filesystem::path& path,
                                                                  const std::string& label,
                                                                  spdlog::level::level_enum missingLevel);

std::filesystem::path UserConfigDirectory();
std::filesystem::path EnsureUserWorldsDirectory();
std::filesystem::path EnsureUserWorldDirectoryForServer(const std::string& host, uint16_t port);

struct ConfigLayerSpec {
    std::filesystem::path relativePath;
    std::string label;
    spdlog::level::level_enum missingLevel = spdlog::level::warn;
    bool required = false;
};

struct ConfigLayer {
    kconfig::common::serialization::Value json;
    std::filesystem::path baseDir;
    std::string label;
};

std::vector<ConfigLayer> LoadConfigLayers(const std::vector<ConfigLayerSpec>& specs);
void MergeJsonObjects(kconfig::common::serialization::Value& destination, const kconfig::common::serialization::Value& source);
void CollectAssetEntries(const kconfig::common::serialization::Value& node,
                         const std::filesystem::path& baseDir,
                         std::map<std::string, std::filesystem::path>& assetMap,
                         const std::string& prefix = "");

struct DataPathSpec {
    std::string appName = "app";
    std::string dataDirEnvVar = "DATA_DIR";
    std::filesystem::path requiredDataMarker;
    std::vector<ConfigLayerSpec> fallbackAssetLayers;
};

void SetDataPathSpec(DataPathSpec spec);

void RegisterPackageMount(const std::string& id,
                          const std::filesystem::path& packagePath,
                          const std::filesystem::path& mountPoint = {});
void ClearPackageMounts();

std::filesystem::path ExecutableDirectory();
std::filesystem::path ResolveConfiguredAsset(const std::string& assetKey,
                                             const std::filesystem::path& defaultRelativePath = {});
const std::filesystem::path& DataRoot();

} // namespace kconfig::common::data
