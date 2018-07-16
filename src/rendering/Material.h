#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <variant>

#include <glm/glm.hpp>

#include "Device.h"
#include "Buffer.h"
#include "Texture.h"
#include "Descriptor.h"
#include "Pipeline.h"
#include "Shader.h"
#include "RenderStructs.h"

enum class ResourceType {
	uniform,
	texture2D,
	textureArray,
	cubemap,
};

VkDescriptorType GetVulkanDescriptorType(ResourceType type);


enum class ResourceStages {
	vertex_only,
	fragment_only,
	vertex_fragment,
};

VkShaderStageFlags GetVulkanShaderStageFlags(ResourceStages stage);

struct MaterialDataSlot {
	ResourceType type;
	ResourceStages stage;
	DescriptorResource resource;

	MaterialDataSlot(ResourceType type, ResourceStages stage,
		DescriptorResource resources) :
		type(type), stage(stage), resource(resource)
	{}
	const int count = 1; //only one resource for now
};


struct Phong_Material {
	glm::vec4 color = glm::vec4(0.5, 0.5, 0.5, 1.0);
	float diffuse = 0.8f;
	float specular = 0.2f;
	float reflectivity = 4;
	float padding = 0;
};

struct PBR_Mat_Value {
	glm::vec3 albedo = glm::vec3(0.5, 0.5, 0.5);
	float metallic = 0.1f;
	float roughness = 0.5f;
	float ao = 1;
	glm::vec3 emmisive = glm::vec3(0.0, 0.0, 0.0);
};

using MaterialOptions = std::variant<Phong_Material, PBR_Mat_Value, PBR_Mat_Tex>;

//struct MaterialResource {
//
//	MaterialResource(std::shared_ptr<VulkanBufferUniform> buffer);
//	MaterialResource(std::shared_ptr<Texture> texture);
//
//
//};

using DataTypeVar = std::variant<float, glm::vec2, glm::vec3, glm::vec4, int>;

struct VariableUniformSlot {
	DataTypeVar data;
	std::string name;

	VariableUniformSlot(DataTypeVar data, std::string& name) :
		data(data), name(name) {}
};

class VariableUniformController {
public:
	VariableUniformController(std::vector<VariableUniformSlot> input);

	void Set(int8_t index, DataTypeVar dataTypes);

	DataTypeVar Get(int8_t index);

private:
	std::vector<VariableUniformSlot> data;
};

class VulkanMaterial {
public:
	VulkanMaterial(VulkanDevice& device);
	~VulkanMaterial();

	void SetShaders(ShaderModuleSet set);

	void AddTexture(std::shared_ptr<VulkanTexture> tex);
	void AddTextureArray(std::shared_ptr<VulkanTexture> texArr);
	void AddValue(MaterialOptions value);

	void AddMaterialDataSlot(MaterialDataSlot slot);

	void Setup();

	void Bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout);

	VkDescriptorSetLayout GetDescriptorSetLayout();
private:
	VulkanDevice & device;

	DescriptorSet descriptorSet;
	VulkanDescriptor descriptor;


	ShaderModuleSet shaderModules;

	std::vector<MaterialDataSlot> dataSlots;

	std::vector<std::shared_ptr<VulkanTexture>> textures;
	std::vector<std::shared_ptr<VulkanTexture>> textureArrays;
	std::vector<MaterialOptions> value_var;
	std::shared_ptr<VulkanBufferUniform> value_data;

};