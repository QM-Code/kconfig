#include "karma/cli/client_runtime_options.hpp"

#include <limits>
#include <stdexcept>

namespace karma::cli {

CliConsumeResult ConsumeClientRuntimeCliOption(const std::string& arg,
                                               int& index,
                                               int argc,
                                               char** argv,
                                               std::string& username_out,
                                               bool& username_explicit_out,
                                               std::string& password_out,
                                               bool& password_explicit_out,
                                               std::string& server_out,
                                               bool& server_explicit_out) {
    CliConsumeResult out{};

    if (arg == "--username") {
        std::string error{};
        auto value = RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!value) {
            out.error = error;
            return out;
        }
        username_out = *value;
        username_explicit_out = true;
        return out;
    }
    if (StartsWith(arg, "--username=")) {
        username_out = ValueAfterEquals(arg, "--username=");
        username_explicit_out = true;
        out.consumed = true;
        return out;
    }

    if (arg == "--password") {
        std::string error{};
        auto value = RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!value) {
            out.error = error;
            return out;
        }
        password_out = *value;
        password_explicit_out = true;
        return out;
    }
    if (StartsWith(arg, "--password=")) {
        password_out = ValueAfterEquals(arg, "--password=");
        password_explicit_out = true;
        out.consumed = true;
        return out;
    }

    if (arg == "--server") {
        std::string error{};
        auto value = RequireValue(arg, index, argc, argv, &error);
        out.consumed = true;
        if (!value) {
            out.error = error;
            return out;
        }
        server_out = *value;
        server_explicit_out = true;
        return out;
    }
    if (StartsWith(arg, "--server=")) {
        server_out = ValueAfterEquals(arg, "--server=");
        server_explicit_out = true;
        out.consumed = true;
        return out;
    }

    return out;
}

void AppendClientRuntimeCliHelp(std::ostream& out) {
    out << "      --username <name>           Player name for join request\n"
        << "      --password <value>          Password/auth payload for pre-auth join\n"
        << "      --server <host:port>        Server endpoint to connect\n";
}

std::string ResolveClientPlayerName(const std::string& username,
                                    bool username_explicit,
                                    const std::string& fallback_player_name) {
    if (username_explicit) {
        return username;
    }
    return fallback_player_name;
}

std::optional<ClientServerEndpoint> ResolveClientServerEndpoint(const std::string& server,
                                                                bool server_explicit,
                                                                std::string* out_error) {
    if (!server_explicit) {
        return std::nullopt;
    }

    const std::size_t split = server.rfind(':');
    if (split == std::string::npos || split == 0 || split + 1 >= server.size()) {
        if (out_error) {
            *out_error = "Client --server must be formatted as <host:port>.";
        }
        return std::nullopt;
    }

    const std::string port_text = server.substr(split + 1);
    uint16_t parsed_port = 0;
    try {
        const unsigned long raw = std::stoul(port_text);
        if (raw == 0 || raw > std::numeric_limits<uint16_t>::max()) {
            throw std::runtime_error("out of range");
        }
        parsed_port = static_cast<uint16_t>(raw);
    } catch (...) {
        if (out_error) {
            *out_error = "Client --server port must be an integer in range 1..65535.";
        }
        return std::nullopt;
    }

    ClientServerEndpoint endpoint{};
    endpoint.host = server.substr(0, split);
    endpoint.port = parsed_port;
    return endpoint;
}

} // namespace karma::cli
