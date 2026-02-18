#pragma once

#include "karma/cli/server_app_options.hpp"

#include <functional>
#include <string_view>

namespace karma::app::server {

struct RunSpec {
    std::string_view parse_app_name = "app";
    std::string_view bootstrap_app_name = "app";
};

using RuntimeHook = std::function<int(const karma::cli::ServerAppOptions&)>;

int Run(int argc,
        char** argv,
        const RunSpec& spec,
        const RuntimeHook& runtime_hook);

} // namespace karma::app::server
