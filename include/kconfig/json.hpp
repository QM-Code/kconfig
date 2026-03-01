#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace kconfig::common::serialization {

using Value = nlohmann::json;

inline Value Parse(std::string_view text) {
    return Value::parse(text);
}

inline Value Object() {
    return Value::object();
}

inline std::string Dump(const Value& value, int indent = -1) {
    return value.dump(indent);
}

} // namespace kconfig::common::serialization
