#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PointLight {
	vec4 lightPos;
	vec4 color;
	vec4 attenuation;
};

#define lightCount 5

//Lighting information
layout(binding = 2) uniform PointLightsBuffer {
	PointLight lights[lightCount];
} plb;

//texture sampling
layout(binding = 3) uniform sampler2D waterTex;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inFragPos;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 texColor = texture(waterTex, inTexCoord);
	vec3 viewVec = normalize(-inFragPos);

	vec3 N = normalize(inNormal);
	vec3 V = normalize(-inFragPos);

	vec3 pointLightContrib = vec3(0.0f);
	for(int i = 0; i < lightCount; i++){
		vec3 L = normalize(plb.lights[i].lightPos.xyz - inFragPos).xyz;
		vec3 H = normalize(viewVec + plb.lights[i].lightPos.xyz);	
		//vec3 R = reflect(-L, N);

		float distance = length(plb.lights[i].lightPos.xyz - inFragPos);
		float attenuation = 1.0f/(plb.lights[i].attenuation.x + plb.lights[i].attenuation.y*distance + plb.lights[i].attenuation.z*distance*distance);

		vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0f) * attenuation * plb.lights[i].color.xyz;
		vec3 specular = pow(max(dot(N, H), 0.0), 16.0) * vec3(1.0f)* attenuation * plb.lights[i].color.xyz;
		pointLightContrib += (diffuse + specular);
	}

	vec3 dirLight = normalize(vec3(0,60,25));
	vec3 dirReflected = reflect(dirLight, N);
	vec3 dirDiffuse = max(dot(N, dirLight),0.0f)* vec3(1.0f);
	vec3 dirSpecular = pow(max(dot(dirReflected, viewVec), 0.0), 16.0f)* vec3(1.0f);
	vec3 dirContrib = (dirDiffuse + dirSpecular)* vec3(1.0f);

    outColor = texColor * vec4((pointLightContrib + dirContrib) * inColor, 1.0f);
	//outColor = texColor * vec4(inColor, 1.0f);
	
}
