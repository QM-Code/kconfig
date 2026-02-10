#pragma once

#include "karma/renderer/types.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace karma::renderer_backend::detail {

struct ResolvedMaterialSemantics {
    glm::vec4 base_color{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec3 emissive{0.0f, 0.0f, 0.0f};
    float metallic = 0.0f;
    float roughness = 1.0f;
    renderer::MaterialAlphaMode alpha_mode = renderer::MaterialAlphaMode::Opaque;
    float alpha_cutoff = 0.5f;
    bool double_sided = false;
    bool alpha_blend = false;
    bool draw = true;
    bool used_metallic_roughness_texture = false;
    bool used_emissive_texture = false;
};

inline float Clamp01(float value, float fallback = 0.0f) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, 0.0f, 1.0f);
}

inline float ClampPositive(float value, float fallback = 0.0f, float max_value = 8.0f) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, 0.0f, max_value);
}

inline std::size_t ResolveTextureChannels(const renderer::MeshData::TextureData& texture) {
    if (texture.channels > 0) {
        return static_cast<std::size_t>(texture.channels);
    }
    if (texture.width <= 0 || texture.height <= 0) {
        return 0;
    }
    const std::size_t pixel_count =
        static_cast<std::size_t>(texture.width) * static_cast<std::size_t>(texture.height);
    if (pixel_count == 0) {
        return 0;
    }
    const std::size_t inferred_channels = texture.pixels.size() / pixel_count;
    return inferred_channels;
}

inline bool IsUsableTexture(const renderer::MeshData::TextureData& texture) {
    if (texture.width <= 0 || texture.height <= 0 || texture.pixels.empty()) {
        return false;
    }
    const std::size_t channels = ResolveTextureChannels(texture);
    if (channels == 0) {
        return false;
    }
    const std::size_t required =
        static_cast<std::size_t>(texture.width) * static_cast<std::size_t>(texture.height) * channels;
    return texture.pixels.size() >= required;
}

inline glm::vec4 SampleOriginTexel(const renderer::MeshData::TextureData& texture) {
    if (!IsUsableTexture(texture)) {
        return glm::vec4(1.0f);
    }
    const std::size_t channels = ResolveTextureChannels(texture);
    const auto get_channel = [&](std::size_t index) -> float {
        const std::size_t clamped_index = std::min(index, channels - 1);
        const uint8_t byte_value = texture.pixels[clamped_index];
        return static_cast<float>(byte_value) / 255.0f;
    };
    const float r = get_channel(0);
    const float g = get_channel(1);
    const float b = get_channel(2);
    const float a = (channels > 3) ? get_channel(3) : 1.0f;
    return glm::vec4(r, g, b, a);
}

inline ResolvedMaterialSemantics ResolveMaterialSemantics(const renderer::MaterialDesc& material) {
    ResolvedMaterialSemantics semantics{};
    semantics.alpha_mode = material.alpha_mode;
    semantics.double_sided = material.double_sided;

    semantics.base_color.r = ClampPositive(material.base_color.r, 1.0f, 4.0f);
    semantics.base_color.g = ClampPositive(material.base_color.g, 1.0f, 4.0f);
    semantics.base_color.b = ClampPositive(material.base_color.b, 1.0f, 4.0f);
    semantics.base_color.a = Clamp01(material.base_color.a, 1.0f);

    semantics.emissive.r = ClampPositive(material.emissive_color.r, 0.0f, 8.0f);
    semantics.emissive.g = ClampPositive(material.emissive_color.g, 0.0f, 8.0f);
    semantics.emissive.b = ClampPositive(material.emissive_color.b, 0.0f, 8.0f);

    semantics.metallic = Clamp01(material.metallic_factor, 0.0f);
    semantics.roughness = std::clamp(Clamp01(material.roughness_factor, 1.0f), 0.04f, 1.0f);
    semantics.alpha_cutoff = Clamp01(material.alpha_cutoff, 0.5f);

    if (material.metallic_roughness && IsUsableTexture(*material.metallic_roughness)) {
        const glm::vec4 mr = SampleOriginTexel(*material.metallic_roughness);
        semantics.metallic = Clamp01(semantics.metallic * mr.b, 0.0f);
        semantics.roughness = std::clamp(Clamp01(semantics.roughness * mr.g, semantics.roughness), 0.04f, 1.0f);
        semantics.used_metallic_roughness_texture = true;
    }

    if (material.emissive && IsUsableTexture(*material.emissive)) {
        const glm::vec4 emissive_sample = SampleOriginTexel(*material.emissive);
        semantics.emissive.r = ClampPositive(semantics.emissive.r * emissive_sample.r, semantics.emissive.r, 8.0f);
        semantics.emissive.g = ClampPositive(semantics.emissive.g * emissive_sample.g, semantics.emissive.g, 8.0f);
        semantics.emissive.b = ClampPositive(semantics.emissive.b * emissive_sample.b, semantics.emissive.b, 8.0f);
        semantics.used_emissive_texture = true;
    }

    float effective_alpha = semantics.base_color.a;
    if (material.albedo && IsUsableTexture(*material.albedo)) {
        const glm::vec4 albedo_sample = SampleOriginTexel(*material.albedo);
        effective_alpha = Clamp01(effective_alpha * albedo_sample.a, effective_alpha);
    }

    switch (semantics.alpha_mode) {
        case renderer::MaterialAlphaMode::Mask:
            semantics.draw = (effective_alpha >= semantics.alpha_cutoff);
            semantics.alpha_blend = false;
            semantics.base_color.a = 1.0f;
            break;
        case renderer::MaterialAlphaMode::Blend:
            semantics.draw = (effective_alpha > 1e-4f);
            semantics.alpha_blend = true;
            semantics.base_color.a = effective_alpha;
            break;
        case renderer::MaterialAlphaMode::Opaque:
        default:
            semantics.draw = true;
            semantics.alpha_blend = false;
            semantics.base_color.a = 1.0f;
            break;
    }

    return semantics;
}

inline bool ValidateResolvedMaterialSemantics(const ResolvedMaterialSemantics& semantics) {
    const auto finite_vec3 = [](const glm::vec3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };
    const auto finite_vec4 = [](const glm::vec4& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.w);
    };
    if (!finite_vec4(semantics.base_color) || !finite_vec3(semantics.emissive)) {
        return false;
    }
    if (!std::isfinite(semantics.metallic) || !std::isfinite(semantics.roughness) || !std::isfinite(semantics.alpha_cutoff)) {
        return false;
    }
    if (semantics.metallic < 0.0f || semantics.metallic > 1.0f) {
        return false;
    }
    if (semantics.roughness < 0.04f || semantics.roughness > 1.0f) {
        return false;
    }
    if (semantics.base_color.a < 0.0f || semantics.base_color.a > 1.0f) {
        return false;
    }
    if (semantics.alpha_cutoff < 0.0f || semantics.alpha_cutoff > 1.0f) {
        return false;
    }
    return true;
}

} // namespace karma::renderer_backend::detail
