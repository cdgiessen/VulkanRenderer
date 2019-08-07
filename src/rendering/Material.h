#pragma once

#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "cml/cml.h"

#include "Buffer.h"
#include "Descriptor.h"
#include "Pipeline.h"
#include "RenderStructs.h"
#include "Shader.h"
#include "Texture.h"

class VulkanDevice;

enum class ResourceType
{
	uniform,
	texture2D,
	textureArray,
	cubemap,
};

VkDescriptorType GetVulkanDescriptorType (ResourceType type);


enum class ResourceStages
{
	vertex_only,
	fragment_only,
	vertex_fragment,
};

VkShaderStageFlags GetVulkanShaderStageFlags (ResourceStages stage);

struct MaterialDataSlot
{
	ResourceType type;
	ResourceStages stage;
	DescriptorResource resource;

	MaterialDataSlot (ResourceType type, ResourceStages stage, DescriptorResource resource)
	: type (type), stage (stage), resource (resource)
	{
	}
	const int count = 1; // only one resource for now
};


struct Phong_Material
{
	cml::vec4f color = cml::vec4f (0.5, 0.5, 0.5, 1.0);
	float diffuse = 0.8f;
	float specular = 0.2f;
	float reflectivity = 4;
	float padding = 0;
};

struct PBR_Mat_Value
{
	cml::vec3f albedo = cml::vec3f (0.5, 0.5, 0.5);
	float metallic = 0.1f;
	float roughness = 0.5f;
	float ao = 1;
	cml::vec3f emissive = cml::vec3f (0.0, 0.0, 0.0);
};

using MaterialOptions = std::variant<Phong_Material, PBR_Mat_Value, PBR_Mat_Tex>;

using DataTypeVar = std::variant<float, cml::vec2f, cml::vec3f, cml::vec4f, int>;

struct VariableUniformSlot
{
	DataTypeVar data;
	std::string name;

	VariableUniformSlot (DataTypeVar data, std::string& name) : data (data), name (name) {}
};

class VariableUniformController
{
	public:
	VariableUniformController (std::vector<VariableUniformSlot> input);

	void Set (int8_t index, DataTypeVar dataTypes);

	DataTypeVar Get (int8_t index);

	private:
	std::vector<VariableUniformSlot> data;
};

class VulkanMaterial
{
	public:
	VulkanMaterial (VulkanDevice& device);
	~VulkanMaterial ();

	void SetShaders (ShaderModuleSet set);

	void AddTexture (VulkanTextureID tex);
	void AddTextureArray (VulkanTextureID texArr);
	void AddValue (MaterialOptions value);

	void AddMaterialDataSlot (MaterialDataSlot slot);

	void Setup ();

	void Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout);

	VkDescriptorSetLayout GetDescriptorSetLayout ();

	private:
	VulkanDevice& device;

	DescriptorSet descriptorSet;
	VulkanDescriptor descriptor;


	ShaderModuleSet shaderModules;

	std::vector<MaterialDataSlot> dataSlots;

	std::vector<VulkanTextureID> textures;
	std::vector<VulkanTextureID> textureArrays;
	std::vector<MaterialOptions> value_var;
	std::unique_ptr<VulkanBuffer> value_data;
};

// class VulkanMaterialInstance
// {
// 	public:
// 	void Bind (VkCommandBuffer cmdBuf, VkPipelineLayout layout);

// 	private:
// 	VulkanDevice& device;

// 	DescriptorSet descriptorSet;
// 	VulkanDescriptor descriptor;


// 	std::vector<VulkanTextureID> textures;
// 	std::vector<VulkanTextureID> textureArrays;
// 	std::vector<MaterialOptions> value_var;
// 	std::shared_ptr<VulkanBuffer> value_data;
// };