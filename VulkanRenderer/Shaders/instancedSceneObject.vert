#version 450
#extension GL_ARB_separate_shader_objects : enable

//Global information
layout(binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	float time;
} cbo;

//per model information
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
	mat4 normal;
} ubo;


// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

// Instanced attributes
layout (location = 4) in vec3 instancePos;
layout (location = 5) in vec3 instanceRot;
layout (location = 6) in float instanceScale;

// Outputs
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec3 outFragPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	outTexCoord = inTexCoord;
	outColor = inColor;

	vec4 locPos = vec4(inPosition.xyz, 1.0);
	vec4 pos = vec4((locPos.xyz * instanceScale) + instancePos, 1.0);

    gl_Position = cbo.proj * cbo.view * ubo.model * pos;

	outNormal = (ubo.normal * vec4(inNormal,1.0f)).xyz;
	outFragPos = (ubo.model * vec4(inPosition.xyz + instancePos, 1.0)).xyz;		
}