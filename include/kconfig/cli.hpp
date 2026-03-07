#pragma once

#include <string>
#include <string_view>

namespace kconfig::cli {

bool StoreAssignment(std::string_view name,
                     std::string_view text,
                     std::string* error = nullptr);

void ParseArgs(int& argc,
               char** argv,
               std::string_view configRoot = "config");

} // namespace kconfig::cli
