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

//VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath);


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
	void SetMaterialValue(MaterialOptions value);

	void Setup();

	void Bind(VkCommandBuffer cmdBuf);


private:
	VulkanDevice & device;

	DescriptorSet descriptorSet;
	VulkanDescriptor descriptor;

	ShaderModuleSet shaderModules;

	std::vector<std::shared_ptr<VulkanTexture>> textures;
	std::vector<std::shared_ptr<VulkanTexture2DArray>> textureArrays;
	MaterialOptions value_var;
	std::shared_ptr<VulkanBufferUniform> value_data;

};