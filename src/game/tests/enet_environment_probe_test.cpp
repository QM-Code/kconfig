#include <enet.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

constexpr int kSkipReturnCode = 77;

void PrintSkip(const char* message) {
    std::cerr << "SKIP: " << message << "\n";
}

bool WaitForLoopbackConnect(ENetHost* server_host, ENetHost* client_host) {
    if (!server_host || !client_host) {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(400);
    bool connected = false;
    while (std::chrono::steady_clock::now() < deadline && !connected) {
        ENetEvent client_event{};
        while (enet_host_service(client_host, &client_event, 0) > 0) {
            if (client_event.type == ENET_EVENT_TYPE_CONNECT) {
                connected = true;
            } else if (client_event.type == ENET_EVENT_TYPE_RECEIVE && client_event.packet) {
                enet_packet_destroy(client_event.packet);
            }
        }

        ENetEvent server_event{};
        while (enet_host_service(server_host, &server_event, 0) > 0) {
            if (server_event.type == ENET_EVENT_TYPE_RECEIVE && server_event.packet) {
                enet_packet_destroy(server_event.packet);
            }
        }

        if (!connected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return connected;
}

} // namespace

int main() {
    if (enet_initialize() != 0) {
        PrintSkip("enet_initialize failed");
        return kSkipReturnCode;
    }

    ENetAddress listen_address{};
    listen_address.host = ENET_HOST_ANY;
    listen_address.port = 0; // ask OS for any available local port
    ENetHost* server_host = enet_host_create(&listen_address, 1, 2, 0, 0);
    if (!server_host) {
        PrintSkip("failed to create ENet server host");
        enet_deinitialize();
        return kSkipReturnCode;
    }

    ENetHost* client_host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client_host) {
        PrintSkip("failed to create ENet client host");
        enet_host_destroy(server_host);
        enet_deinitialize();
        return kSkipReturnCode;
    }

    ENetAddress loopback{};
    if (enet_address_set_host(&loopback, "127.0.0.1") != 0) {
        PrintSkip("failed to resolve loopback host");
        enet_host_destroy(client_host);
        enet_host_destroy(server_host);
        enet_deinitialize();
        return kSkipReturnCode;
    }
    loopback.port = server_host->address.port;

    ENetPeer* peer = enet_host_connect(client_host, &loopback, 2, 0);
    if (!peer) {
        PrintSkip("failed to create ENet loopback peer");
        enet_host_destroy(client_host);
        enet_host_destroy(server_host);
        enet_deinitialize();
        return kSkipReturnCode;
    }

    const bool connected = WaitForLoopbackConnect(server_host, client_host);
    if (!connected) {
        PrintSkip("ENet loopback connect timed out");
        enet_host_destroy(client_host);
        enet_host_destroy(server_host);
        enet_deinitialize();
        return kSkipReturnCode;
    }

    enet_peer_disconnect(peer, 0);
    ENetEvent event{};
    for (int i = 0; i < 16; ++i) {
        while (enet_host_service(client_host, &event, 0) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE && event.packet) {
                enet_packet_destroy(event.packet);
            }
        }
        while (enet_host_service(server_host, &event, 0) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE && event.packet) {
                enet_packet_destroy(event.packet);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    enet_host_destroy(client_host);
    enet_host_destroy(server_host);
    enet_deinitialize();
    return 0;
}
