#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <kconfig/json.hpp>

namespace kconfig::store::read {

bool Bool(std::initializer_list<const char*> paths, bool defaultValue);
bool RequiredBool(const char* path);

uint16_t Uint16(std::initializer_list<const char*> paths, uint16_t defaultValue);
uint16_t RequiredUint16(const char* path);

uint16_t PositiveUint16(std::initializer_list<const char*> paths, uint16_t defaultValue);
uint16_t RequiredPositiveUint16(const char* path);

uint32_t Uint32(std::initializer_list<const char*> paths, uint32_t defaultValue);
uint32_t RequiredUint32(const char* path);

float Float(std::initializer_list<const char*> paths, float defaultValue);
float RequiredFloat(const char* path);

float PositiveFiniteFloat(std::initializer_list<const char*> paths, float defaultValue);
float RequiredPositiveFiniteFloat(const char* path);

std::string String(const char* path, const std::string& defaultValue);
std::string RequiredString(const char* path);

std::string NonEmptyString(const char* path, const std::string& defaultValue);
std::string RequiredNonEmptyString(const char* path);

std::vector<float> FloatArray(std::string_view path, std::vector<float> defaultValue = {});
std::vector<float> RequiredFloatArray(std::string_view path);

std::vector<std::string> StringArray(std::string_view path, std::vector<std::string> defaultValue = {});
std::vector<std::string> RequiredStringArray(std::string_view path);

kconfig::json::Value Object(std::string_view path, kconfig::json::Value defaultValue = kconfig::json::Object());
kconfig::json::Value RequiredObject(std::string_view path);

} // namespace kconfig::store::read
