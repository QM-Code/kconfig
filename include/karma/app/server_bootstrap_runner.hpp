#pragma once

#include "karma/cli/server_app_options.hpp"

#include <string_view>

namespace karma::app {

void RunServerBootstrap(const karma::cli::ServerAppOptions& options,
                        int argc,
                        char** argv,
                        std::string_view app_name);

} // namespace karma::app
