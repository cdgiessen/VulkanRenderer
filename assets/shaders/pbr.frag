#version 450 core
#extension GL_ARB_separate_shader_objects : enable

const int DirectionalLightCount = 5;
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
	float dummy;
	vec3 cameraPos;
} cam;

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
	float dum;
};

struct PointLight {
	vec3 position;
	float intensity;
	vec3 color;
};

struct SpotLight {
	vec3 position;
	float attenuation;
	vec3 color;
	float cutoff;
	float outerCutOff;
	float padding;
};

layout(set = 1, binding = 0) uniform DirectionalLightData {
	DirectionalLight lights[DirectionalLightCount];
} directional;

layout(set = 1, binding = 1) uniform PointLightData {
	PointLight lights[PointLightCount];
} point; 

layout(set = 1, binding = 2) uniform SpotLightData { 
	SpotLight lights[SpotLightCount];
} spot;

layout(set = 2, binding = 1) uniform PBR_Material_Constant {
	vec3  albedo;
	float metallic;
	float roughness;
	float ao;

} pbr_mat;

//layout(push_constant) uniform PER_OBJECT
//{
//	bool tex_albedo;
//	bool tex_metallic;
//	bool tex_roughness;
//	bool tex_ao;
//	bool tex_normal;
//	bool tex_emissive;
//
//}pc;
//
//
//layout(set = 4, binding = 0) uniform sampler pbr_sampler;
//layout(set = 4, binding = 1) uniform texture2D pbr_tex[4];
//layout(set = 4, binding = 2) uniform texture2D normalMap;



layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

 vec3 PBR_Calc(vec3 N, vec3 V, vec3 F0, vec3 L, vec3 H, vec3 radiance){
	// cook-torrance brdf
     float NDF = DistributionGGX(N, H, pbr_mat.roughness);        
     float G   = GeometrySmith(N, V, L, pbr_mat.roughness);      
     vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
   
     vec3 kS = F;
     vec3 kD = vec3(1.0) - kS;
     kD *= 1.0 - pbr_mat.metallic;	  
   
     vec3 numerator    = NDF * G * F;
     float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
     vec3 specular     = numerator / max(denominator, 0.0001);  
       
     // add to outgoing radiance Lo
     float NdotL = max(dot(N, L), 0.0);                
     return (kD * pbr_mat.albedo / PI + specular) * radiance * NdotL; 
 }

vec3 DirectionalLightingCalc(int i, vec3 N, vec3 V, vec3 F0){

     // calculate per-light radiance
     vec3 L = normalize(directional.lights[i].direction);
     vec3 H = normalize(V + L);
     vec3 radiance     = directional.lights[i].color
					   * directional.lights[i].intensity; 
   
    return PBR_Calc(N, V, F0, L, H, radiance);
 } 

vec3 PointLightingCalc(int i, vec3 N, vec3 V, vec3 F0){

     // calculate per-light radiance
     vec3 L = normalize(point.lights[i].position - inFragPos);
     vec3 H = normalize(V + L);
     float separation    = distance(point.lights[i].position, inFragPos);
     float attenuation = 1.0 / (separation * separation);
     vec3 radiance     = point.lights[i].color	
		* point.lights[i].intensity 
		* attenuation;        
   
	return PBR_Calc(N, V, F0, L, H, radiance);
} 

vec3 LightingContribution(vec3 N, vec3 V, vec3 F0)
{
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < DirectionalLightCount; ++i) 
    {
		Lo += DirectionalLightingCalc(i, N, V, F0);
	} 
    for(int i = 0; i < PointLightCount; ++i) 
    {
		//Lo += PointLightingCalc(i, N, V, F0);
	}

	return Lo;
}


void main()
{		
    vec3 N = normalize(inNormal);
    vec3 V = normalize(cam.cameraPos - inFragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, pbr_mat.albedo, pbr_mat.metallic);
	           
    vec3 ambient = vec3(0.0) * pbr_mat.albedo * pbr_mat.ao;
    vec3 color = ambient + LightingContribution(N, V, F0);
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    outColor = vec4(color, 1.0);
}