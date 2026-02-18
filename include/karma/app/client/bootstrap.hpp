#pragma once

#include "karma/cli/client/app_options.hpp"

#include <string_view>

namespace karma::app::client {

void RunBootstrap(const karma::cli::client::AppOptions& options,
                  int argc,
                  char** argv,
                  std::string_view app_name);

} // namespace karma::app::client
