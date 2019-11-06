#pragma once

#include "cml/cml.h"

#include "Texture.h"

/* Common uniform buffers */

struct GlobalData
{
	float time;
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