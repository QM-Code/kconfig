#pragma once

#include <string_view>

namespace kconfig::init {

void Initialize();
void ParseCLI(int& argc, char** argv, std::string_view config_root = "--config");

} // namespace kconfig::init
