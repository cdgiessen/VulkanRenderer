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

layout (push_constant) uniform Bound
{
	vec2 tl;
	vec2 br;
}
bounds;



layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outTexCoord;

out gl_PerVertex { vec4 gl_Position; };

float CalcVerticalDisp (vec2 position)
{
	vec2 uv = mix (bounds.tl, bounds.br, position);

	return texture (texHeightMap, uv).r;
}

void main ()
{
	vec3 pos = inPosition;
	pos.y = CalcVerticalDisp (vec2 (inPosition.x, inPosition.z));
	gl_Position = cam.projView * mnd.model * vec4 (pos, 1.0);

	outTexCoord = inTexCoord;
	outNormal = inNormal;
	outFragPos = (mnd.model * vec4 (pos, 1.0)).xyz;
}