#pragma once

#include "server/cli_options.hpp"

namespace bz3::server {

void ConfigureLogging(const CLIOptions& options);
void ConfigureDataAndConfig(const CLIOptions& options, int argc, char** argv);

} // namespace bz3::server
