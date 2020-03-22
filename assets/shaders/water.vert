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

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;

out gl_PerVertex { vec4 gl_Position; };

float height (vec3 pos)
{
	float t = global.time / 3;
	return sin (t + pos.x / 8) * cos (t + pos.z / 7) + sin (1.4 * t + pos.x / 5) * cos (0.7 * t + pos.z / 6);
}

void main ()
{
	vec3 vert_pos = inPosition;
	outFragPos = vert_pos;
	vec3 static_pos = inPosition + cam.camera_pos; // for y value comp

	float disp = height (static_pos);

	float xDiff = (height (static_pos + vec3 (0.1, 0, 0)) - height (static_pos + vec3 (-0.1, 0, 0)));
	float zDiff = (height (static_pos + vec3 (0, 0, 0.1)) - height (static_pos + vec3 (0, 0, -0.1)));

	vec3 out_norm = vec3 (xDiff, 0.5, zDiff);

	vert_pos += vec3 (xDiff, disp * 1, zDiff);

	gl_Position = cam.proj_view * mnd.model * vec4 (vert_pos, 1.0);
	outNormal = normalize (out_norm);
}
