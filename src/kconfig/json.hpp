#pragma once

#include <kconfig/json.hpp>

#include <nlohmann/json.hpp>

namespace kconfig::json::detail {

Value FromNlohmann(const nlohmann::json& json);
nlohmann::json ToNlohmann(const Value& value);

} // namespace kconfig::json::detail
