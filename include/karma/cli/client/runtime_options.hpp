#pragma once

#include "karma/cli/shared/parse.hpp"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

namespace karma::cli::client {

struct ServerEndpoint {
    std::string host{};
    uint16_t port = 0;
};

shared::ConsumeResult ConsumeRuntimeOption(const std::string& arg,
                                           int& index,
                                           int argc,
                                           char** argv,
                                           std::string& username_out,
                                           bool& username_explicit_out,
                                           std::string& password_out,
                                           bool& password_explicit_out,
                                           std::string& server_out,
                                           bool& server_explicit_out);

void AppendRuntimeHelp(std::ostream& out);

std::string ResolvePlayerName(const std::string& username,
                              bool username_explicit,
                              const std::string& fallback_player_name);

std::optional<ServerEndpoint> ResolveServerEndpoint(const std::string& server,
                                                    bool server_explicit,
                                                    std::string* out_error);

} // namespace karma::cli::client
