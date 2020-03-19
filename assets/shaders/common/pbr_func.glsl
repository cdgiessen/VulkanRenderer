
vec3 fresnelSchlick (float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow (2.0, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float DistributionGGX (vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max (dot (N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;
	return num / denom;
}

float GeometrySchlickGGX (float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;
	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	return num / denom;
}

float GeometrySmith (vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max (dot (N, V), 0.0);
	float NdotL = max (dot (N, L), 0.0);
	float ggx2 = GeometrySchlickGGX (NdotV, roughness);
	float ggx1 = GeometrySchlickGGX (NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 PBR_Calc (vec3 N, vec3 V, vec3 F0, vec3 L, vec3 H, vec3 radiance, vec3 albedo, float roughness, float metalness)
{
	// cook-torrance brdf
	float NDF = DistributionGGX (N, H, roughness);
	float G = GeometrySmith (N, V, L, roughness);
	vec3 F = fresnelSchlick (max (dot (H, V), 0.0), F0);
	vec3 kS = F;
	vec3 kD = vec3 (1.0) - kS;
	kD *= 1.0 - metalness;
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max (dot (N, V), 0.0) * max (dot (N, L), 0.0);
	vec3 specular = numerator / max (denominator, 0.001);
	// add to outgoing radiance Lo
	float NdotL = max (dot (N, L), 0.0);
	return (kD * albedo / PI + specular) * radiance * NdotL;
}