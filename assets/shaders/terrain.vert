#version 450
#extension GL_ARB_separate_shader_objects : enable

// Global information
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

// per model information
layout (set = 2, binding = 0) uniform ModelMatrixData
{
	mat4 model;
	mat4 normal;
}
mnd;

layout (set = 2, binding = 1) uniform sampler2D texHeightMap;

// layout (push_constant) uniform Bound
//{
//	vec2 pos_tl;
//	vec2 pos_br;
//	vec2 uv_tl;
//	vec2 uv_br;
//	float height_scale;
//	float width_scale;
//}
// bounds;



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
	gl_Position = cam.projView * vec4 (new_pos, 1.0);


	outTexCoord = new_uv;
	outNormal = inNormal;
	outFragPos = (vec4 (new_pos, 1.0)).xyz;
}