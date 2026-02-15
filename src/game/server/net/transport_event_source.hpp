#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "server/net/event_source.hpp"

namespace bz3::server::net {

std::unique_ptr<ServerEventSource> CreateServerTransportEventSource(uint16_t port,
                                                                    std::string_view app_name = "server");

} // namespace bz3::server::net
