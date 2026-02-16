#include "karma/cli/client_app_options.hpp"
#include "karma/cli/client_runtime_options.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

bool Fail(const std::string& message) {
    std::cerr << message << "\n";
    return false;
}

bool Expect(bool condition, const std::string& message) {
    if (!condition) {
        return Fail(message);
    }
    return true;
}

bool TestClientRuntimeFlagsRouteThroughParseCliOptions() {
    std::vector<std::string> args{
        "bz3",
        "--username",
        "alice",
        "--password",
        "secret",
        "--server",
        "localhost:5154"};
    std::vector<char*> argv{};
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }

    const karma::cli::ClientAppOptions options =
        karma::cli::ParseClientAppCliOptions(static_cast<int>(argv.size()), argv.data(), "bz3");
    return Expect(options.username_explicit, "expected --username to set username_explicit")
           && Expect(options.username == "alice", "expected --username value to round-trip")
           && Expect(options.password_explicit, "expected --password to set password_explicit")
           && Expect(options.password == "secret", "expected --password value to round-trip")
           && Expect(options.server_explicit, "expected --server to set server_explicit")
           && Expect(options.server == "localhost:5154", "expected --server value to round-trip");
}

bool TestClientServerEndpointResolution() {
    std::string error{};
    const auto endpoint = karma::cli::ResolveClientServerEndpoint("127.0.0.1:3000", true, &error);
    if (!Expect(endpoint.has_value(), "expected valid --server endpoint to resolve")) {
        return false;
    }
    if (!Expect(endpoint->host == "127.0.0.1", "expected endpoint host to parse")) {
        return false;
    }
    if (!Expect(endpoint->port == 3000, "expected endpoint port to parse")) {
        return false;
    }

    const auto fallback_name = karma::cli::ResolveClientPlayerName("", false, "Player");
    if (!Expect(fallback_name == "Player", "expected fallback player name when username is omitted")) {
        return false;
    }
    const auto explicit_name = karma::cli::ResolveClientPlayerName("alice", true, "Player");
    if (!Expect(explicit_name == "alice", "expected explicit username to win")) {
        return false;
    }

    error.clear();
    const auto invalid = karma::cli::ResolveClientServerEndpoint("localhost:notaport", true, &error);
    return Expect(!invalid.has_value(), "expected invalid --server endpoint to fail resolution")
           && Expect(!error.empty(), "expected endpoint parse error message");
}

bool TestNoLegacyAliasConsumption() {
    int index_name = 0;
    int index_addr = 0;
    int index_credentials = 0;
    int index_dev_quick_start = 0;
    int index_community_list_active = 0;
    char* argv_stub[1] = {nullptr};
    std::string username{};
    std::string password{};
    std::string server{};
    bool username_explicit = false;
    bool password_explicit = false;
    bool server_explicit = false;

    const auto name_result = karma::cli::ConsumeClientRuntimeCliOption("--name",
                                                                        index_name,
                                                                        0,
                                                                        argv_stub,
                                                                        username,
                                                                        username_explicit,
                                                                        password,
                                                                        password_explicit,
                                                                        server,
                                                                        server_explicit);
    const auto addr_result = karma::cli::ConsumeClientRuntimeCliOption("--addr=localhost:1",
                                                                        index_addr,
                                                                        0,
                                                                        argv_stub,
                                                                        username,
                                                                        username_explicit,
                                                                        password,
                                                                        password_explicit,
                                                                        server,
                                                                        server_explicit);
    const auto credentials_result = karma::cli::ConsumeClientRuntimeCliOption("--credentials=opaque",
                                                                        index_credentials,
                                                                        0,
                                                                        argv_stub,
                                                                        username,
                                                                        username_explicit,
                                                                        password,
                                                                        password_explicit,
                                                                        server,
                                                                        server_explicit);
    const auto dev_quick_start_result = karma::cli::ConsumeClientRuntimeCliOption("--dev-quick-start",
                                                                        index_dev_quick_start,
                                                                        0,
                                                                        argv_stub,
                                                                        username,
                                                                        username_explicit,
                                                                        password,
                                                                        password_explicit,
                                                                        server,
                                                                        server_explicit);
    const auto community_list_active_result =
        karma::cli::ConsumeClientRuntimeCliOption("--community-list-active=localhost:8080",
                                                  index_community_list_active,
                                                  0,
                                                  argv_stub,
                                                  username,
                                                  username_explicit,
                                                  password,
                                                  password_explicit,
                                                  server,
                                                  server_explicit);
    return Expect(!name_result.consumed, "legacy --name alias must not be consumed")
           && Expect(!addr_result.consumed, "legacy --addr alias must not be consumed")
           && Expect(!credentials_result.consumed, "legacy --credentials alias must not be consumed")
           && Expect(!dev_quick_start_result.consumed, "legacy --dev-quick-start flag must not be consumed")
           && Expect(!community_list_active_result.consumed,
                     "legacy --community-list-active flag must not be consumed");
}

} // namespace

int main() {
    if (!TestClientRuntimeFlagsRouteThroughParseCliOptions()) {
        return 1;
    }
    if (!TestClientServerEndpointResolution()) {
        return 1;
    }
    if (!TestNoLegacyAliasConsumption()) {
        return 1;
    }
    return 0;
}
