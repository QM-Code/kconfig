#include <kconfig/store.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kconfig::store {

namespace {

struct NamespacedPath {
    std::string name;
    std::string path;
};

bool ParseNamespacedPath(std::string_view qualifiedPath,
                         NamespacedPath& parsed,
                         std::string* error = nullptr) {
    if (qualifiedPath.empty()) {
        if (error) {
            *error = "path must not be empty";
        }
        return false;
    }

    const std::size_t dot = qualifiedPath.find('.');
    if (dot == std::string_view::npos || dot == 0) {
        if (error) {
            *error = "expected '<namespace>.<json-path>'";
        }
        return false;
    }

    parsed.name = std::string(qualifiedPath.substr(0, dot));
    for (const char ch : parsed.name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (error) {
                *error = "namespace must not contain whitespace";
            }
            return false;
        }
    }

    const auto suffix = qualifiedPath.substr(dot + 1);
    parsed.path = suffix.empty() ? std::string(".") : std::string(suffix);
    return true;
}

std::optional<kconfig::json::Value> GetNamespacedValue(std::string_view qualifiedPath,
                                                        bool warnOnInvalidPath = true) {
    NamespacedPath parsed;
    std::string parseError;
    if (!ParseNamespacedPath(qualifiedPath, parsed, &parseError)) {
        if (warnOnInvalidPath) {
            spdlog::warn("Config path '{}' is invalid: {}", qualifiedPath, parseError);
        }
        return std::nullopt;
    }

    return kconfig::store::Get(parsed.name, parsed.path);
}

NamespacedPath ParseRequiredNamespacedPath(std::string_view qualifiedPath) {
    NamespacedPath parsed;
    std::string parseError;
    if (!ParseNamespacedPath(qualifiedPath, parsed, &parseError)) {
        throw std::runtime_error(std::string("Invalid required config path: ")
                                 + std::string(qualifiedPath)
                                 + " ("
                                 + parseError
                                 + ")");
    }
    return parsed;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::optional<bool> TryParseBoolValue(const kconfig::json::Value& value) {
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number_integer()) {
        return value.get<long long>() != 0;
    }
    if (value.is_number_float()) {
        return value.get<double>() != 0.0;
    }
    if (value.is_string()) {
        const std::string text = ToLower(value.get<std::string>());
        if (text == "true" || text == "1" || text == "yes" || text == "on") {
            return true;
        }
        if (text == "false" || text == "0" || text == "no" || text == "off") {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<unsigned long long> TryParseUnsignedValue(const kconfig::json::Value& value) {
    if (value.is_number_unsigned()) {
        return value.get<unsigned long long>();
    }
    if (value.is_number_integer()) {
        const long long signedValue = value.get<long long>();
        if (signedValue < 0) {
            return std::nullopt;
        }
        return static_cast<unsigned long long>(signedValue);
    }
    if (value.is_string()) {
        const std::string raw = value.get<std::string>();
        try {
            size_t consumed = 0;
            const unsigned long long parsed = std::stoull(raw, &consumed, 0);
            if (consumed == raw.size()) {
                return parsed;
            }
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<uint16_t> TryParseUInt16Value(const kconfig::json::Value& value) {
    if (const auto parsed = TryParseUnsignedValue(value)) {
        if (*parsed <= std::numeric_limits<uint16_t>::max()) {
            return static_cast<uint16_t>(*parsed);
        }
    }
    if (value.is_number_float()) {
        const double parsed = value.get<double>();
        if (std::isfinite(parsed)
            && parsed >= 0.0
            && parsed <= static_cast<double>(std::numeric_limits<uint16_t>::max())) {
            return static_cast<uint16_t>(parsed);
        }
    }
    return std::nullopt;
}

std::optional<uint32_t> TryParseUInt32Value(const kconfig::json::Value& value) {
    if (const auto parsed = TryParseUnsignedValue(value)) {
        if (*parsed <= std::numeric_limits<uint32_t>::max()) {
            return static_cast<uint32_t>(*parsed);
        }
    }
    return std::nullopt;
}

std::optional<float> TryParseFloatValue(const kconfig::json::Value& value) {
    if (value.is_number_float()) {
        return static_cast<float>(value.get<double>());
    }
    if (value.is_number_integer()) {
        return static_cast<float>(value.get<long long>());
    }
    if (value.is_string()) {
        try {
            return std::stof(value.get<std::string>());
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<std::vector<float>> TryParseFloatArrayValue(const kconfig::json::Value& value) {
    if (!value.is_array()) {
        return std::nullopt;
    }

    std::vector<float> out;
    out.reserve(value.size());
    for (const auto& entry : value) {
        if (entry.is_number_float()) {
            out.push_back(static_cast<float>(entry.get<double>()));
            continue;
        }
        if (entry.is_number_integer()) {
            out.push_back(static_cast<float>(entry.get<long long>()));
            continue;
        }
        return std::nullopt;
    }
    return out;
}

std::optional<std::vector<std::string>> TryParseStringArrayValue(const kconfig::json::Value& value) {
    if (!value.is_array()) {
        return std::nullopt;
    }

    std::vector<std::string> out;
    out.reserve(value.size());
    for (const auto& entry : value) {
        if (!entry.is_string()) {
            return std::nullopt;
        }
        out.push_back(entry.get<std::string>());
    }
    return out;
}

} // namespace

bool ReadBool(std::initializer_list<const char*> paths, bool defaultValue) {
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseBoolValue(*value)) {
                return *parsed;
            }
            spdlog::warn("Config '{}' cannot be interpreted as boolean", path);
        }
    }
    return defaultValue;
}

bool ReadRequiredBool(const char* path) {
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("Missing required boolean config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseBoolValue(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required boolean config: ") + path);
    }
    throw std::runtime_error(std::string("Missing required boolean config: ") + path);
}

uint16_t ReadUInt16(std::initializer_list<const char*> paths, uint16_t defaultValue) {
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseUInt16Value(*value)) {
                return *parsed;
            }
            spdlog::warn("Config '{}' cannot be interpreted as uint16", path);
        }
    }
    return defaultValue;
}

uint16_t ReadRequiredUInt16(const char* path) {
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("Missing required uint16 config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseUInt16Value(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required uint16 config: ") + path);
    }
    throw std::runtime_error(std::string("Missing required uint16 config: ") + path);
}

uint16_t ReadPositiveUInt16(std::initializer_list<const char*> paths, uint16_t defaultValue) {
    const uint16_t value = ReadUInt16(paths, defaultValue);
    if (value == 0) {
        return defaultValue;
    }
    return value;
}

uint16_t ReadRequiredPositiveUInt16(const char* path) {
    const uint16_t value = ReadRequiredUInt16(path);
    if (value == 0) {
        throw std::runtime_error(std::string("Invalid required uint16 config: ")
                                 + path
                                 + " (must be > 0)");
    }
    return value;
}

uint32_t ReadUInt32(std::initializer_list<const char*> paths, uint32_t defaultValue) {
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseUInt32Value(*value)) {
                return *parsed;
            }
            spdlog::warn("Config '{}' cannot be interpreted as uint32", path);
        }
    }
    return defaultValue;
}

uint32_t ReadRequiredUInt32(const char* path) {
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("Missing required uint32 config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseUInt32Value(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required uint32 config: ") + path);
    }
    throw std::runtime_error(std::string("Missing required uint32 config: ") + path);
}

float ReadFloat(std::initializer_list<const char*> paths, float defaultValue) {
    for (const char* path : paths) {
        if (path == nullptr || *path == '\0') {
            continue;
        }
        if (const auto value = GetNamespacedValue(path); value) {
            if (const auto parsed = TryParseFloatValue(*value)) {
                return *parsed;
            }
            spdlog::warn("Config '{}' cannot be interpreted as float", path);
        }
    }
    return defaultValue;
}

float ReadRequiredFloat(const char* path) {
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("Missing required float config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (const auto parsed = TryParseFloatValue(*value)) {
            return *parsed;
        }
        throw std::runtime_error(std::string("Invalid required float config: ") + path);
    }
    throw std::runtime_error(std::string("Missing required float config: ") + path);
}

float ReadPositiveFiniteFloat(std::initializer_list<const char*> paths, float defaultValue) {
    const float value = ReadFloat(paths, defaultValue);
    if (!std::isfinite(value) || value <= 0.0f) {
        return defaultValue;
    }
    return value;
}

float ReadRequiredPositiveFiniteFloat(const char* path) {
    const float value = ReadRequiredFloat(path);
    if (!std::isfinite(value) || value <= 0.0f) {
        throw std::runtime_error(std::string("Invalid required float config: ")
                                 + path
                                 + " (must be finite and > 0)");
    }
    return value;
}

std::string ReadString(const char* path, const std::string& defaultValue) {
    if (path == nullptr || *path == '\0') {
        return defaultValue;
    }
    if (const auto value = GetNamespacedValue(path); value) {
        if (value->is_string()) {
            return value->get<std::string>();
        }
        spdlog::warn("Config '{}' cannot be interpreted as string", path);
    }
    return defaultValue;
}

std::string ReadRequiredString(const char* path) {
    if (path == nullptr || *path == '\0') {
        throw std::runtime_error("Missing required string config path");
    }
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    if (const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path); value) {
        if (value->is_string()) {
            return value->get<std::string>();
        }
        throw std::runtime_error(std::string("Invalid required string config: ") + path);
    }
    throw std::runtime_error(std::string("Missing required string config: ") + path);
}

std::string ReadNonEmptyString(const char* path, const std::string& defaultValue) {
    const std::string value = ReadString(path, defaultValue);
    if (value.empty()) {
        return defaultValue;
    }
    return value;
}

std::string ReadRequiredNonEmptyString(const char* path) {
    const std::string value = ReadRequiredString(path);
    if (value.empty()) {
        throw std::runtime_error(std::string("Invalid required string config: ")
                                 + path
                                 + " (must be non-empty)");
    }
    return value;
}

std::vector<float> ReadFloatArray(std::string_view path, std::vector<float> defaultValue) {
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (const auto parsed = TryParseFloatArrayValue(*value)) {
        return *parsed;
    }
    spdlog::warn("Config '{}' cannot be interpreted as float array", path);
    return defaultValue;
}

std::vector<float> ReadRequiredFloatArray(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required float array config: ") + std::string(path));
    }
    if (const auto parsed = TryParseFloatArrayValue(*value)) {
        return *parsed;
    }
    throw std::runtime_error(std::string("Invalid required float array config: ") + std::string(path));
}

std::vector<std::string> ReadStringArray(std::string_view path, std::vector<std::string> defaultValue) {
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (const auto parsed = TryParseStringArrayValue(*value)) {
        return *parsed;
    }
    spdlog::warn("Config '{}' cannot be interpreted as string array", path);
    return defaultValue;
}

std::vector<std::string> ReadRequiredStringArray(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required string array config: ") + std::string(path));
    }
    if (const auto parsed = TryParseStringArrayValue(*value)) {
        return *parsed;
    }
    throw std::runtime_error(std::string("Invalid required string array config: ") + std::string(path));
}

kconfig::json::Value ReadObject(std::string_view path, kconfig::json::Value defaultValue) {
    const auto value = GetNamespacedValue(path);
    if (!value) {
        return defaultValue;
    }
    if (!value->is_object()) {
        spdlog::warn("Config '{}' cannot be interpreted as object", path);
        return defaultValue;
    }
    return *value;
}

kconfig::json::Value ReadRequiredObject(std::string_view path) {
    const auto parsedPath = ParseRequiredNamespacedPath(path);
    const auto value = kconfig::store::Get(parsedPath.name, parsedPath.path);
    if (!value) {
        throw std::runtime_error(std::string("Missing required object config: ") + std::string(path));
    }
    if (!value->is_object()) {
        throw std::runtime_error(std::string("Invalid required object config: ") + std::string(path));
    }
    return *value;
}

} // namespace kconfig::store
