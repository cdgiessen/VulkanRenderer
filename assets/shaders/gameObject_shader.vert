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

layout(location = 0) out vec3 outFragPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;


out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	outTexCoord = inTexCoord;

    gl_Position = cam.proj_view * mnd.model * vec4(inPosition, 1.0);

	outNormal = (mnd.normal * vec4(inNormal,1.0f)).xyz;
	outFragPos = (mnd.model * vec4(inPosition, 1.0)).xyz;		
}