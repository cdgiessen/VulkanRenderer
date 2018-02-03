#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//Global information
layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	float time;
} gub;


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
	vec4 pos =  gub.proj * mat4(mat3(gub.view)) * vec4(inPos.xyz, 1.0);
	pos.z = 0;
	gl_Position = pos;
}
