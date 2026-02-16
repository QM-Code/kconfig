#include "server/runtime.hpp"
#include "server/domain/shot_system.hpp"
#include "server/domain/world_session.hpp"
#include "server/net/event_source.hpp"
#include "server/server_game.hpp"
#include "server/runtime_event_rules.hpp"

#include "karma/app/backend_resolution.hpp"
#include "karma/app/bootstrap_scaffold.hpp"
#include "karma/cli/server_runtime_options.hpp"
#include "karma/app/engine_server_app.hpp"
#include "karma/common/config_helpers.hpp"
#include "karma/common/config_store.hpp"
#include "karma/common/config_validation.hpp"
#include "karma/common/logging.hpp"
#include "karma/network/community_heartbeat.hpp"
#include "karma/network/server_join_runtime.hpp"
#include "karma/network/server_preauth.hpp"
#include "karma/network/server_session_hooks.hpp"
#include "karma/network/server_session_runtime.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <csignal>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace bz3::server {

namespace {

std::atomic<bool> g_running{true};
std::string g_app_name = "server";

void OnSignal(int signum) {
    KARMA_TRACE("engine.server", "{}: received signal {}, requesting stop", g_app_name, signum);
    g_running.store(false);
}

std::vector<net::SessionSnapshotEntry> ToNetSessionSnapshot(
    const std::vector<karma::network::ServerSessionSnapshotEntry>& sessions) {
    std::vector<net::SessionSnapshotEntry> out{};
    out.reserve(sessions.size());
    for (const auto& session : sessions) {
        out.push_back(net::SessionSnapshotEntry{
            session.session_id,
            session.session_name});
    }
    return out;
}

std::vector<net::WorldManifestEntry> ToNetWorldManifest(
    const std::vector<karma::network::ServerWorldManifestEntry>& manifest) {
    std::vector<net::WorldManifestEntry> out{};
    out.reserve(manifest.size());
    for (const auto& entry : manifest) {
        out.push_back(net::WorldManifestEntry{
            .path = entry.path,
            .size = entry.size,
            .hash = entry.hash});
    }
    return out;
}

void EmitJoinResultToEventSource(net::ServerEventSource& event_source,
                                 const karma::network::ServerJoinResultPayload& payload) {
    static const std::vector<net::WorldManifestEntry> empty_world_manifest{};
    static const std::vector<std::byte> empty_world_package{};

    const auto* world_state = payload.world;
    const auto* world_manifest =
        (world_state && world_state->world_manifest) ? world_state->world_manifest : nullptr;
    const auto* world_package =
        (world_state && world_state->world_package) ? world_state->world_package : &empty_world_package;
    const auto net_sessions = ToNetSessionSnapshot(payload.sessions);
    const auto net_world_manifest = world_manifest ? ToNetWorldManifest(*world_manifest) : empty_world_manifest;
    event_source.onJoinResult(payload.client_id,
                              payload.accepted,
                              payload.reason,
                              world_state ? world_state->world_name : std::string_view{},
                              world_state ? world_state->world_id : std::string_view{},
                              world_state ? world_state->world_revision : std::string_view{},
                              world_state ? world_state->world_package_hash : std::string_view{},
                              world_state ? world_state->world_content_hash : std::string_view{},
                              world_state ? world_state->world_manifest_hash : std::string_view{},
                              world_state ? world_state->world_manifest_file_count : 0U,
                              world_state ? world_state->world_package_size : 0U,
                              world_state ? world_state->world_dir : std::filesystem::path{},
                              net_sessions,
                              net_world_manifest,
                              *world_package);
}

} // namespace

int RunRuntime(const karma::cli::ServerAppOptions& options) {
    g_app_name = options.app_name.empty() ? std::string("server") : options.app_name;
    const auto issues = karma::config::ValidateRequiredKeys(karma::config::ServerRequiredKeys());
    if (!karma::app::ReportRequiredConfigIssues(issues, options.strict_config)) {
        return 1;
    }

    const auto world_context = domain::LoadWorldSessionContext(options);
    if (!world_context.has_value()) {
        return 1;
    }

    std::signal(SIGINT, OnSignal);
    std::signal(SIGTERM, OnSignal);

    g_running.store(true);
    karma::app::EngineServerConfig engineConfig{};
    engineConfig.target_tick_hz = karma::config::ReadFloatConfig({"simulation.fixedHz"}, engineConfig.target_tick_hz);
    engineConfig.max_delta_time = karma::config::ReadFloatConfig({"simulation.maxFrameDeltaTime"},
                                                                  engineConfig.max_delta_time);
    engineConfig.max_substeps =
        static_cast<int>(karma::config::ReadUInt16Config({"simulation.maxSubsteps"},
                                                          static_cast<uint16_t>(engineConfig.max_substeps)));
    engineConfig.physics_backend =
        karma::app::ResolvePhysicsBackendFromOption(options.backend_physics, options.backend_physics_explicit);
    engineConfig.audio_backend =
        karma::app::ResolveAudioBackendFromOption(options.backend_audio, options.backend_audio_explicit);
    engineConfig.enable_audio = options.backend_audio_explicit
        || karma::config::ReadBoolConfig({"audio.serverEnabled"}, false);
    const uint16_t listen_port = karma::cli::ResolveServerListenPort(options.listen_port,
                                                                      options.listen_port_explicit,
                                                                      static_cast<uint16_t>(11899));
    karma::network::ServerPreAuthConfig pre_auth_config = karma::network::ReadServerPreAuthConfig();

    karma::network::CommunityHeartbeat community_heartbeat{};
    const std::string community_override = options.community_explicit ? options.community : std::string{};
    community_heartbeat.configureFromConfig(karma::config::ConfigStore::Merged(),
                                            listen_port,
                                            community_override);
    if (community_heartbeat.enabled()) {
        spdlog::info("Community heartbeat enabled: target='{}' advertise='{}' interval={}s max_players={}",
                     community_heartbeat.communityUrl(),
                     community_heartbeat.serverAddress(),
                     community_heartbeat.intervalSeconds(),
                     community_heartbeat.maxPlayers());
    } else {
        spdlog::info("Community heartbeat disabled: target='{}' advertise='{}' interval={}s",
                     community_heartbeat.communityUrl(),
                     community_heartbeat.serverAddress(),
                     community_heartbeat.intervalSeconds());
    }
    pre_auth_config.community_url = community_heartbeat.communityUrl();
    pre_auth_config.world_name = world_context->world_name;
    KARMA_TRACE("engine.server",
                "Server pre-auth {} (community_auth={})",
                pre_auth_config.required_password.empty() ? "disabled" : "enabled",
                pre_auth_config.community_url.empty() ? "disabled" : "enabled");

    ServerGame game{world_context->world_name};
    std::unique_ptr<net::ServerEventSource> event_source = net::CreateServerEventSource(options, listen_port);
    domain::ShotSystem shot_system{};
    const float shot_lifetime_seconds =
        std::max(0.1f, karma::config::ReadFloatConfig({"gameplay.shotLifetimeSeconds"}, 5.0f));
    shot_system.setLifetime(std::chrono::milliseconds(static_cast<int64_t>(shot_lifetime_seconds * 1000.0f)));
    const float shot_step_dt =
        (engineConfig.target_tick_hz > 1e-6f) ? (1.0f / engineConfig.target_tick_hz) : (1.0f / 60.0f);
    uint32_t next_global_shot_id = 1;
    karma::network::ServerSessionHooks session_hooks{};
    session_hooks.on_join = [&game](const karma::network::ServerPreAuthRequest& request) {
        return karma::network::ServerSessionJoinDecision{
            .accepted = game.onClientJoin(request.client_id, request.player_name),
            .reject_reason = std::string{}};
    };
    session_hooks.has_client = [&game](uint32_t client_id) {
        return game.hasClient(client_id);
    };
    session_hooks.on_leave = [&game](uint32_t client_id) {
        return game.onClientLeave(client_id);
    };
    session_hooks.last_join_reject_reason = [&game]() {
        return game.lastJoinRejectReason();
    };

    RuntimeEventRuleHandlers runtime_handlers{};
    runtime_handlers.has_client = session_hooks.has_client;
    runtime_handlers.on_player_death = [event_source_ptr = event_source.get()](uint32_t client_id) {
        event_source_ptr->onPlayerDeath(client_id);
    };
    runtime_handlers.on_player_spawn = [event_source_ptr = event_source.get()](uint32_t client_id) {
        event_source_ptr->onPlayerSpawn(client_id);
    };
    runtime_handlers.on_create_shot =
        [event_source_ptr = event_source.get(), &shot_system](uint32_t source_client_id,
                                                              uint32_t global_shot_id,
                                                              float pos_x,
                                                              float pos_y,
                                                              float pos_z,
                                                              float vel_x,
                                                              float vel_y,
                                                              float vel_z) {
            shot_system.addShot(source_client_id,
                                global_shot_id,
                                pos_x,
                                pos_y,
                                pos_z,
                                vel_x,
                                vel_y,
                                vel_z);
            event_source_ptr->onCreateShot(source_client_id,
                                           global_shot_id,
                                           pos_x,
                                           pos_y,
                                           pos_z,
                                           vel_x,
                                           vel_y,
                                           vel_z);
        };
    karma::app::EngineServerApp app{};
    app.start(game, engineConfig);

    std::vector<karma::network::ServerWorldManifestEntry> world_manifest{};
    world_manifest.reserve(world_context->world_manifest.size());
    for (const auto& entry : world_context->world_manifest) {
        world_manifest.push_back(karma::network::ServerWorldManifestEntry{
            .path = entry.path,
            .size = entry.size,
            .hash = entry.hash});
    }
    static const std::vector<std::byte> empty_world_package{};
    const auto* world_package = world_context->world_package_enabled ? &world_context->world_package : &empty_world_package;
    const karma::network::ServerJoinWorldState world_state{
        .world_name = world_context->world_name,
        .world_id = world_context->world_id,
        .world_revision = world_context->world_revision,
        .world_package_hash = world_context->world_package_hash,
        .world_content_hash = world_context->world_content_hash,
        .world_manifest_hash = world_context->world_manifest_hash,
        .world_manifest_file_count = world_context->world_manifest_file_count,
        .world_package_size = world_context->world_package_size,
        .world_dir = world_context->world_dir,
        .world_manifest = &world_manifest,
        .world_package = world_package};

    while (app.isRunning()) {
        for (const auto& event : event_source->poll()) {
            switch (event.type) {
                case net::ServerInputEvent::Type::ClientJoin: {
                    const auto join_decision = karma::network::ResolveServerSessionJoinDecision(
                        pre_auth_config,
                        karma::network::ServerPreAuthRequest{
                            event.join.client_id,
                            event.join.player_name,
                            event.join.auth_payload,
                            event.join.peer_ip,
                            event.join.peer_port},
                        session_hooks);
                    const bool accepted = join_decision.accepted;
                    const std::string reason = join_decision.reject_reason;
                    if (!accepted) {
                        KARMA_TRACE("engine.server",
                                    "Server join rejected client_id={} name='{}' auth_payload_present={} ip={} port={} reason='{}'",
                                    event.join.client_id,
                                    event.join.player_name,
                                    event.join.auth_payload.empty() ? 0 : 1,
                                    event.join.peer_ip,
                                    event.join.peer_port,
                                    reason);
                    }
                    const auto payload = karma::network::BuildServerJoinResultPayload(
                        event.join.client_id,
                        join_decision,
                        world_state,
                        [&game]() {
                            std::vector<karma::network::ServerSessionSnapshotEntry> sessions{};
                            for (const auto& session : game.activeSessionSnapshot()) {
                                sessions.push_back(karma::network::ServerSessionSnapshotEntry{
                                    session.session_id,
                                    session.session_name});
                            }
                            return sessions;
                        });
                    karma::network::EmitServerJoinResult(payload,
                                                         [event_source_ptr = event_source.get()](
                                                             const karma::network::ServerJoinResultPayload& emitted) {
                                                             EmitJoinResultToEventSource(*event_source_ptr, emitted);
                                                         });
                    break;
                }
                case net::ServerInputEvent::Type::ClientLeave: {
                    const auto leave_event_result = karma::network::ApplyServerSessionLeaveEvent(
                        event.leave.client_id,
                        session_hooks,
                        [event_source_ptr = event_source.get()](uint32_t client_id) {
                            event_source_ptr->onPlayerDeath(client_id);
                        });
                    if (leave_event_result != karma::network::ServerSessionLeaveEventResult::Applied) {
                        KARMA_TRACE("net.server",
                                    "RunRuntime: leave {} client_id={}",
                                    karma::network::DescribeServerSessionLeaveEventResult(leave_event_result),
                                    event.leave.client_id);
                    }
                    break;
                }
                case net::ServerInputEvent::Type::ClientRequestSpawn:
                case net::ServerInputEvent::Type::ClientCreateShot: {
                    const auto rule_result =
                        ApplyRuntimeEventRules(event, runtime_handlers, &next_global_shot_id);
                    if (rule_result == RuntimeEventRuleResult::Applied) {
                        break;
                    }

                    if (rule_result == RuntimeEventRuleResult::IgnoredUnknownClient) {
                        if (event.type == net::ServerInputEvent::Type::ClientRequestSpawn) {
                            KARMA_TRACE("net.server",
                                        "RunRuntime: ignoring spawn request for unknown client_id={}",
                                        event.request_spawn.client_id);
                        } else {
                            KARMA_TRACE("net.server",
                                        "RunRuntime: ignoring create_shot for unknown client_id={} local_shot_id={}",
                                        event.create_shot.client_id,
                                        event.create_shot.local_shot_id);
                        }
                        break;
                    }

                    KARMA_TRACE("net.server",
                                "RunRuntime: runtime event rule not applied type={} result={}",
                                static_cast<int>(event.type),
                                static_cast<int>(rule_result));
                    break;
                }
            }
        }
        app.tick();
        const auto expired_shots = shot_system.update(domain::ShotSystem::Clock::now(), shot_step_dt);
        if (!expired_shots.empty()) {
            for (const auto& expired : expired_shots) {
                event_source->onRemoveShot(expired.global_shot_id, true);
            }
            KARMA_TRACE("net.server",
                        "RunRuntime: expired shots removed count={} remaining={}",
                        expired_shots.size(),
                        shot_system.activeShotCount());
        }
        community_heartbeat.update(game.connectedClientCount());
        if (!g_running.load()) {
            app.requestStop();
        }
    }

    return 0;
}

} // namespace bz3::server
