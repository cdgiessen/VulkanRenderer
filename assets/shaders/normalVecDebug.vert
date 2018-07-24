#version 450
#extension GL_ARB_separate_shader_objects : enable

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

//per model information
layout(set = 2, binding = 0) uniform ModelMatrixData {
    mat4 model;
	mat4 normal;
} mnd;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VS_OUT {
    vec3 normal;
} vs_out;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = cam.projView * mnd.model * vec4(inPosition, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(mnd.model)));
    vs_out.normal = normalize(vec3(cam.projView * vec4(normalMatrix * inNormal, 1.0)));
}