#pragma once

#include "karma/cli/cli_parse_scaffold.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

namespace karma::cli {

CliConsumeResult ConsumeServerRuntimeCliOption(const std::string& arg,
                                               int& index,
                                               int argc,
                                               char** argv,
                                               std::string& server_config_path_out,
                                               bool& server_config_explicit_out,
                                               uint16_t& listen_port_out,
                                               bool& listen_port_explicit_out,
                                               std::string& community_out,
                                               bool& community_explicit_out);

void AppendServerRuntimeCliHelp(std::ostream& out);

std::optional<std::filesystem::path> ResolveServerConfigOverlayPath(const std::string& server_config_path,
                                                                    bool server_config_explicit);

std::optional<std::filesystem::path> ApplyServerConfigOverlay(const std::string& server_config_path,
                                                              bool server_config_explicit);

uint16_t ResolveServerListenPort(uint16_t listen_port,
                                 bool listen_port_explicit,
                                 uint16_t fallback_port);

} // namespace karma::cli
