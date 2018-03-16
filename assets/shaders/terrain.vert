#version 450
#extension GL_ARB_separate_shader_objects : enable

//Global information
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	float time;
	vec3 sunDir;
	float sunIntensity;
	vec3 sunColor;
} cbo;

//per model information
//layout(set = 2, binding = 2) uniform UniformBufferObject {
//    mat4 model;
//	mat4 normal;
//} ubo;

layout(push_constant) uniform PER_OBJECT
{
    mat4 model;
    mat4 normal;
} obj;



layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec3 outFragPos;
layout(location = 4) out float outTime;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	outTexCoord = inTexCoord;
	outColor = inColor;

    gl_Position = cbo.proj * cbo.view * obj.model * vec4(inPosition, 1.0);

	outNormal = (obj.normal * vec4(inNormal,1.0f)).xyz;
	outFragPos = (obj.model * vec4(inPosition, 1.0)).xyz;		
}