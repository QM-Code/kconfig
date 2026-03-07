#include <kconfig/store.hpp>

#include "store/api_impl.hpp"

#include <utility>

namespace kconfig::store {

bool AddMutable(std::string_view name, const kconfig::json::Value& json) {
    return api::AddMutable(name, json);
}

bool AddReadOnly(std::string_view name, const kconfig::json::Value& json) {
    return api::AddReadOnly(name, json);
}

bool LoadMutable(std::string_view name, const std::filesystem::path& filename) {
    return api::LoadMutable(name, filename);
}

bool LoadReadOnly(std::string_view name, const std::filesystem::path& filename) {
    return api::LoadReadOnly(name, filename);
}

bool Merge(std::string_view targetName, const std::vector<std::string>& sourceNames) {
    return api::Merge(targetName, sourceNames);
}

bool Unregister(std::string_view name) {
    return api::Unregister(name);
}

bool Delete(std::string_view name) {
    return api::Delete(name);
}

std::optional<kconfig::json::Value> Get(std::string_view name, std::string_view path) {
    return api::Get(name, path);
}

bool Set(std::string_view name, std::string_view path, kconfig::json::Value value) {
    return api::Set(name, path, std::move(value));
}

bool Erase(std::string_view name, std::string_view path) {
    return api::Erase(name, path);
}

bool StoreCliConfig(std::string_view name, std::string_view text, std::string* error) {
    return api::StoreCliConfig(name, text, error);
}

bool SetAssetRoot(std::string_view name, const std::filesystem::path& fullFilesystemPath) {
    return api::SetAssetRoot(name, fullFilesystemPath);
}

bool SetBackingFile(std::string_view name, const std::filesystem::path& fullFilesystemPath) {
    return api::SetBackingFile(name, fullFilesystemPath);
}

bool DetachBackingFile(std::string_view name) {
    return api::DetachBackingFile(name);
}

const std::filesystem::path* BackingFilePath(std::string_view name) {
    return api::BackingFilePath(name);
}

bool SetUserConfigFilePath(const std::filesystem::path& fullFilesystemPath) {
    return api::SetUserConfigFilePath(fullFilesystemPath);
}

bool WriteBackingFile(std::string_view name, std::string* error) {
    return api::WriteBackingFile(name, error);
}

bool ReloadBackingFile(std::string_view name, std::string* error) {
    return api::ReloadBackingFile(name, error);
}

bool SetUserConfigDirname(std::string_view dirname) {
    return api::SetUserConfigDirname(dirname);
}

bool HasUserConfigFile() {
    return api::HasUserConfigFile();
}

bool InitializeUserConfigFile(const kconfig::json::Value& json, std::string* error) {
    return api::InitializeUserConfigFile(json, error);
}

bool LoadUserConfigFile(std::string_view name,
                        const LoadUserConfigFileOptions& options,
                        std::string* error) {
    return api::LoadUserConfigFile(name, options, error);
}

bool SetSaveIntervalSeconds(std::string_view name, std::optional<double> seconds) {
    return api::SetSaveIntervalSeconds(name, seconds);
}

std::optional<double> SaveIntervalSeconds(std::string_view name) {
    return api::SaveIntervalSeconds(name);
}

bool FlushWrites(std::string* error) {
    return api::FlushWrites(error);
}

} // namespace kconfig::store
