#include "server/runtime_event_rules.hpp"

namespace bz3::server {

RuntimeEventRuleResult ApplyRuntimeEventRules(const net::ServerInputEvent& event,
                                              RuntimeEventRuleHandlers& handlers,
                                              uint32_t* next_global_shot_id) {
    switch (event.type) {
        case net::ServerInputEvent::Type::ClientLeave: {
            if (!handlers.has_client || !handlers.on_client_leave || !handlers.on_player_death) {
                return RuntimeEventRuleResult::InvalidHandlers;
            }
            const uint32_t client_id = event.leave.client_id;
            if (!handlers.has_client(client_id)) {
                return RuntimeEventRuleResult::IgnoredUnknownClient;
            }
            if (!handlers.on_client_leave(client_id)) {
                return RuntimeEventRuleResult::IgnoredLeaveFailed;
            }
            handlers.on_player_death(client_id);
            return RuntimeEventRuleResult::Applied;
        }
        case net::ServerInputEvent::Type::ClientRequestSpawn: {
            if (!handlers.has_client || !handlers.on_player_spawn) {
                return RuntimeEventRuleResult::InvalidHandlers;
            }
            const uint32_t client_id = event.request_spawn.client_id;
            if (!handlers.has_client(client_id)) {
                return RuntimeEventRuleResult::IgnoredUnknownClient;
            }
            handlers.on_player_spawn(client_id);
            return RuntimeEventRuleResult::Applied;
        }
        case net::ServerInputEvent::Type::ClientCreateShot: {
            if (!handlers.has_client || !handlers.on_create_shot || !next_global_shot_id) {
                return RuntimeEventRuleResult::InvalidHandlers;
            }
            const uint32_t client_id = event.create_shot.client_id;
            if (!handlers.has_client(client_id)) {
                return RuntimeEventRuleResult::IgnoredUnknownClient;
            }
            const uint32_t global_shot_id = (*next_global_shot_id)++;
            handlers.on_create_shot(client_id,
                                    global_shot_id,
                                    event.create_shot.pos_x,
                                    event.create_shot.pos_y,
                                    event.create_shot.pos_z,
                                    event.create_shot.vel_x,
                                    event.create_shot.vel_y,
                                    event.create_shot.vel_z);
            return RuntimeEventRuleResult::Applied;
        }
        default:
            return RuntimeEventRuleResult::UnsupportedEvent;
    }
}

} // namespace bz3::server
