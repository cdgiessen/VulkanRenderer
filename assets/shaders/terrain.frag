#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "globals.glsl"

#include "camera.glsl"

#include "lighting.glsl"

// texture sampling
layout (set = 2, binding = 1) uniform sampler2D texHeightMap;
layout (set = 2, binding = 2) uniform sampler2D texNormalMap;
layout (set = 2, binding = 3) uniform sampler2D texSplatMap;
layout (set = 2, binding = 4) uniform sampler2DArray texArrayAlbedo;
layout (set = 2, binding = 5) uniform sampler2DArray texArrayRoughness;
layout (set = 2, binding = 6) uniform sampler2DArray texArrayMetalness;
layout (set = 2, binding = 7) uniform sampler2DArray texArrayNormal;

layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

vec3 perturbNormal (vec3 texNormal)
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
	// vec4 texColor = inColor; //splatmap not in yet, so just use vertex colors until then
	vec4 texColor = texture (texSplatMap, inTexCoord);
	float texSampleDensity = 500.0f;

	vec4 albedo1 = texture (
	    texArrayAlbedo, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0));
	vec4 albedo2 = texture (
	    texArrayAlbedo, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1));
	vec4 albedo3 = texture (
	    texArrayAlbedo, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2));
	vec4 albedo4 = texture (
	    texArrayAlbedo, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3));
	vec3 albedo = vec3 (
	    albedo1 * texColor.x + albedo2 * texColor.y + albedo3 * texColor.z + albedo4 * texColor.w);
	albedo = pow (albedo, vec3 (2.2));

	float roughness1 = float(texture (texArrayRoughness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0)));
	float roughness2 = float(texture (texArrayRoughness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1)));
	float roughness3 = float(texture (texArrayRoughness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2)));
	float roughness4 = float(texture (texArrayRoughness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3)));
	float roughness = (roughness1 * texColor.x + roughness2 * texColor.y + roughness3 * texColor.z +
	                   roughness4 * texColor.w);

	float metalness1 = float(texture (texArrayMetalness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0)));
	float metalness2 = float(texture (texArrayMetalness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1)));
	float metalness3 = float(texture (texArrayMetalness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2)));
	float metalness4 = float(texture (texArrayMetalness,
	    vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3)));
	float metalness = (metalness1 * texColor.x + metalness2 * texColor.y + metalness3 * texColor.z +
	                   metalness4 * texColor.w);

	vec3 normal1 = vec3 (texture (
	    texArrayNormal, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0)));
	vec3 normal2 = vec3 (texture (
	    texArrayNormal, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1)));
	vec3 normal3 = vec3 (texture (
	    texArrayNormal, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2)));
	vec3 normal4 = vec3 (texture (
	    texArrayNormal, vec3 (inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3)));
	vec3 normal = normalize (
	    normal1 * texColor.x + normal2 * texColor.y + normal3 * texColor.z + normal4 * texColor.w);

	vec3 viewVec = normalize (-cam.camera_dir);

	// vec3 normalVec = normalize(inNormal);

	// vec3 pointLightContrib = vec3(0.0f);
	// for(int i = 0; i < PointLightCount; i++){
	//	vec3 lightVec = normalize(point.lights[i].position - inFragPos).xyz;
	//	vec3 reflectVec = reflect(-lightVec, normalVec);
	//	vec3 halfwayVec = normalize(viewVec + point.lights[i].position);

	//	float separation = distance(point.lights[i].position, inFragPos);
	//	float attenuation = 1.0f/(separation*separation);

	//	vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(1.0f) * attenuation *
	// point.lights[i].color; 	vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) *
	// vec3(0.75f)* attenuation * point.lights[i].color; 	pointLightContrib += (diffuse + specular);
	//}

	// vec3 dirContrib = DirPhongLighting(viewVec, sun.light[0].direction, normalVec,
	// sun.light[0].color, sun.light[0].intensity); vec3 N = normalize(inNormal);
	vec3 N = perturbNormal (normal);
	vec3 V = normalize (cam.camera_pos - inFragPos);
	vec3 F0 = vec3 (0.04);
	F0 = mix (F0, albedo, metalness);
	vec3 ambient = vec3 (0.002);
	vec3 lighting = ambient + LightingContribution (N, V, F0, inFragPos, albedo, roughness, metalness);

	float belowWaterLevelDarkening = -1 * (((-clamp (inFragPos.y, -7, -0.5) - 0.5) / 5) - 1);


	// outColor = (vec4(0,0,0,0) + inColor) * vec4((pointLightContrib + dirContrib), 1.0f);
	// outColor = vec4(lighting, 1.0f);
	// outColor = vec4(pointLightContrib * inColor, 1.0f);
	vec3 color = lighting / (lighting + vec3 (1.0));
	outColor = vec4 (pow (color, vec3 (1.0 / 2.2)), 1.0f);

	// float r = outColor.r * belowWaterLevelDarkening;
	// float g = outColor.g * belowWaterLevelDarkening;
	// float b = outColor.b * clamp (belowWaterLevelDarkening * 0.8, 0, 1);
	//
	//
	// outColor = vec4 (r, g, b, 1.0f);
}
