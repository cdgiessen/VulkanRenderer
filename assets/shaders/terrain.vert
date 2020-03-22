#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "globals.glsl"

#include "camera.glsl"

// per model information
layout (set = 2, binding = 0) uniform ModelMatrixData
{
	mat4 model;
	mat4 normal;
}
mnd;

// height and normal map
layout (set = 2, binding = 1) uniform sampler2D texHeightMap;
layout (set = 2, binding = 2) uniform sampler2D texNormalMap;


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

// Instanced attributes
layout (location = 3) in vec4 pos;
layout (location = 4) in vec4 uv;
layout (location = 5) in vec4 h_w_pp;
layout (location = 6) in vec4 dumb;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outTexCoord;

out gl_PerVertex { vec4 gl_Position; };

void main ()
{
	vec3 new_pos = inPosition;

	new_pos.x = mix (pos.x, pos.z, inPosition.x);
	new_pos.z = mix (pos.y, pos.w, inPosition.z);

	vec2 new_uv = mix (uv.xy, uv.zw, vec2 (inPosition.x, inPosition.z));
	float height = texture (texHeightMap, new_uv.yx).r;

	new_pos.y = height * h_w_pp.x;
	gl_Position = cam.proj_view * vec4 (new_pos, 1.0);

	vec3 new_norm = texture (texNormalMap, new_uv.yx).rgb;

	outTexCoord = new_uv;
	outNormal = new_norm;
	outFragPos = (vec4 (new_pos, 1.0)).xyz;
}