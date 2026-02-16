#pragma once

#include "karma/cli/client_app_options.hpp"

#include <string_view>

namespace karma::app {

void RunClientBootstrap(const karma::cli::ClientAppOptions& options,
                        int argc,
                        char** argv,
                        std::string_view app_name);

} // namespace karma::app
