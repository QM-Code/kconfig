#pragma once

#include "karma/ecs/world.hpp"
#include "karma/physics/physics_system.hpp"
#include "physics/ecs_sync_system.hpp"

#include <memory>
#include <vector>

namespace karma::physics::detail {

enum class EngineFixedStepPhase {
    PreSimulate,
    Simulate,
    PostSimulate
};

struct EngineFixedStepEvent {
    int step_index = 0;
    EngineFixedStepPhase phase = EngineFixedStepPhase::Simulate;
};

class EngineFixedStepObserver {
 public:
    virtual ~EngineFixedStepObserver() = default;
    virtual void onFixedStepEvent(const EngineFixedStepEvent& event) = 0;
};

inline void RecordFixedStepEvent(EngineFixedStepObserver* observer, int step_index, EngineFixedStepPhase phase) {
    if (!observer) {
        return;
    }
    observer->onFixedStepEvent(EngineFixedStepEvent{step_index, phase});
}

inline std::unique_ptr<EcsSyncSystem> CreateEngineSyncIfPhysicsInitialized(PhysicsSystem& physics_system) {
    if (!physics_system.isInitialized()) {
        return {};
    }
    return std::make_unique<EcsSyncSystem>(physics_system);
}

inline void ResetEngineSyncBeforePhysicsShutdown(std::unique_ptr<EcsSyncSystem>& sync_system) {
    if (!sync_system) {
        return;
    }
    sync_system->clear();
    sync_system.reset();
}

inline void SimulateFixedStepsWithSync(PhysicsSystem& physics_system,
                                       EcsSyncSystem* sync_system,
                                       ecs::World& world,
                                       int substep_count,
                                       float fixed_dt,
                                       EngineFixedStepObserver* observer = nullptr) {
    if (substep_count <= 0) {
        return;
    }

    for (int step = 0; step < substep_count; ++step) {
        if (sync_system) {
            RecordFixedStepEvent(observer, step, EngineFixedStepPhase::PreSimulate);
            sync_system->preSimulate(world);
        }

        RecordFixedStepEvent(observer, step, EngineFixedStepPhase::Simulate);
        physics_system.simulateFixedStep(fixed_dt);

        if (sync_system) {
            RecordFixedStepEvent(observer, step, EngineFixedStepPhase::PostSimulate);
            sync_system->postSimulate(world);
        }
    }
}

} // namespace karma::physics::detail
