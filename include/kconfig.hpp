#pragma once

#include <string_view>

#include <kconfig/data/directory_override.hpp>
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/data/path_utils.hpp>
#include <kconfig/data/root_policy.hpp>
#include <kconfig/i18n.hpp>
#include <kconfig/json.hpp>
#include <kconfig/store.hpp>

namespace kconfig {

void Initialize();
void ParseCLI(int& argc, char** argv, std::string_view config_root = "config");

} // namespace kconfig
