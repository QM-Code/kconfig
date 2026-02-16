#pragma once

#include "karma/cli/client_app_options.hpp"

#include <functional>
#include <string_view>

namespace karma::app {

struct ClientRunSpec {
    std::string_view parse_app_name = "app";
    std::string_view bootstrap_app_name = "app";
};

using ClientRuntimeHook = std::function<int(const karma::cli::ClientAppOptions&)>;

int RunClient(int argc,
              char** argv,
              const ClientRunSpec& spec,
              const ClientRuntimeHook& runtime_hook);

} // namespace karma::app
