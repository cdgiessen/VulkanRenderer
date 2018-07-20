#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//Global information
layout(set = 0, binding = 0) uniform GlobalData {
	float time;
} global;

layout(set = 0, binding = 1) uniform CameraData {
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
} cam;

layout(set = 2, binding = 0) uniform SkyboxCameraData {
	mat4 proj;
	mat4 view;
} sky;


layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	outUVW.x *= -1.0;
	vec4 pos =  sky.proj * sky.view * vec4(inPos, 1.0);
	pos.z = 0;
	gl_Position = pos;
}
