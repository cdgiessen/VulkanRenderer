#version 450
#extension GL_ARB_separate_shader_objects : enable

//Global information
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	float time;
} cbo;

//per model information
layout(push_constant) uniform PER_OBJECT
{
    mat4 model;
    mat4 normal;
} obj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(location = 0) out VS_OUT {
    vec3 normal;
} vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = cbo.proj * cbo.view * obj.model * vec4(inPosition, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(cbo.view * obj.model)));
    vs_out.normal = normalize(vec3(cbo.proj * vec4(normalMatrix * inNormal, 1.0)));
}