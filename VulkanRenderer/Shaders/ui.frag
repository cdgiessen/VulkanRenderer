#version 450

layout (set = 0, binding = 0) uniform sampler2D fontSampler;

layout(location = 0) in struct{
    vec4 Color;
    vec2 UV;
} In;

layout (location = 0) out vec4 outColor;

void main() 
{
	outColor = In.Color * texture(fontSampler, In.UV.st);
}