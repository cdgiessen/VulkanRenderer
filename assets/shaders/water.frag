#version 450
#extension GL_ARB_separate_shader_objects : enable

const int PointLightCount = 10;
const int SpotLightCount = 10;
const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform GlobalData {
	float time;
} global;

layout(set = 0, binding = 1) uniform CameraData {
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
} cam;

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
};

struct PointLight {
	vec3 position;
	float intensity;
	vec3 color;
};

struct SpotLight {
	vec3 position;
	vec3 color;
	float attenuation;
	float cutoff;
};

layout(set = 1, binding = 0) uniform DirectionalLightData {
	DirectionalLight light[1];
} sun;

layout(set = 1, binding = 1) uniform PointLightData {
	PointLight lights[PointLightCount];
} point; 

layout(set = 1, binding = 2) uniform SpotLightData { 
	SpotLight lights[SpotLightCount];
} spot;

//texture sampling
layout(set = 2, binding = 1) uniform sampler2D waterTex;

layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec4 outColor;

vec4 causticsSampler(vec2 uv, float timeIn) {
	#define TAU 6.28318530718
	#define MAX_ITER 5
	

	float time = global.time * .5+23.0;
    vec2 p = mod(uv*TAU, TAU)-250.0;
	vec2 i = vec2(p);
	float c = 1.0;
	float inten = .005;

	for (int n = 0; n < MAX_ITER; n++) 
	{
		float t = time * (1.0 - (3.5 / float(n+1)));
		i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
	}
	c /= float(MAX_ITER);
	c = 1.17-pow(c, 1.4);
	vec3 colour = vec3(pow(abs(c), 8.0));
    colour = clamp(colour + vec3(0.01, 0.35, 0.5) * 1.3f, 0.0, 1.0);
    
	return  vec4(colour, 1.0);

}

vec3 DirPhongLighting(vec3 view, vec3 dir, vec3 normal, vec3 color, float intensity) {
	vec3 light = normalize(dir);
	vec3 halfway = normalize(light + view);
	vec3 reflect = reflect(-light, normal);
	vec3 diffuse = max(dot(normal, light), 0.0f)* vec3(0.8f);
	vec3 specular = pow(max(dot(view, reflect), 0.0), 16.0f)* vec3(0.25f);
	vec3 contrib = (diffuse + specular)* vec3(intensity) * color;

	return contrib;
}

void main() {
	float inTime = global.time;
	vec4 texSample1 = texture(waterTex, vec2(inTexCoord.x + cos(inTime/5.0f + 2.0f)/7.0f, inTexCoord.y + cos(inTime/6.0f)/7.5f+ 1.0f));
	vec4 texSample2 = texture(waterTex, vec2(inTexCoord.x + 0.1f + sin(inTime/5.0f+ 0.5f)/8.0f, inTexCoord.y + 0.1f + sin(inTime/4.0f+ 1.5f)/6.0f));
	vec4 texSample3 = texture(waterTex, vec2(inTexCoord.x , inTexCoord.y + sin(inTime/4.5f)/10.0f));
	vec4 texSampleAll =  vec4(0.25) * (texSample1 + texSample2 + texSample3);

	float newTime = inTime * 0.05f;
	vec2 uvModified = vec2(	inFragPos.x + sin(newTime * 1.2f + cos(newTime * 0.25f) * 1.2f) * 1.6f + sin(newTime * 0.25f) * 1.2f + newTime * 0.8f * (sin(newTime) + 3) + 1.0f, 
							inFragPos.z + cos(newTime * 1.1f + sin(newTime * 0.3f ) * 1.6f) * 1.8f + cos(newTime * 0.3f ) * 1.6f + newTime * 0.5f * (sin(newTime) + 2) + 1.0f);
	vec2 uvModified2 = vec2(inFragPos.x + cos(newTime * 1.3f + sin(newTime * 0.35f) * 1.5f) * 1.5f + sin(newTime * 0.35f) * 1.5f + newTime * 0.5f * (sin(newTime) + 2), 
							inFragPos.z + sin(newTime * 1.4f + cos(newTime * 0.45f ) * 1.4f) * 1.4f + cos(newTime * 0.45f ) * 1.4f + newTime * 0.8f * (sin(newTime) + 3) );
	vec4 texColor = vec4(0.3) * causticsSampler(uvModified, inTime * 0.2f) + vec4(0.3) * causticsSampler(uvModified2, inTime * 0.6f) + texSampleAll;

	vec3 viewVec = normalize(-cam.cameraDir);

	vec3 normalVec = normalize(inNormal);

vec3 pointLightContrib = vec3(0.0f);
	for(int i = 0; i < PointLightCount; i++){
		vec3 lightVec = normalize(point.lights[i].position - inFragPos).xyz;
		vec3 reflectVec = reflect(-lightVec, normalVec);
		vec3 halfwayVec = normalize(viewVec + point.lights[i].position);	
		
		float separation = distance(point.lights[i].position, inFragPos);
		float attenuation = 1.0f/(separation*separation);

		vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(1.0f) * attenuation * point.lights[i].color;
		vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) * vec3(0.75f)* attenuation * point.lights[i].color;
		pointLightContrib += (diffuse + specular);
	}

	vec3 dirContrib = DirPhongLighting(viewVec, sun.light[0].direction, normalVec, sun.light[0].color, sun.light[0].intensity);

    outColor = texColor * vec4((pointLightContrib + dirContrib), 1.0f) * inColor;
	//outColor = texColor * vec4(pointLightContrib * inColor, 1.0f);
	
}
