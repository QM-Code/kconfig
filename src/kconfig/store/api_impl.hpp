#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>
#include <kconfig/store.hpp>

namespace kconfig::store::api {

bool AddMutable(std::string_view name, const kconfig::json::Value& json);
bool AddReadOnly(std::string_view name, const kconfig::json::Value& json);
bool LoadMutable(std::string_view name, const std::filesystem::path& filename);
bool LoadReadOnly(std::string_view name, const std::filesystem::path& filename);
bool Merge(std::string_view targetName, const std::vector<std::string>& sourceNames);
bool Unregister(std::string_view name);
bool Delete(std::string_view name);
std::optional<kconfig::json::Value> Get(std::string_view name, std::string_view path);
bool Set(std::string_view name, std::string_view path, kconfig::json::Value value);
bool Erase(std::string_view name, std::string_view path);
bool StoreCliConfig(std::string_view name, std::string_view text, std::string* error = nullptr);
bool SetAssetRoot(std::string_view name, const std::filesystem::path& fullFilesystemPath);
bool SetBackingFile(std::string_view name, const std::filesystem::path& fullFilesystemPath);
bool DetachBackingFile(std::string_view name);
const std::filesystem::path* BackingFilePath(std::string_view name);
bool SetUserConfigFilePath(const std::filesystem::path& fullFilesystemPath);
bool WriteBackingFile(std::string_view name, std::string* error = nullptr);
bool ReloadBackingFile(std::string_view name, std::string* error = nullptr);
bool SetUserConfigDirname(std::string_view dirname);
bool HasUserConfigFile();
bool InitializeUserConfigFile(const kconfig::json::Value& json, std::string* error = nullptr);
bool LoadUserConfigFile(std::string_view name,
                        const LoadUserConfigFileOptions& options = {},
                        std::string* error = nullptr);
bool SetSaveIntervalSeconds(std::string_view name, std::optional<double> seconds);
std::optional<double> SaveIntervalSeconds(std::string_view name);
bool FlushWrites(std::string* error = nullptr);

} // namespace kconfig::store::api
