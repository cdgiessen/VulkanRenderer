#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "globals.glsl"

#include "camera.glsl"

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
    gl_Position = cam.proj_view * mnd.model * vec4(inPosition, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(mnd.model)));
    vs_out.normal = normalize(vec3(cam.proj_view * vec4(normalMatrix * inNormal, 1.0)));
}