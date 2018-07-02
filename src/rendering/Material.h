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

class VulkanMaterial {
public:
	VulkanMaterial(VulkanDevice& device);
	void CleanUp();

	void SetShaders(ShaderModuleSet set);

	void AddTexture(std::shared_ptr<VulkanTexture> tex);
	void AddTextureArray(std::shared_ptr<VulkanTexture2DArray> texArr);
	void AddValue(MaterialOptions value);

	void Setup();

	void Bind(VkCommandBuffer cmdBuf);


private:
	VulkanDevice & device;

	DescriptorSet descriptorSet;
	VulkanDescriptor descriptor;

	ShaderModuleSet shaderModules;

	std::vector<std::shared_ptr<VulkanTexture>> textures;
	std::vector<std::shared_ptr<VulkanTexture2DArray>> textureArrays;
	std::vector<MaterialOptions> value_var;
	std::shared_ptr<VulkanBufferUniform> value_data;

};