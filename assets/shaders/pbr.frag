#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "globals.glsl"

#include "camera.glsl"

#include "lighting.glsl"

layout (set = 2, binding = 1) uniform PBR_Material_Constant
{
	vec3 albedo;
	float metallic;
	float roughness;
	float ao;
}
pbr_mat;

// layout(push_constant) uniform PER_OBJECT
//{
//	bool tex_albedo;
//	bool tex_metallic;
//	bool tex_roughness;
//	bool tex_ao;
//	bool tex_normal;
//	bool tex_emissive;
//
//}pc;
//
//
// layout(set = 4, binding = 0) uniform sampler pbr_sampler;
// layout(set = 4, binding = 1) uniform texture2D pbr_tex[4];
// layout(set = 4, binding = 2) uniform texture2D normalMap;



layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;



void main ()
{
	vec3 N = normalize (inNormal);
	vec3 V = normalize (cam.camera_pos - inFragPos);

	vec3 F0 = vec3 (0.04);
	F0 = mix (F0, pbr_mat.albedo, pbr_mat.metallic);

	vec3 ambient = vec3 (0.0) * pbr_mat.albedo * pbr_mat.ao;
	vec3 color = ambient + LightingContribution (
	                           N, V, F0, inFragPos, pbr_mat.albedo, pbr_mat.roughness, pbr_mat.metallic);

	color = color / (color + vec3 (1.0));
	color = pow (color, vec3 (1.0 / 2.2));

	outColor = vec4 (color, 1.0);
}