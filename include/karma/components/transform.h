#pragma once

#include "karma/ecs/component.h"

namespace karma::components {

enum class TransformWriteMode {
  WarnOnPhysics,
  AllowPhysics
};

class TransformComponent : public ecs::ComponentTag {
 public:
  TransformComponent();
  TransformComponent(const glm::vec3& position, const glm::quat& rotation = {},
                     const glm::vec3& scale = {1.0f, 1.0f, 1.0f});

  const glm::vec3& position() const { return position_; }
  const glm::quat& rotation() const { return rotation_; }
  const glm::vec3& scale() const { return scale_; }

  void setPosition(const glm::vec3& position,
                   TransformWriteMode mode = TransformWriteMode::WarnOnPhysics);
  void setRotation(const glm::quat& rotation,
                   TransformWriteMode mode = TransformWriteMode::WarnOnPhysics);
  void setScale(const glm::vec3& scale,
                TransformWriteMode mode = TransformWriteMode::WarnOnPhysics);

  void setHasPhysics(bool has_physics) { has_physics_ = has_physics; }
  void setPhysicsWriteWarning(bool enabled) { warn_on_physics_write_ = enabled; }

 private:
  void warnIfPhysics(const char* action, TransformWriteMode mode) const;

  glm::vec3 position_{};
  glm::quat rotation_{};
  glm::vec3 scale_{1.0f, 1.0f, 1.0f};
  bool has_physics_ = false;
  bool warn_on_physics_write_ = true;
};

}  // namespace karma::components
