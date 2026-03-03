#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>

namespace kconfig::store {

// Named config API (used by external tests/tools).
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
bool SetAssetRoot(std::string_view name, const std::filesystem::path& fullFilesystemPath);
bool SetBackingFile(std::string_view name, const std::filesystem::path& fullFilesystemPath);
bool DetachBackingFile(std::string_view name);
const std::filesystem::path* BackingFilePath(std::string_view name);
bool WriteBackingFile(std::string_view name, std::string* error = nullptr);
bool ReloadBackingFile(std::string_view name, std::string* error = nullptr);

// Required typed readers over merged config data.
bool ReadRequiredBool(const char* path);
uint16_t ReadRequiredUInt16(const char* path);
uint16_t ReadRequiredPositiveUInt16(const char* path);
uint32_t ReadRequiredUInt32(const char* path);
float ReadRequiredFloat(const char* path);
float ReadRequiredPositiveFiniteFloat(const char* path);
std::string ReadRequiredString(const char* path);
std::string ReadRequiredNonEmptyString(const char* path);
std::vector<float> ReadRequiredFloatArray(std::string_view path);

} // namespace kconfig::store
