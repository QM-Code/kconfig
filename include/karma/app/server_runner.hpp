#pragma once

#include "karma/cli/server_app_options.hpp"

#include <functional>
#include <string_view>

namespace karma::app {

struct ServerRunSpec {
    std::string_view parse_app_name = "app";
    std::string_view bootstrap_app_name = "app";
};

using ServerRuntimeHook = std::function<int(const karma::cli::ServerAppOptions&)>;

int RunServer(int argc,
              char** argv,
              const ServerRunSpec& spec,
              const ServerRuntimeHook& runtime_hook);

} // namespace karma::app
