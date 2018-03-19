#version 450
#extension GL_ARB_separate_shader_objects : enable

struct PointLight {
	vec4 lightPos;
	vec4 color;
	vec4 attenuation;
};

#define lightCount 5

layout(set = 0, binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraDir;
	float time;
	vec3 sunDir;
	float sunIntensity;
	vec3 sunColor;

} cbo;

//Lighting information
layout(set = 1, binding = 1) uniform PointLightsBuffer {
	PointLight lights[lightCount];
} plb;

//texture sampling
layout(set = 2, binding = 3) uniform sampler2D texSplatMap;
layout(set = 2, binding = 4) uniform sampler2DArray texArray;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inFragPos;

layout(location = 0) out vec4 outColor;

vec3 DirPhongLighting(vec3 view, vec3 dir, vec3 normal, vec3 color, float intensity) {
	vec3 light = normalize(dir);
	vec3 halfway = normalize(light + view);
	vec3 reflect = reflect(-light, normal);
	vec3 diffuse = max(dot(normal, light), 0.0f)* vec3(0.8f);
	vec3 specular = pow(max(dot(view, reflect), 0.0), 16.0f)* vec3(0.15f);
	vec3 contrib = (diffuse + specular)* vec3(intensity) * color;

	return contrib;
}

void main() {
	//vec4 texColor = inColor; //splatmap not in yet, so just use vertex colors until then
	vec4 texColor = texture(texSplatMap, inTexCoord);

	float texSampleDensity = 2000.0f;

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

	vec3 dirContrib = DirPhongLighting(viewVec, cbo.sunDir, normalVec, cbo.sunColor, cbo.sunIntensity);

	//float belowWaterLevelDarkening = clamp(inFragPos.y, -1, 0);
	//outColor = (vec4(0,0,0,0) + inColor) * vec4((pointLightContrib + dirContrib), 1.0f);
	outColor = terrainSplatter * vec4((pointLightContrib + dirContrib), 1.0f);
	//outColor = vec4(pointLightContrib * inColor, 1.0f);
	
	
}