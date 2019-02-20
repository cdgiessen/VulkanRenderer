#version 450
#extension GL_ARB_separate_shader_objects : enable

const int DirectionalLightCount = 5;
const int PointLightCount = 10;
const int SpotLightCount = 10;
const float PI = 3.14159265359;

layout (set = 0, binding = 0) uniform GlobalData { float time; }
global;

layout (set = 0, binding = 1) uniform CameraData
{
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
}
cam;

struct DirectionalLight
{
	vec3 direction;
	float intensity;
	vec3 color;
	float dum;
};

struct PointLight
{
	vec3 position;
	float intensity;
	vec3 color;
};

struct SpotLight
{
	vec3 position;
	float attenuation;
	vec3 color;
	float cutoff;
	float outerCutOff;
	float padding;
};

layout (set = 1, binding = 0) uniform DirectionalLightData
{
	DirectionalLight lights[DirectionalLightCount];
}
directional;

layout (set = 1, binding = 1) uniform PointLightData { PointLight lights[PointLightCount]; }
point;

layout (set = 1, binding = 2) uniform SpotLightData { SpotLight lights[SpotLightCount]; }
spot;

// texture sampling
layout (set = 2, binding = 1) uniform sampler2D water_normal;
layout (set = 2, binding = 2) uniform sampler2D depth;

layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outColor;

vec4 causticsSampler (vec2 uv, float timeIn)
{
#define TAU 6.28318530718
#define MAX_ITER 5


	float time = global.time * .5 + 23.0;
	vec2 p = mod (uv * TAU, TAU) - 250.0;
	vec2 i = vec2 (p);
	float c = 1.0;
	float inten = .005;

	for (int n = 0; n < MAX_ITER; n++)
	{
		float t = time * (1.0 - (3.5 / float(n + 1)));
		i = p + vec2 (cos (t - i.x) + sin (t + i.y), sin (t - i.y) + cos (t + i.x));
		c += 1.0 / length (vec2 (p.x / (sin (i.x + t) / inten), p.y / (cos (i.y + t) / inten)));
	}
	c /= float(MAX_ITER);
	c = 1.17 - pow (c, 1.4);
	vec3 colour = vec3 (pow (abs (c), 8.0));
	colour = clamp (colour + vec3 (0.01, 0.35, 0.5) * 1.3f, 0.0, 1.0);

	return vec4 (colour, 1.0);
}

vec3 fresnelSchlick (float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow (2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float DistributionGGX (vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max (dot (N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX (float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith (vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max (dot (N, V), 0.0);
	float NdotL = max (dot (N, L), 0.0);
	float ggx2 = GeometrySchlickGGX (NdotV, roughness);
	float ggx1 = GeometrySchlickGGX (NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 PBR_Calc (vec3 N, vec3 V, vec3 F0, vec3 L, vec3 H, vec3 radiance)
{
	// cook-torrance brdf
	float NDF = DistributionGGX (N, H, 1.0f /*pbr_mat.roughness*/);
	float G = GeometrySmith (N, V, L, 1.0f /*pbr_mat.roughness*/);
	vec3 F = fresnelSchlick (max (dot (H, V), 0.0), F0);

	vec3 kS = F;
	vec3 kD = vec3 (1.0) - kS;
	kD *= 1.0 - 0.5f /*pbr_mat.metallic*/;

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max (dot (N, V), 0.0) * max (dot (N, L), 0.0);
	vec3 specular = numerator / max (denominator, 0.001);

	// add to outgoing radiance Lo
	float NdotL = max (dot (N, L), 0.0);
	return (kD * vec3 (0.129, 0.404, 1.0f) /*pbr_mat.albedo*/ / PI + specular) * radiance * NdotL;
}

vec3 DirectionalLightingCalc (int i, vec3 N, vec3 V, vec3 F0)
{

	// calculate per-light radiance
	vec3 L = normalize (directional.lights[i].direction);
	vec3 H = normalize (V + L);
	vec3 radiance = directional.lights[i].color * directional.lights[i].intensity;

	return PBR_Calc (N, V, F0, L, H, radiance);
}

vec3 PointLightingCalc (int i, vec3 N, vec3 V, vec3 F0)
{

	// calculate per-light radiance
	vec3 L = normalize (point.lights[i].position - inFragPos);
	vec3 H = normalize (V + L);
	float separation = distance (point.lights[i].position, inFragPos);
	float attenuation = 1.0 / (separation * separation);
	vec3 radiance = point.lights[i].color * point.lights[i].intensity * attenuation;

	return PBR_Calc (N, V, F0, L, H, radiance);
}

vec3 LightingContribution (vec3 N, vec3 V, vec3 F0)
{
	vec3 Lo = vec3 (0.0);
	for (int i = 0; i < DirectionalLightCount; ++i)
	{
		Lo += DirectionalLightingCalc (i, N, V, F0);
	}
	for (int i = 0; i < PointLightCount; ++i)
	{
		Lo += PointLightingCalc (i, N, V, F0);
	}

	return Lo;
}

vec3 perturbNormal (vec3 texNormal, vec2 inTexCoord)
{
	vec3 tangentNormal = texNormal * 2.0 - 1.0;

	vec3 q1 = dFdx (inFragPos);
	vec3 q2 = dFdy (inFragPos);
	vec2 st1 = dFdx (inTexCoord);
	vec2 st2 = dFdy (inTexCoord);

	vec3 N = normalize (inNormal);
	vec3 T = normalize (q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize (cross (N, T));
	mat3 TBN = mat3 (T, B, N);

	return normalize (TBN * tangentNormal);
}

void main ()
{
	float inTime = global.time;
	float time = global.time * 0.1f;

	vec2 sampPoint = inFragPos.xz * 0.25f;

	vec4 lighterBlue = vec4 (87.0 / 255.0, 174.0 / 255.0, 249.0 / 255.0, 1);
	vec4 baseBlue = vec4 (0.129, 0.404, 1.0f, 1.0f);
	// float samp1 = dot(texture(waterTex, vec2(cos(time) + sampPoint.x, cos(time) + sampPoint.y +
	// 0.1f)).xyz, vec3(0.299, 0.587, 0.114)); float samp2 = dot(texture(waterTex, vec2(cos(time*
	// 0.5f) + sampPoint.x + 0.1f, sin(time) + sampPoint.y)).xyz, vec3(0.299, 0.587, 0.114));
	vec4 sampAll = lighterBlue;


	float newTime = inTime * 0.05f;
	vec2 uvModified = vec2 (inFragPos.x + sin (newTime * 1.2f + cos (newTime * 0.35f) * 1.2f) * 1.6f +
	                            sin (newTime * 0.35f) * 1.2f + newTime * 0.8f * (sin (newTime) + 3) + 1.0f,
	    inFragPos.z + cos (newTime * 1.1f + sin (newTime * 0.4f) * 1.6f) * 1.8f +
	        cos (newTime * 0.4f) * 1.6f + newTime * 0.5f * (sin (newTime) + 2) + 1.0f);
	vec2 uvModified2 = vec2 (inFragPos.x + cos (newTime * 1.3f + sin (newTime * 0.45f) * 1.5f) * 1.5f +
	                             sin (newTime * 0.45f) * 1.5f + newTime * 0.5f * (sin (newTime) + 2),
	    inFragPos.z + sin (newTime * 1.4f + cos (newTime * 0.55f) * 1.4f) * 1.4f +
	        cos (newTime * 0.55f) * 1.4f + newTime * 0.8f * (sin (newTime) + 3));
	vec4 texColor = vec4 (0.65) * sampAll + vec4 (0.45) * (causticsSampler (uvModified, inTime * 0.2f) +
	                                                          causticsSampler (uvModified2, inTime * 0.6f));

	vec3 normal = texture (water_normal, -uvModified).xyz;
	vec3 normal2 = texture (water_normal, uvModified2).xyz;

	vec3 N = perturbNormal (normalize (normal + normal2), inFragPos.xz);
	vec3 V = normalize (cam.cameraPos - inFragPos);

	vec3 F0 = vec3 (0.04);
	F0 = mix (F0, vec3 (0.129, 0.404, 1.0f) /*pbr_mat.albedo*/, 0.1f /*pbr_mat.metallic*/);

	vec3 ambient = vec3 (0.0);
	vec3 lighting = ambient + LightingContribution (N, V, F0);

	outColor = texColor * vec4 (lighting, 1.0f);
}
