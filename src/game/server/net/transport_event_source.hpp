#pragma once

#include <cstdint>
#include <memory>

#include "server/net/event_source.hpp"

namespace bz3::server::net {

std::unique_ptr<ServerEventSource> CreateServerTransportEventSource(uint16_t port);

} // namespace bz3::server::net
