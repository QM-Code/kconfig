#pragma once

#include "karma/cli/client/app_options.hpp"

#include <functional>
#include <string_view>

namespace karma::app::client {

struct RunSpec {
    std::string_view parse_app_name = "app";
    std::string_view bootstrap_app_name = "app";
};

using RuntimeHook = std::function<int(const karma::cli::client::AppOptions&)>;

int Run(int argc,
        char** argv,
        const RunSpec& spec,
        const RuntimeHook& runtime_hook);

} // namespace karma::app::client
