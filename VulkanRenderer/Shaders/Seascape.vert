#version 450
#extension GL_ARB_separate_shader_objects : enable

//Global information
layout(binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	float time; // in seconds
} cbo;

//per model information
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
	mat4 normal;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outFragPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec4 outColor;
layout(location = 4) out float outTime;
layout(location = 5) out vec3 fragCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	outTexCoord = inTexCoord;
	outColor = inColor;

	fragCoord = ( ubo.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = cbo.proj * cbo.view * ubo.model * vec4(inPosition, 1.0);

	outNormal = (ubo.normal * vec4(inNormal, 1.0f)).xyz;
	outFragPos = (ubo.model * vec4(inPosition, 1.0)).xyz;		
	outTime = cbo.time;
}