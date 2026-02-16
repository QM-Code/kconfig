$input v_texcoord0, v_normal, v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_color;
uniform vec4 u_lightDir;
uniform vec4 u_lightColor;
uniform vec4 u_ambientColor;
uniform vec4 u_unlit;
uniform vec4 u_textureMode;
uniform vec4 u_shadowParams0;
uniform vec4 u_shadowParams1;
uniform vec4 u_shadowParams2;
uniform vec4 u_shadowBiasParams; // receiver_scale, normal_scale, raster_depth_bias, raster_slope_bias
uniform vec4 u_shadowAxisRight;
uniform vec4 u_shadowAxisUp;
uniform vec4 u_shadowAxisForward;
uniform vec4 u_localLightCount;
uniform vec4 u_localLightParams; // distance_damping, range_exponent, ao_affects_local, directional_shadow_lift
uniform vec4 u_localLightPosRange[4];
uniform vec4 u_localLightColorIntensity[4];
uniform vec4 u_localLightShadowSlot[4];
uniform vec4 u_pointShadowParams; // enabled, face_texel_size, pcf_radius, active_slots
uniform vec4 u_pointShadowAtlasTexel; // atlas_inv_width, atlas_inv_height
uniform vec4 u_pointShadowTuning; // constant, slope_scale, normal_scale, receiver_scale
uniform mat4 u_pointShadowUvProj[24];
SAMPLER2D(s_tex, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_occlusion, 2);
SAMPLER2D(s_shadow, 3);
SAMPLER2D(s_pointShadow, 4);

float sampleShadowVisibility(vec3 worldPos, float ndotl) {
    if (u_shadowParams0.x < 0.5) {
        return 1.0;
    }
    if (ndotl <= 0.0001) {
        return 1.0;
    }

    float extent = max(u_shadowParams1.x, 0.001);
    float centerX = u_shadowParams1.y;
    float centerY = u_shadowParams1.z;
    float minDepth = u_shadowParams1.w;
    float invDepthRange = max(u_shadowParams2.x, 0.0001);
    float radius = clamp(u_shadowParams2.y, 0.0, 4.0);
    float invMapSize = max(u_shadowParams0.w, 0.0);
    float slope = clamp(1.0 - ndotl, 0.0, 1.0);
    float bias = clamp(
        u_shadowParams0.z +
            (u_shadowParams0.w * (u_shadowBiasParams.x + (u_shadowBiasParams.y * slope))),
        0.0,
        0.02);

    float lx = dot(worldPos, u_shadowAxisRight.xyz);
    float ly = dot(worldPos, u_shadowAxisUp.xyz);
    float lz = dot(worldPos, u_shadowAxisForward.xyz);
    float u = 0.5 + ((lx - centerX) / (2.0 * extent));
    float v = 0.5 + ((ly - centerY) / (2.0 * extent));
    float depth = (lz - minDepth) * invDepthRange;

    if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0 || depth < 0.0 || depth > 1.0) {
        return 1.0;
    }

    int iradius = int(clamp(floor(radius + 0.5), 0.0, 4.0));
    if (iradius <= 0) {
        float mapDepth = texture2D(s_shadow, vec2(u, v)).r;
        return step(depth - bias, mapDepth);
    }

    float lit = 0.0;
    float count = 0.0;
    if (iradius == 1) {
        for (int oy = -1; oy <= 1; ++oy) {
            for (int ox = -1; ox <= 1; ++ox) {
                vec2 uv = vec2(u, v) + vec2(float(ox), float(oy)) * invMapSize;
                float mapDepth = texture2D(s_shadow, uv).r;
                float w = 1.0 / (1.0 + float((ox * ox) + (oy * oy)));
                lit += step(depth - bias, mapDepth) * w;
                count += w;
            }
        }
    } else if (iradius == 2) {
        for (int oy = -2; oy <= 2; ++oy) {
            for (int ox = -2; ox <= 2; ++ox) {
                vec2 uv = vec2(u, v) + vec2(float(ox), float(oy)) * invMapSize;
                float mapDepth = texture2D(s_shadow, uv).r;
                float w = 1.0 / (1.0 + float((ox * ox) + (oy * oy)));
                lit += step(depth - bias, mapDepth) * w;
                count += w;
            }
        }
    } else if (iradius == 3) {
        for (int oy = -3; oy <= 3; ++oy) {
            for (int ox = -3; ox <= 3; ++ox) {
                vec2 uv = vec2(u, v) + vec2(float(ox), float(oy)) * invMapSize;
                float mapDepth = texture2D(s_shadow, uv).r;
                float w = 1.0 / (1.0 + float((ox * ox) + (oy * oy)));
                lit += step(depth - bias, mapDepth) * w;
                count += w;
            }
        }
    } else {
        for (int oy = -4; oy <= 4; ++oy) {
            for (int ox = -4; ox <= 4; ++ox) {
                vec2 uv = vec2(u, v) + vec2(float(ox), float(oy)) * invMapSize;
                float mapDepth = texture2D(s_shadow, uv).r;
                float w = 1.0 / (1.0 + float((ox * ox) + (oy * oy)));
                lit += step(depth - bias, mapDepth) * w;
                count += w;
            }
        }
    }
    if (count < 0.5) {
        return 1.0;
    }
    return lit / count;
}

int selectPointShadowFace(vec3 dirWs) {
    vec3 a = abs(dirWs);
    if (a.x >= a.y && a.x >= a.z) {
        return dirWs.x >= 0.0 ? 0 : 1;
    }
    if (a.y >= a.x && a.y >= a.z) {
        return dirWs.y >= 0.0 ? 2 : 3;
    }
    return dirWs.z >= 0.0 ? 4 : 5;
}

float samplePointShadowVisibility(int localIndex, vec3 worldPos, vec3 geomNormal, vec3 localDir) {
    if (u_pointShadowParams.x < 0.5 || localIndex < 0 || localIndex >= 4) {
        return 1.0;
    }

    int shadowSlot = int(floor(u_localLightShadowSlot[localIndex].x + 0.5));
    int activeSlots = int(clamp(floor(u_pointShadowParams.w + 0.5), 0.0, 4.0));
    if (shadowSlot < 0 || shadowSlot >= activeSlots) {
        return 1.0;
    }

    vec3 toSample = worldPos - u_localLightPosRange[localIndex].xyz;
    int face = selectPointShadowFace(toSample);
    int matrixIndex = (shadowSlot * 6) + face;
    if (matrixIndex < 0 || matrixIndex >= 24) {
        return 1.0;
    }

    float texelSize = max(u_pointShadowParams.y, 0.0);
    float slope = 1.0 - clamp(dot(geomNormal, localDir), 0.0, 1.0);
    float normalWs = texelSize * max(u_pointShadowTuning.z, 0.0) * (0.5 + slope);
    vec3 shadowWorldPos = worldPos + (geomNormal * normalWs);

    vec4 shadowUvDepth = mul(u_pointShadowUvProj[matrixIndex], vec4(shadowWorldPos, 1.0));
    shadowUvDepth.xyz /= max(shadowUvDepth.w, 1e-7);
    vec2 shadowUv = shadowUvDepth.xy;
    float shadowDepth = max(shadowUvDepth.z, 1e-7);
    if (shadowUv.x < 0.0 || shadowUv.x > 1.0 ||
        shadowUv.y < 0.0 || shadowUv.y > 1.0 ||
        shadowDepth < 0.0 || shadowDepth > 1.0) {
        return 1.0;
    }

    float constBias = max(u_pointShadowTuning.x, 0.0);
    float slopeBias = texelSize * max(u_pointShadowTuning.y, 0.0) * (0.4 + slope);
    float receiverBias =
        (abs(dFdx(shadowDepth)) + abs(dFdy(shadowDepth))) * max(u_pointShadowTuning.w, 0.0);
    float bias = clamp(constBias + slopeBias + receiverBias, 0.0, 0.04);

    int iradius = int(clamp(floor(u_pointShadowParams.z + 0.5), 0.0, 1.0));
    if (iradius <= 0) {
        float mapDepth = texture2D(s_pointShadow, shadowUv).r;
        return step(shadowDepth - bias, mapDepth);
    }

    vec2 atlasTexel = max(u_pointShadowAtlasTexel.xy, vec2(0.0, 0.0));
    float lit = 0.0;
    float count = 0.0;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (abs(ox) > iradius || abs(oy) > iradius) {
                continue;
            }
            vec2 uv = shadowUv + vec2(float(ox), float(oy)) * atlasTexel;
            float mapDepth = texture2D(s_pointShadow, uv).r;
            lit += step(shadowDepth - bias, mapDepth);
            count += 1.0;
        }
    }
    return count > 0.5 ? (lit / count) : 1.0;
}

void main() {
    vec4 texColor = texture2D(s_tex, v_texcoord0);
    float normalModulation = 1.0;
    if (u_textureMode.x > 0.5) {
        vec2 encodedNormal = texture2D(s_normal, v_texcoord0).rg * 2.0 - vec2(1.0, 1.0);
        float normalResponse = clamp(length(encodedNormal), 0.0, 1.0);
        normalModulation = clamp(1.0 + (0.16 * max(0.0, normalResponse - 0.18)), 1.0, 1.18);
    }

    float occlusionModulation = 1.0;
    float ao = 1.0;
    if (u_textureMode.y > 0.5) {
        ao = clamp(texture2D(s_occlusion, v_texcoord0).r, 0.0, 1.0);
        occlusionModulation = clamp(0.25 + (0.75 * ao), 0.25, 1.0);
    }

    float combinedModulation = clamp(normalModulation * occlusionModulation, 0.20, 1.20);
    vec4 shadedTexColor = vec4(clamp(texColor.rgb * combinedModulation, vec3(0.0), vec3(1.0)),
                               clamp(texColor.a, 0.0, 1.0));
    vec4 baseColor = u_color * shadedTexColor;
    if (u_unlit.x > 0.5) {
        gl_FragColor = baseColor;
        return;
    }
    vec3 n = normalize(v_normal);
    float ndotl = max(dot(n, normalize(u_lightDir.xyz)), 0.0);
    float shadowVis = ndotl > 0.0001 ? sampleShadowVisibility(v_worldPos, ndotl) : 1.0;
    float shadowFactor = clamp(1.0 - (u_shadowParams0.y * (1.0 - shadowVis)), 0.0, 1.0);
    int localCount = int(clamp(floor(u_localLightCount.x + 0.5), 0.0, 4.0));
    float localDamping = max(u_localLightParams.x, 0.0);
    float localRangeExponent = max(u_localLightParams.y, 0.1);
    bool aoAffectsLocal = u_localLightParams.z > 0.5;
    float localShadowLiftStrength = max(u_localLightParams.w, 0.0);
    vec3 localLightAccum = vec3(0.0, 0.0, 0.0);
    float localShadowLiftEnergy = 0.0;
    for (int i = 0; i < 4; ++i) {
        if (i >= localCount) {
            continue;
        }
        vec3 toLight = u_localLightPosRange[i].xyz - v_worldPos;
        float lightDistance = length(toLight);
        float lightRange = max(u_localLightPosRange[i].w, 0.001);
        if (lightDistance >= lightRange || lightDistance <= 1e-5) {
            continue;
        }
        vec3 localDir = toLight / lightDistance;
        float localNdotL = max(dot(n, localDir), 0.0);
        if (localNdotL <= 0.0) {
            continue;
        }
        float rangeAttenuation = pow(clamp(1.0 - (lightDistance / lightRange), 0.0, 1.0), localRangeExponent);
        float distanceAttenuation = 1.0 / (1.0 + (localDamping * lightDistance * lightDistance));
        float attenuation = rangeAttenuation * distanceAttenuation;
        float aoMod = aoAffectsLocal ? ao : 1.0;
        float pointShadow = samplePointShadowVisibility(i, v_worldPos, n, localDir);
        float localScalar = u_localLightColorIntensity[i].a * attenuation * localNdotL * aoMod * pointShadow;
        localLightAccum += baseColor.rgb * u_localLightColorIntensity[i].rgb * localScalar;
        float localLuminance = dot(u_localLightColorIntensity[i].rgb, vec3(0.2126, 0.7152, 0.0722));
        localShadowLiftEnergy += localLuminance * localScalar;
    }
    float shadowLift = 1.0 - exp(-localShadowLiftEnergy * localShadowLiftStrength);
    float liftedShadowFactor = mix(shadowFactor, 1.0, clamp(shadowLift, 0.0, 1.0));
    vec3 lit = baseColor.rgb * (u_ambientColor.rgb + (u_lightColor.rgb * ndotl * liftedShadowFactor));
    lit += localLightAccum;
    gl_FragColor = vec4(lit, baseColor.a);
}
