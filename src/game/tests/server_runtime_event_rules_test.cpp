#include "server/runtime_event_rules.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct Recorder {
    std::unordered_set<uint32_t> connected_clients{};
    std::vector<uint32_t> death_events{};
    std::vector<uint32_t> spawn_events{};

    struct ShotCall {
        uint32_t source_client_id = 0;
        uint32_t global_shot_id = 0;
        float pos_x = 0.0f;
        float pos_y = 0.0f;
        float pos_z = 0.0f;
        float vel_x = 0.0f;
        float vel_y = 0.0f;
        float vel_z = 0.0f;
    };
    std::vector<ShotCall> shot_events{};

    bool force_leave_fail = false;
    int leave_calls = 0;
};

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

bz3::server::RuntimeEventRuleHandlers BuildHandlers(Recorder* recorder) {
    bz3::server::RuntimeEventRuleHandlers handlers{};
    handlers.has_client = [recorder](uint32_t client_id) {
        return recorder->connected_clients.contains(client_id);
    };
    handlers.on_client_leave = [recorder](uint32_t client_id) {
        ++recorder->leave_calls;
        if (recorder->force_leave_fail) {
            return false;
        }
        recorder->connected_clients.erase(client_id);
        return true;
    };
    handlers.on_player_death = [recorder](uint32_t client_id) {
        recorder->death_events.push_back(client_id);
    };
    handlers.on_player_spawn = [recorder](uint32_t client_id) {
        recorder->spawn_events.push_back(client_id);
    };
    handlers.on_create_shot = [recorder](uint32_t source_client_id,
                                         uint32_t global_shot_id,
                                         float pos_x,
                                         float pos_y,
                                         float pos_z,
                                         float vel_x,
                                         float vel_y,
                                         float vel_z) {
        recorder->shot_events.push_back(Recorder::ShotCall{
            source_client_id,
            global_shot_id,
            pos_x,
            pos_y,
            pos_z,
            vel_x,
            vel_y,
            vel_z});
    };
    return handlers;
}

bz3::server::net::ServerInputEvent MakeLeave(uint32_t client_id) {
    bz3::server::net::ServerInputEvent event{};
    event.type = bz3::server::net::ServerInputEvent::Type::ClientLeave;
    event.leave.client_id = client_id;
    return event;
}

bz3::server::net::ServerInputEvent MakeSpawn(uint32_t client_id) {
    bz3::server::net::ServerInputEvent event{};
    event.type = bz3::server::net::ServerInputEvent::Type::ClientRequestSpawn;
    event.request_spawn.client_id = client_id;
    return event;
}

bz3::server::net::ServerInputEvent MakeShot(uint32_t client_id,
                                            uint32_t local_shot_id,
                                            float pos_x,
                                            float pos_y,
                                            float pos_z,
                                            float vel_x,
                                            float vel_y,
                                            float vel_z) {
    bz3::server::net::ServerInputEvent event{};
    event.type = bz3::server::net::ServerInputEvent::Type::ClientCreateShot;
    event.create_shot.client_id = client_id;
    event.create_shot.local_shot_id = local_shot_id;
    event.create_shot.pos_x = pos_x;
    event.create_shot.pos_y = pos_y;
    event.create_shot.pos_z = pos_z;
    event.create_shot.vel_x = vel_x;
    event.create_shot.vel_y = vel_y;
    event.create_shot.vel_z = vel_z;
    return event;
}

bool TestUnknownLeaveIgnored() {
    Recorder recorder{};
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 1;

    const auto result = bz3::server::ApplyRuntimeEventRules(
        MakeLeave(99),
        handlers,
        &next_global_shot_id);

    return Expect(result == bz3::server::RuntimeEventRuleResult::IgnoredUnknownClient,
                  "unknown leave should be ignored")
           && Expect(recorder.leave_calls == 0, "leave callback should not be called for unknown client")
           && Expect(recorder.death_events.empty(), "death event should not be emitted for unknown leave")
           && Expect(next_global_shot_id == 1, "leave should not affect shot id");
}

bool TestKnownLeaveEmitsDeathOnce() {
    Recorder recorder{};
    recorder.connected_clients.insert(7);
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 10;

    const auto result = bz3::server::ApplyRuntimeEventRules(
        MakeLeave(7),
        handlers,
        &next_global_shot_id);

    return Expect(result == bz3::server::RuntimeEventRuleResult::Applied,
                  "known leave should apply")
           && Expect(recorder.leave_calls == 1, "leave callback should be called once")
           && Expect(recorder.death_events.size() == 1 && recorder.death_events[0] == 7,
                     "death event should be emitted once for leaving client")
           && Expect(!recorder.connected_clients.contains(7), "client should be removed after successful leave")
           && Expect(next_global_shot_id == 10, "leave should not affect shot id");
}

bool TestLeaveFailureDoesNotEmitDeath() {
    Recorder recorder{};
    recorder.connected_clients.insert(11);
    recorder.force_leave_fail = true;
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 5;

    const auto result = bz3::server::ApplyRuntimeEventRules(
        MakeLeave(11),
        handlers,
        &next_global_shot_id);

    return Expect(result == bz3::server::RuntimeEventRuleResult::IgnoredLeaveFailed,
                  "leave failure should produce IgnoredLeaveFailed")
           && Expect(recorder.leave_calls == 1, "leave callback should be called for known client")
           && Expect(recorder.death_events.empty(), "death event should not be emitted when leave fails")
           && Expect(recorder.connected_clients.contains(11),
                     "client should remain connected when leave callback fails")
           && Expect(next_global_shot_id == 5, "leave failure should not affect shot id");
}

bool TestSpawnRules() {
    Recorder recorder{};
    recorder.connected_clients.insert(3);
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 1;

    const auto unknown_result = bz3::server::ApplyRuntimeEventRules(
        MakeSpawn(99),
        handlers,
        &next_global_shot_id);
    const auto known_result = bz3::server::ApplyRuntimeEventRules(
        MakeSpawn(3),
        handlers,
        &next_global_shot_id);

    return Expect(unknown_result == bz3::server::RuntimeEventRuleResult::IgnoredUnknownClient,
                  "unknown spawn should be ignored")
           && Expect(known_result == bz3::server::RuntimeEventRuleResult::Applied,
                     "known spawn should apply")
           && Expect(recorder.spawn_events.size() == 1 && recorder.spawn_events[0] == 3,
                     "known spawn should emit exactly one spawn event")
           && Expect(next_global_shot_id == 1, "spawn should not affect shot id");
}

bool TestShotRulesAndIdMonotonicity() {
    Recorder recorder{};
    recorder.connected_clients.insert(2);
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 100;

    const auto unknown_result = bz3::server::ApplyRuntimeEventRules(
        MakeShot(9, 1, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f),
        handlers,
        &next_global_shot_id);
    const auto first_result = bz3::server::ApplyRuntimeEventRules(
        MakeShot(2, 11, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f),
        handlers,
        &next_global_shot_id);
    const auto second_result = bz3::server::ApplyRuntimeEventRules(
        MakeShot(2, 12, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f),
        handlers,
        &next_global_shot_id);

    return Expect(unknown_result == bz3::server::RuntimeEventRuleResult::IgnoredUnknownClient,
                  "unknown create_shot should be ignored")
           && Expect(first_result == bz3::server::RuntimeEventRuleResult::Applied,
                     "first known create_shot should apply")
           && Expect(second_result == bz3::server::RuntimeEventRuleResult::Applied,
                     "second known create_shot should apply")
           && Expect(recorder.shot_events.size() == 2, "expected exactly two emitted shot events")
           && Expect(recorder.shot_events[0].source_client_id == 2
                         && recorder.shot_events[0].global_shot_id == 100,
                     "first emitted shot should use next_global_shot_id=100")
           && Expect(recorder.shot_events[1].source_client_id == 2
                         && recorder.shot_events[1].global_shot_id == 101,
                     "second emitted shot should use next_global_shot_id=101")
           && Expect(next_global_shot_id == 102, "next_global_shot_id should advance only for applied shot events");
}

bool TestPostLeaveSpawnAndShotIgnored() {
    Recorder recorder{};
    recorder.connected_clients.insert(21);
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 55;

    const auto leave_result = bz3::server::ApplyRuntimeEventRules(
        MakeLeave(21),
        handlers,
        &next_global_shot_id);
    const auto spawn_after_leave = bz3::server::ApplyRuntimeEventRules(
        MakeSpawn(21),
        handlers,
        &next_global_shot_id);
    const auto shot_after_leave = bz3::server::ApplyRuntimeEventRules(
        MakeShot(21, 99, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f),
        handlers,
        &next_global_shot_id);

    return Expect(leave_result == bz3::server::RuntimeEventRuleResult::Applied,
                  "known leave should apply before post-leave checks")
           && Expect(spawn_after_leave == bz3::server::RuntimeEventRuleResult::IgnoredUnknownClient,
                     "spawn after leave should be ignored as unknown client")
           && Expect(shot_after_leave == bz3::server::RuntimeEventRuleResult::IgnoredUnknownClient,
                     "create_shot after leave should be ignored as unknown client")
           && Expect(recorder.leave_calls == 1, "leave callback should be called exactly once")
           && Expect(recorder.death_events.size() == 1 && recorder.death_events[0] == 21,
                     "known leave should emit exactly one death event")
           && Expect(recorder.spawn_events.empty(), "spawn callback should not run after leave")
           && Expect(recorder.shot_events.empty(), "create_shot callback should not run after leave")
           && Expect(next_global_shot_id == 55, "ignored post-leave shot should not advance global shot id");
}

bool TestUnsupportedRuntimeEventType() {
    Recorder recorder{};
    auto handlers = BuildHandlers(&recorder);
    uint32_t next_global_shot_id = 7;

    bz3::server::net::ServerInputEvent event{};
    event.type = bz3::server::net::ServerInputEvent::Type::ClientJoin;
    event.join.client_id = 1;
    event.join.player_name = "ignored";

    const auto result = bz3::server::ApplyRuntimeEventRules(event, handlers, &next_global_shot_id);

    return Expect(result == bz3::server::RuntimeEventRuleResult::UnsupportedEvent,
                  "client join should be unsupported by runtime event rules")
           && Expect(recorder.leave_calls == 0, "unsupported event should not invoke leave callback")
           && Expect(recorder.death_events.empty(), "unsupported event should not emit death events")
           && Expect(recorder.spawn_events.empty(), "unsupported event should not emit spawn events")
           && Expect(recorder.shot_events.empty(), "unsupported event should not emit shot events")
           && Expect(next_global_shot_id == 7, "unsupported event should not mutate shot id");
}

bool TestInvalidHandlers() {
    bz3::server::RuntimeEventRuleHandlers handlers{};
    uint32_t next_global_shot_id = 1;

    const auto leave_result = bz3::server::ApplyRuntimeEventRules(
        MakeLeave(1),
        handlers,
        &next_global_shot_id);
    const auto spawn_result = bz3::server::ApplyRuntimeEventRules(
        MakeSpawn(1),
        handlers,
        &next_global_shot_id);
    const auto shot_result = bz3::server::ApplyRuntimeEventRules(
        MakeShot(1, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
        handlers,
        &next_global_shot_id);

    return Expect(leave_result == bz3::server::RuntimeEventRuleResult::InvalidHandlers,
                  "leave with invalid handlers should return InvalidHandlers")
           && Expect(spawn_result == bz3::server::RuntimeEventRuleResult::InvalidHandlers,
                     "spawn with invalid handlers should return InvalidHandlers")
           && Expect(shot_result == bz3::server::RuntimeEventRuleResult::InvalidHandlers,
                     "shot with invalid handlers should return InvalidHandlers")
           && Expect(next_global_shot_id == 1, "invalid handlers should not mutate shot id");
}

} // namespace

int main() {
    if (!TestUnknownLeaveIgnored()) {
        return 1;
    }
    if (!TestKnownLeaveEmitsDeathOnce()) {
        return 1;
    }
    if (!TestLeaveFailureDoesNotEmitDeath()) {
        return 1;
    }
    if (!TestSpawnRules()) {
        return 1;
    }
    if (!TestShotRulesAndIdMonotonicity()) {
        return 1;
    }
    if (!TestPostLeaveSpawnAndShotIgnored()) {
        return 1;
    }
    if (!TestUnsupportedRuntimeEventType()) {
        return 1;
    }
    if (!TestInvalidHandlers()) {
        return 1;
    }
    return 0;
}
