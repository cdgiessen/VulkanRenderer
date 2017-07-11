#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inViewVec;
layout(location = 4) in vec3 inLightVec;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 texColor = texture(texSampler, inTexCoord);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);

	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0f);
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
    outColor = texColor * vec4((diffuse + specular)* inColor, 1.0f);
}