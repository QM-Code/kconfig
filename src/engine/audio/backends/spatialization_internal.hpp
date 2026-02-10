#pragma once

#include "karma/audio/backend.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace karma::audio_backend::detail {

inline bool IsFiniteVec3(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

inline glm::vec3 NormalizeOr(const glm::vec3& value, const glm::vec3& fallback) {
    const float length_sq = glm::dot(value, value);
    if (!std::isfinite(length_sq) || length_sq <= 1e-12f) {
        return fallback;
    }
    return value / std::sqrt(length_sq);
}

inline ListenerState SanitizeListenerState(const ListenerState& state) {
    ListenerState sanitized = state;
    if (!IsFiniteVec3(sanitized.position)) {
        sanitized.position = {0.0f, 0.0f, 0.0f};
    }
    if (!IsFiniteVec3(sanitized.velocity)) {
        sanitized.velocity = {0.0f, 0.0f, 0.0f};
    }

    sanitized.forward = NormalizeOr(sanitized.forward, {0.0f, 0.0f, -1.0f});

    glm::vec3 up = sanitized.up;
    if (!IsFiniteVec3(up)) {
        up = {0.0f, 1.0f, 0.0f};
    }
    const float projection = glm::dot(up, sanitized.forward);
    up = up - projection * sanitized.forward;
    sanitized.up = NormalizeOr(up, {0.0f, 1.0f, 0.0f});

    const float alignment = std::abs(glm::dot(sanitized.forward, sanitized.up));
    if (!std::isfinite(alignment) || alignment > 0.999f) {
        sanitized.up = {0.0f, 1.0f, 0.0f};
    }
    return sanitized;
}

inline std::optional<glm::vec3> SanitizeEmitterPosition(const std::optional<glm::vec3>& world_position) {
    if (!world_position.has_value()) {
        return std::nullopt;
    }
    if (!IsFiniteVec3(*world_position)) {
        return std::nullopt;
    }
    return *world_position;
}

struct SpatialGains {
    float master = 1.0f;
    float left = 1.0f;
    float right = 1.0f;
};

enum class MultiChannelSpatialRoutingPolicy {
    StereoDerivedFallback
};

inline constexpr MultiChannelSpatialRoutingPolicy kMultiChannelSpatialRoutingPolicy =
    MultiChannelSpatialRoutingPolicy::StereoDerivedFallback;

inline SpatialGains ComputeSpatialGains(const ListenerState& listener,
                                        const std::optional<glm::vec3>& emitter_position) {
    if (!emitter_position.has_value()) {
        return {};
    }

    const glm::vec3 offset = *emitter_position - listener.position;
    const float distance_sq = glm::dot(offset, offset);
    if (!std::isfinite(distance_sq) || distance_sq < 0.0f) {
        return {};
    }

    const float distance = std::sqrt(distance_sq);
    const float attenuation = 1.0f / (1.0f + distance);

    const glm::vec3 right = NormalizeOr(glm::cross(listener.forward, listener.up), {1.0f, 0.0f, 0.0f});

    float pan = 0.0f;
    if (distance > 1e-6f) {
        pan = glm::dot(offset / distance, right);
        pan = std::clamp(pan, -1.0f, 1.0f);
    }

    SpatialGains gains{};
    gains.master = attenuation;
    gains.left = attenuation * std::sqrt(0.5f * (1.0f - pan));
    gains.right = attenuation * std::sqrt(0.5f * (1.0f + pan));
    return gains;
}

inline const char* SpatialRoutingPolicyName() {
    switch (kMultiChannelSpatialRoutingPolicy) {
        case MultiChannelSpatialRoutingPolicy::StereoDerivedFallback:
            return "stereo-derived-fallback";
    }
    return "unknown";
}

inline float StereoDerivedFallbackGain(const SpatialGains& gains) {
    return 0.5f * (gains.left + gains.right);
}

inline float ChannelSpatialGain(const SpatialGains& gains, int channel, int channel_count) {
    if (channel_count <= 1) {
        return gains.master;
    }
    if (channel == 0) {
        return gains.left;
    }
    if (channel == 1) {
        return gains.right;
    }
    switch (kMultiChannelSpatialRoutingPolicy) {
        case MultiChannelSpatialRoutingPolicy::StereoDerivedFallback:
            return StereoDerivedFallbackGain(gains);
    }
    return gains.master;
}

} // namespace karma::audio_backend::detail
