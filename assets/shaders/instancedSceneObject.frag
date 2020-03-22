#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "globals.glsl"

#include "camera.glsl"

#include "lighting.glsl"

// texture sampling
layout (set = 2, binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

// vec3 DirPhongLighting(vec3 view, vec3 dir, vec3 normal, vec3 color, float intensity) {
//	vec3 light = normalize(dir);
//	vec3 halfway = normalize(light + view);
//	vec3 reflect = reflect(-light, normal);
//	vec3 diffuse = max(dot(normal, light), 0.0f)* vec3(0.8f);
//	vec3 specular = pow(max(dot(view, reflect), 0.0), 16.0f)* vec3(0.15f);
//	vec3 contrib = (diffuse + specular)* vec3(intensity) * color;

//	return contrib;
//}



void main ()
{
	vec4 texColor = texture (texSampler, inTexCoord);


	vec3 N = normalize (inNormal);
	vec3 V = normalize (cam.camera_pos - inFragPos);

	vec3 F0 = vec3 (0.04);
	F0 = mix (F0, vec3 (texColor) /*pbr_mat.albedo*/, 0.1f /*pbr_mat.metallic*/);

	vec3 ambient = vec3 (0.0);
	vec3 lighting = ambient + LightingContribution (N, V, F0, inFragPos, ambient, 0, 0);

	// vec3 viewVec = normalize(-cam.camera_dir);
	//
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

	// vec3 dirContrib = DirPhongLighting(viewVec, sun.light[0].direction, normalVec, sun.light[0].color, sun.light[0].intensity);

	// outColor = texColor * vec4(pointLightContrib * inColor, 1.0f);
	outColor = texColor * vec4 (lighting, 1.0f);
}