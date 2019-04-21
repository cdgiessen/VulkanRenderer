#version 450
#extension GL_ARB_separate_shader_objects : enable

const int DirectionalLightCount = 5;
const int PointLightCount = 10;
const int SpotLightCount = 10;
const float PI = 3.14159265359;

layout (set = 0, binding = 0) uniform GlobalData { float time; }
global;

layout (set = 0, binding = 1) uniform CameraData
{
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
}
cam;

struct DirectionalLight
{
	vec3 direction;
	float intensity;
	vec3 color;
	float dum;
};

struct PointLight
{
	vec3 position;
	float intensity;
	vec3 color;
};

struct SpotLight
{
	vec3 position;
	vec3 color;
	float attenuation;
	float cutoff;
};

layout (set = 1, binding = 0) uniform DirectionalLightData { DirectionalLight lights[1]; }
directional;

layout (set = 1, binding = 1) uniform PointLightData { PointLight lights[PointLightCount]; }
point;

layout (set = 1, binding = 2) uniform SpotLightData { SpotLight lights[SpotLightCount]; }
spot;

layout (set = 2, binding = 1) uniform Phong_Material
{
	vec4 color;
	float diffuse;
	float specular;
	float reflectivity;
	float padding;
}
phong_mat;

// texture sampling
// layout(set = 2, binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

vec3 DirPhongLighting (vec3 view, vec3 normal, int i)
{

	vec3 light = normalize (directional.lights[i].direction);
	vec3 halfway = normalize (light + view);
	vec3 reflect = reflect (-light, normal);
	vec3 diffuse = max (dot (normal, light), 0.0f) * vec3 (phong_mat.diffuse);
	vec3 specular = pow (max (dot (view, reflect), 0.0), phong_mat.reflectivity) * vec3 (phong_mat.specular);
	vec3 contrib =
	    (diffuse + specular) * vec3 (directional.lights[i].intensity) * directional.lights[i].color;

	return contrib;
}

void main ()
{
	vec4 texColor = vec4 (1.0); // texture(texSampler, inTexCoord);

	vec3 viewVec = normalize (-cam.cameraDir);

	vec3 normalVec = normalize (inNormal);

	vec3 pointLightContrib = vec3 (0.0f);
	for (int i = 0; i < PointLightCount; i++)
	{
		vec3 lightVec = normalize (point.lights[i].position - inFragPos).xyz;
		vec3 reflectVec = reflect (-lightVec, normalVec);
		vec3 halfwayVec = normalize (viewVec + point.lights[i].position);

		float separation = distance (point.lights[i].position, inFragPos);
		float attenuation = 1.0f / (separation * separation);

		vec3 diffuse =
		    max (dot (normalVec, lightVec), 0.0) * vec3 (1.0f) * attenuation * point.lights[i].color;
		vec3 specular = pow (max (dot (viewVec, reflectVec), 0.0), 16.0) * vec3 (0.75f) *
		                attenuation * point.lights[i].color;
		pointLightContrib += (diffuse + specular);
	}

	vec3 spotLightContrib = vec3 (0.0f);
	for (int i = 0; i < DirectionalLightCount; i++)
	{
		spotLightContrib += DirPhongLighting (viewVec, normalVec, i);
	}

	outColor = texColor * vec4 (((pointLightContrib + spotLightContrib)), 1.0f) * phong_mat.color;
	// outColor = texColor * vec4(pointLightContrib * inColor, 1.0f);

	outColor = vec4 (pow (outColor.xyz, vec3 (1.0 / 2.2)), 1.0);
}