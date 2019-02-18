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

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;

out gl_PerVertex { vec4 gl_Position; };

float height (vec3 pos)
{
	float t = global.time / 5;
	return sin (t + pos.x / 10) * cos (t + pos.z / 11);
}

void main ()
{
	vec3 vert_pos = inPosition;
	vec3 pos = inPosition + cam.cameraPos; // for y value comp

	float disp = height (pos);
	vert_pos.y = disp * 2;

	gl_Position = cam.projView * mnd.model * vec4 (vert_pos, 1.0);

	vec3 out_norm = normalize (vec3 (height (pos + vec3 (1, 0, 0)) - height (pos + vec3 (-1, 0, 0)),
	    2,
	    height (pos + vec3 (0, 0, 1)) - height (pos + vec3 (0, 0, -1))));

	outNormal = (mnd.normal * vec4 (out_norm, 1.0f)).xyz;
	outFragPos = (vec4 (vert_pos, 1.0)).xyz;
}
