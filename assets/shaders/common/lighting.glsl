#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

const int DirectionalLightCount = 5;
const int PointLightCount = 10;
const int SpotLightCount = 10;

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
	vec3 color;
	float attenuation;
	float cutoff;
};

layout (set = 1, binding = 0) uniform DirectionalLightData { DirectionalLight lights[1]; }
directional;

layout (set = 1, binding = 1) uniform PointLightData { PointLight lights[PointLightCount]; }
point;

layout (set = 1, binding = 2) uniform SpotLightData { SpotLight lights[SpotLightCount]; }
spot;