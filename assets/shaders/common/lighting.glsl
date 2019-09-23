#extension GL_GOOGLE_include_directive : enable

#include "pbr_mat.glsl"

const int DirectionalLightCount = 5;
const int PointLightCount = 10;
const int SpotLightCount = 10;

struct DirectionalLight
{
	vec3 direction;
	vec3 color;
	float intensity;
};

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
	float cutOff;
};

struct SpotLight
{
	vec3 position;
	vec3 color;
	float attenuation;
	float cutoff;
	float outerCutOff;
};

layout (set = 1, binding = 0) uniform DirectionalLightData { DirectionalLight lights[1]; }
directional;

layout (set = 1, binding = 1) uniform PointLightData { PointLight lights[PointLightCount]; }
point;

layout (set = 1, binding = 2) uniform SpotLightData { SpotLight lights[SpotLightCount]; }
spot;


vec3 DirectionalLightingCalc (
    DirectionalLight dl, vec3 N, vec3 V, vec3 F0, vec3 albedo, float roughness, float metalness)
{
	// calculate per-light radiance
	vec3 L = normalize (dl.direction);
	vec3 H = normalize (V + L);
	vec3 radiance = dl.color * dl.intensity;
	return PBR_Calc (N, V, F0, L, H, radiance, albedo, roughness, metalness);
}

vec3 PointLightingCalc (PointLight pl, vec3 N, vec3 V, vec3 F0, vec3 Pos, vec3 albedo, float roughness, float metalness)
{
	// calculate per-light radiance
	vec3 L = normalize (pl.position - Pos);
	vec3 H = normalize (V + L);
	float separation = distance (pl.position, Pos);
	float attenuation = 1.0 / (separation * separation);
	vec3 radiance = pl.color * pl.intensity * attenuation;
	return PBR_Calc (N, V, F0, L, H, radiance, albedo, roughness, metalness);
}

vec3 LightingContribution (vec3 N, vec3 V, vec3 F0, vec3 Pos, vec3 albedo, float roughness, float metalness)
{
	vec3 Lo = vec3 (0.0);
	for (int i = 0; i < DirectionalLightCount; ++i)
	{
		Lo += DirectionalLightingCalc (directional.lights[i], N, V, F0, albedo, roughness, metalness);
	}

	for (int i = 0; i < PointLightCount; ++i)
	{
		Lo += PointLightingCalc (point.lights[i], N, V, F0, Pos, albedo, roughness, metalness);
	}

	return Lo;
}