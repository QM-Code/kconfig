#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "kconfig/json.hpp"
#include <spdlog/spdlog.h>

namespace kconfig::common::config {

struct ConfigFileSpec {
    std::filesystem::path path;
    std::string label;
    spdlog::level::level_enum missingLevel = spdlog::level::warn;
    bool required = false;
    bool resolveRelativeToDataRoot = true;
};

class ConfigStore {
public:
    static bool Add(std::string_view name,
                    const kconfig::common::serialization::Value &json,
                    bool isMutable = true);
    static bool Merge(std::string_view targetName, const std::vector<std::string> &sourceNames);
    static bool Remove(std::string_view name);
    static std::optional<kconfig::common::serialization::Value> Get(std::string_view name, std::string_view path);

    static bool Set(std::string_view name, std::string_view path, kconfig::common::serialization::Value value);
    static bool Erase(std::string_view name, std::string_view path);
    static bool SetAssetRoot(std::string_view name, const std::filesystem::path &fullFilesystemPath);
    static bool SetBackingFile(std::string_view name, const std::filesystem::path &fullFilesystemPath);
    static bool RemoveBackingFile(std::string_view name);
    static const std::filesystem::path *Path(std::string_view name);
    static bool Save(std::string_view name, std::string *error = nullptr);

    static void Initialize(const std::vector<ConfigFileSpec> &defaultSpecs,
                           const std::optional<std::filesystem::path> &userConfigPath = std::nullopt,
                           const std::vector<ConfigFileSpec> &runtimeSpecs = {});
    static bool Initialized();
    static const kconfig::common::serialization::Value *Get(std::string_view path);
    static std::optional<kconfig::common::serialization::Value> GetCopy(std::string_view path);
    static bool Set(std::string_view path, kconfig::common::serialization::Value value);
    static bool Erase(std::string_view path);

    static bool AddRuntimeLayer(const std::string &label,
                                const kconfig::common::serialization::Value &layerJson,
                                const std::filesystem::path &baseDir);
    static const kconfig::common::serialization::Value *LayerByLabel(const std::string &label);

    static std::filesystem::path ResolveAssetPath(const std::string &assetKey,
                                                  const std::filesystem::path &defaultPath = {});
};

} // namespace kconfig::common::config
