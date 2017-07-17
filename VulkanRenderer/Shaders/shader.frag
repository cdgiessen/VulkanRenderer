#version 450
#extension GL_ARB_separate_shader_objects : enable
//Lighting information
layout(binding = 2) uniform LightsBufferObject {
	vec4 lightPos;
	vec4 color;
} lbo;

//texture sampling
layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inFragPos;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 texColor = texture(texSampler, inTexCoord);
	vec3 viewVec = normalize(-inFragPos);
	vec3 lightVec = normalize(lbo.lightPos.xyz - inFragPos);

	float distance = length(lbo.lightPos.xyz - inFragPos);
	float attenuation = 1.0f/(1.0f + 0.007f*distance + 0.0002f*distance*distance);


	vec3 N = normalize(inNormal);
	vec3 L = normalize(lightVec);
	vec3 V = normalize(-inFragPos);
	//vec3 R = reflect(-L, N);
	vec3 H = normalize(viewVec + lightVec);

	vec3 dirLight = normalize(vec3(0,60,25));
	vec3 dirReflected = reflect(dirLight, N);
	vec3 dirDiffuse = max(dot(N, dirLight),0.0f)* vec3(1.0f);
	vec3 dirSpecular = pow(max(dot(dirReflected, viewVec), 0.0), 16.0f)* vec3(1.0f);
	vec3 dirContrib = (dirDiffuse + dirSpecular)* vec3(0.75f);

	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0f);
	vec3 specular = pow(max(dot(N, H), 0.0), 16.0) * vec3(1.0f);
	vec3 pointContrib = (diffuse + specular) * attenuation;

    outColor = texColor * vec4((pointContrib + dirContrib) * inColor, 1.0f);
	//outColor = texColor * vec4(inColor, 1.0f);
	
}