#version 450
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

//texture sampling
layout(set = 2, binding = 1) uniform sampler2D texSplatMap;
layout(set = 2, binding = 2) uniform sampler2DArray texArray;

layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

//vec3 DirPhongLighting(vec3 view, vec3 dir, vec3 normal, vec3 color, float intensity) {
//	vec3 light = normalize(dir);
//	vec3 halfway = normalize(light + view);
//	vec3 reflect = reflect(-light, normal);
//	vec3 diffuse = max(dot(normal, light), 0.0f)* vec3(0.8f);
//	vec3 specular = pow(max(dot(view, reflect), 0.0), 2.0f)* vec3(0.05f);
//	vec3 contrib = (diffuse + specular)* vec3(intensity) * color;

//	return contrib;
//}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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

 vec3 PBR_Calc(vec3 N, vec3 V, vec3 F0, vec3 L, vec3 H, vec3 radiance, vec3 albedo){
	// cook-torrance brdf
     float NDF = DistributionGGX(N, H, 0.5f/*pbr_mat.roughness*/);        
     float G   = GeometrySmith(N, V, L, 0.5f/*pbr_mat.roughness*/);      
     vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
   
     vec3 kS = F;
     vec3 kD = vec3(1.0) - kS;
     kD *= 1.0 - 0.2f/*pbr_mat.metallic*/;	  
   
     vec3 numerator    = NDF * G * F;
     float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
     vec3 specular     = numerator / max(denominator, 0.0001);  
       
     // add to outgoing radiance Lo
     float NdotL = max(dot(N, L), 0.0);                
     return (kD * albedo/*pbr_mat.albedo*/ / PI + specular) * radiance * NdotL; 
 }

vec3 DirectionalLightingCalc(int i, vec3 N, vec3 V, vec3 F0, vec3 albedo){

     // calculate per-light radiance
     vec3 L = normalize(directional.lights[i].direction);
     vec3 H = normalize(V + L);
     vec3 radiance     = directional.lights[i].color
					   * directional.lights[i].intensity; 
   
    return PBR_Calc(N, V, F0, L, H, radiance, albedo);
 } 

vec3 PointLightingCalc(int i, vec3 N, vec3 V, vec3 F0, vec3 albedo){

     // calculate per-light radiance
     vec3 L = normalize(point.lights[i].position - inFragPos);
     vec3 H = normalize(V + L);
     float separation    = distance(point.lights[i].position, inFragPos);
     float attenuation = 1.0 / (separation * separation);
     vec3 radiance     = point.lights[i].color	
		* point.lights[i].intensity 
		* attenuation;        
   
	return PBR_Calc(N, V, F0, L, H, radiance, albedo);
} 

vec3 LightingContribution(vec3 N, vec3 V, vec3 F0, vec3 albedo)
{
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < DirectionalLightCount; ++i) 
    {
		Lo += DirectionalLightingCalc(i, N, V, F0, albedo);
	} 
    for(int i = 0; i < PointLightCount; ++i) 
    {
		Lo += PointLightingCalc(i, N, V, F0, albedo);
	}

	return Lo;
}

void main() {
	//vec4 texColor = inColor; //splatmap not in yet, so just use vertex colors until then
	vec4 texColor = texture(texSplatMap, inTexCoord);

	float texSampleDensity = 1000.0f;

	vec4 layerFirst = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0));
	vec4 layerSecond = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1));
	vec4 layerThird = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2));
	vec4 layerFourth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3));
	//vec4 layerFifth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 4));

	vec4 terrainSplatter = (layerFirst * texColor.x + layerSecond*texColor.y + layerThird * texColor.z + layerFourth * (1 - texColor.r - texColor.g - texColor.b));

	//vec3 viewVec = normalize(-cam.cameraDir);

	//vec3 normalVec = normalize(inNormal);

	//vec3 pointLightContrib = vec3(0.0f);
	//for(int i = 0; i < PointLightCount; i++){
	//	vec3 lightVec = normalize(point.lights[i].position - inFragPos).xyz;
	//	vec3 reflectVec = reflect(-lightVec, normalVec);
	//	vec3 halfwayVec = normalize(viewVec + point.lights[i].position);	
		
	//	float separation = distance(point.lights[i].position, inFragPos);
	//	float attenuation = 1.0f/(separation*separation);

	//	vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(1.0f) * attenuation * point.lights[i].color;
	//	vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) * vec3(0.75f)* attenuation * point.lights[i].color;
	//	pointLightContrib += (diffuse + specular);
	//}

	//vec3 dirContrib = DirPhongLighting(viewVec, sun.light[0].direction, normalVec, sun.light[0].color, sun.light[0].intensity);

	vec3 N = normalize(inNormal);
    vec3 V = normalize(cam.cameraPos - inFragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, vec3(0.129, 0.404, 1.0f)/*albedo*/, 0.0f);
	           
    vec3 ambient = vec3(0.0);
    vec3 lighting = ambient + LightingContribution(N, V, F0, vec3(terrainSplatter));
	

	//float belowWaterLevelDarkening = clamp(inFragPos.y, -1, 0);
	//outColor = (vec4(0,0,0,0) + inColor) * vec4((pointLightContrib + dirContrib), 1.0f);
	outColor = terrainSplatter * vec4(lighting, 1.0f);
	//outColor = vec4(pointLightContrib * inColor, 1.0f);
	
	
}