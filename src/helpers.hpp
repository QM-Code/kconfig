#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <cstdint>

#include <kconfig/json.hpp>

namespace kconfig::store {

bool ReadBoolConfig(std::initializer_list<const char*> paths, bool defaultValue);
uint16_t ReadUInt16Config(std::initializer_list<const char*> paths, uint16_t defaultValue);
float ReadFloatConfig(std::initializer_list<const char*> paths, float defaultValue);
std::string ReadStringConfig(const char *path, const std::string &defaultValue);
bool ReadRequiredBool(const char *path);
uint16_t ReadRequiredUInt16(const char *path);
uint16_t ReadRequiredPositiveUInt16(const char *path);
uint32_t ReadRequiredUInt32(const char *path);
float ReadRequiredFloat(const char *path);
float ReadRequiredPositiveFiniteFloat(const char *path);
std::string ReadRequiredString(const char *path);
std::string ReadRequiredNonEmptyString(const char *path);
std::vector<float> ReadRequiredFloatArray(std::string_view path);
std::vector<std::string> ReadRequiredStringArrayConfig(std::string_view path);
kconfig::json::Value ReadRequiredObjectConfig(std::string_view path);

} // namespace kconfig::store
