#pragma once

#include "cml/cml.h"

#include "Texture.h"

/* Common uniform buffers */

struct GlobalData
{
	float time;
};

struct CameraData
{
	cml::mat4f projView;
	cml::mat4f view;
	cml::vec3f cameraDir;
	cml::vec3f cameraPos;
};

/* Lighting */

struct DirectionalLight
{
	cml::vec3f direction;
	cml::vec3f color;
	float intensity = 0.0f;

	DirectionalLight (){};
	DirectionalLight (cml::vec3f dir, cml::vec3f color, float intensity)
	: direction (dir), color (color), intensity (intensity)
	{
	}
};

struct PointLight
{
	cml::vec3f position;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cufOff = 0.0f;

	PointLight (){};
	PointLight (cml::vec3f position, cml::vec3f color, float attenuation)
	: position (position), color (color), attenuation (attenuation)
	{
	}
};

struct SpotLight
{
	cml::vec3f position;
	cml::vec3f direction;
	cml::vec3f color;
	float attenuation = 0.0f;
	float cutoff = 0.0f;
	float outerCutOff = 0.0f;


	SpotLight (){};
	SpotLight (cml::vec3f position, cml::vec3f dir, cml::vec3f color, float attenuation, float cutoff, float outerCutOff)
	: position (position), direction (dir), color (color), attenuation (attenuation), cutoff (cutoff), outerCutOff (outerCutOff)
	{
	}
};

/* Model and Normal matrices */

struct TransformMatrixData
{
	cml::mat4f model;
	cml::mat4f normal;
};

struct ModelBufferObject
{
	cml::mat4f model;
	cml::mat4f normal;

	cml::mat4f paddingOne;
	cml::mat4f paddingTwo;
};

struct ModelPushConstant
{
	cml::mat4f model;
	cml::mat4f normal;
};

struct StaticModelBuffer
{
	cml::mat4f model;
};

struct StaticModelPushConstant
{
	cml::mat4f model;
};

/* Materials */

struct PBR_Mat_Tex
{
	VulkanTextureID tx_albedo;
	VulkanTextureID tx_metallic;
	VulkanTextureID tx_roughness;
	VulkanTextureID tx_ao;
	VulkanTextureID tx_emissiveTexture;
	VulkanTextureID tx_normal;
};

struct PBR_Material
{
	cml::vec3f albedo = cml::vec3f (0.5, 0.5, 0.5);
	float metallic = 0.1f;
	float roughness = 0.5f;
	float ao = 1;
	cml::vec3f emissive = cml::vec3f (0.0, 0.0, 0.0);

	bool useTexAlbedo = false;
	bool useTexMetallic = false;
	bool useTexRoughness = false;
	bool useTexAmbientOcclusion = false;
	bool useTexEmissive = false;
	bool useTexNormal = false;

	VulkanTextureID tx_albedo;
	VulkanTextureID tx_metallic;
	VulkanTextureID tx_roughness;
	VulkanTextureID tx_ao;
	VulkanTextureID tx_emissiveTexture;
	VulkanTextureID tx_normal;
};