#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <kconfig/json.hpp>
#include <spdlog/spdlog.h>

namespace kconfig::common::config {

struct ConfigFileSpec {
    std::filesystem::path path;
    std::string label;
    spdlog::level::level_enum missingLevel = spdlog::level::warn;
    bool required = false;
    bool resolveRelativeToDataRoot = true;
};

struct ConfigLayer {
    kconfig::common::serialization::Value json;
    std::filesystem::path baseDir;
    std::string label;
    bool isMutable = true;
};

class ConfigStore {
public:
    // New named config API.
    static bool Add(std::string_view name,
                    const kconfig::common::serialization::Value &json,
                    bool isMutable = true);
    static bool Load(std::string_view name, const ConfigFileSpec &spec, bool isMutable = true);
    static bool Merge(std::string_view targetName, const std::vector<std::string> &sourceNames);
    static bool Remove(std::string_view name);

    // Returns a copy from the named config entry.
    // Pass "" or "." for `path` to read the full root object.
    static std::optional<kconfig::common::serialization::Value> Get(std::string_view name, std::string_view path);

    static bool Set(std::string_view name, std::string_view path, kconfig::common::serialization::Value value);
    static bool Erase(std::string_view name, std::string_view path);
    // Sets an explicit asset root for this config/view.
    // When set, this takes precedence over any backing-file-derived root.
    static bool SetAssetRoot(std::string_view name, const std::filesystem::path &fullFilesystemPath);
    // Associates a full filesystem path used by Path()/Save().
    // If no explicit asset root is set via SetAssetRoot(), asset resolution
    // for this entry defaults to the backing file's parent directory.
    static bool SetBackingFile(std::string_view name, const std::filesystem::path &fullFilesystemPath);
    static bool RemoveBackingFile(std::string_view name);
    static const std::filesystem::path *Path(std::string_view name);
    static bool Save(std::string_view name, std::string *error = nullptr);

    // Legacy compatibility API.
    static void Initialize(const std::vector<ConfigFileSpec> &defaultSpecs,
                           const std::optional<std::filesystem::path> &userConfigPath = std::nullopt,
                           const std::vector<ConfigFileSpec> &runtimeSpecs = {});
    static bool Initialized();
    static uint64_t Revision();

    static const kconfig::common::serialization::Value &Defaults();
    static const kconfig::common::serialization::Value &User();
    static const kconfig::common::serialization::Value &Merged();

    static const kconfig::common::serialization::Value *Get(std::string_view path);
    static std::optional<kconfig::common::serialization::Value> GetCopy(std::string_view path);

    static bool Set(std::string_view path, kconfig::common::serialization::Value value);
    static bool Erase(std::string_view path);
    static bool ReplaceUserConfig(kconfig::common::serialization::Value userConfig, std::string *error = nullptr);

    static bool SaveUser(std::string *error = nullptr);
    static void Tick();
    static const std::filesystem::path &UserConfigPath();

    static bool AddRuntimeLayer(const std::string &label,
                                const kconfig::common::serialization::Value &layerJson,
                                const std::filesystem::path &baseDir);
    static bool RemoveRuntimeLayer(const std::string &label);
    static const kconfig::common::serialization::Value *LayerByLabel(const std::string &label);

    static std::filesystem::path ResolveAssetPath(const std::string &assetKey,
                                                  const std::filesystem::path &defaultPath = {});

private:
    static void rebuildMergedLocked();
    static void syncLegacyNamedStateLocked();
    static const kconfig::common::serialization::Value *findNamedValueLocked(std::string_view name);
    static kconfig::common::serialization::Value *findMutableNamedValueLocked(std::string_view name);
    static bool writeJsonFileUnlocked(const std::filesystem::path &path,
                                      const kconfig::common::serialization::Value &json,
                                      std::string *error);
    static bool setValueAtPath(kconfig::common::serialization::Value &root, std::string_view path, kconfig::common::serialization::Value value);
    static bool eraseValueAtPath(kconfig::common::serialization::Value &root, std::string_view path);
    static bool saveUserUnlocked(std::string *error, bool ignoreInterval);
};

} // namespace kconfig::common::config
