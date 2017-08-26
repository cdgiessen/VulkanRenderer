#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (push_constant) uniform PushConstants {
	vec2 scale;
	vec2 translate;
} pushConstants;

layout(location = 0) out struct{
    vec4 Color;
    vec2 UV;
} Out;

out gl_PerVertex {
	vec4 gl_Position;   
};

void main() 
{
	Out.Color = inColor;
    Out.UV = inUV;
	gl_Position = vec4(inPos * pushConstants.scale + pushConstants.translate, 0.0, 1.0);
}