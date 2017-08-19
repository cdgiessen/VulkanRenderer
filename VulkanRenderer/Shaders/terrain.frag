#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PointLight {
	vec4 lightPos;
	vec4 color;
	vec4 attenuation;
};

#define lightCount 5

layout(binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraDir;
	float time;
} cbo;

//Lighting information
layout(binding = 2) uniform PointLightsBuffer {
	PointLight lights[lightCount];
} plb;

//texture sampling
layout(binding = 3) uniform sampler2D texSplatMap;
layout(binding = 4) uniform sampler2DArray texArray;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inFragPos;
layout(location = 4) in float inTime;

layout(location = 0) out vec4 outColor;

void main() {
	//vec4 texColor = inColor; //splatmap not in yet, so just use vertex colors until then
	vec4 texColor = texture(texSplatMap, inTexCoord);

	float texSampleDensity = 50.0f;

	vec4 layerFirst = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0));
	vec4 layerSecond = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1));
	vec4 layerThird = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2));
	vec4 layerFourth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3));
	//vec4 layerFifth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 4));

	vec4 terrainSplatter = (layerFirst * texColor.x + layerSecond*texColor.y + layerThird * texColor.z + layerFourth * (1 - texColor.r - texColor.g - texColor.b));

	vec3 viewVec = normalize(-cbo.cameraDir);

	vec3 normalVec = normalize(inNormal);

	vec3 pointLightContrib = vec3(0.0f);
	for(int i = 0; i < lightCount; i++){
		vec3 lightVec = normalize(plb.lights[i].lightPos.xyz - inFragPos).xyz;
		vec3 reflectVec = reflect(-lightVec, normalVec);
			vec3 halfwayVec = normalize(viewVec + plb.lights[i].lightPos.xyz);	
		
		float distance = length(plb.lights[i].lightPos.xyz - inFragPos);
		float attenuation = 1.0f/(plb.lights[i].attenuation.x + plb.lights[i].attenuation.y*distance + plb.lights[i].attenuation.z*distance*distance);

		vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(0.8f) * attenuation * plb.lights[i].color.xyz;
		vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) * vec3(0.75f)* attenuation * plb.lights[i].color.xyz;
		pointLightContrib += (diffuse + specular);
	}

	vec3 dirLight = normalize(vec3(0, 60, 25));
		vec3 dirHalfway = normalize(dirLight + viewVec);
	vec3 dirReflect = reflect(-dirLight, normalVec);
	vec3 dirDiffuse = max(dot(normalVec, dirLight), 0.0f)* vec3(0.9f);
	vec3 dirSpecular = pow(max(dot(viewVec, dirReflect), 0.0), 1.0f)* vec3(0.2f);
	vec3 dirContrib = (dirDiffuse + dirSpecular)* vec3(1.0f);

	//float belowWaterLevelDarkening = clamp(inFragPos.y, -1, 0);

    outColor = terrainSplatter * vec4((pointLightContrib + dirContrib), 1.0f);
	//outColor = vec4(pointLightContrib * inColor, 1.0f);
	
	
}