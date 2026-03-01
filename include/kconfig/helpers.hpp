#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <cstdint>

#include "kconfig/json.hpp"

namespace kconfig::common::config {

bool ReadRequiredBoolConfig(const char *path);
uint16_t ReadRequiredUInt16Config(const char *path);
uint16_t ReadRequiredPositiveUInt16Config(const char *path);
uint32_t ReadRequiredUInt32Config(const char *path);
float ReadRequiredFloatConfig(const char *path);
float ReadRequiredPositiveFiniteFloatConfig(const char *path);
std::string ReadRequiredStringConfig(const char *path);
std::string ReadRequiredNonEmptyStringConfig(const char *path);
std::vector<float> ReadRequiredFloatArrayConfig(std::string_view path);
const kconfig::common::serialization::Value& ReadRequiredObjectConfig(std::string_view path);

} // namespace kconfig::common::config
