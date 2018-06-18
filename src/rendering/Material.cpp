#include "Material.h"

#include "Initializers.h"

#include "../core/CoreTools.h"

#include <json.hpp>

VulkanMaterial::VulkanMaterial(VulkanDevice& device)
	:device(device),
	descriptor(device)
{



}

void VulkanMaterial::CleanUp() {
	descriptor.CleanUp();
	value_data->CleanBuffer();

	for (auto& tex : textures) {
		tex->destroy();
	}

	for (auto& texArr : textureArrays) {
		texArr->destroy();
	}
}

void VulkanMaterial::SetShaders(ShaderModuleSet set) {
	shaderModules = set;
}

void VulkanMaterial::AddTexture(std::shared_ptr<VulkanTexture> tex) {
	//textures.push_back(tex);
}

void VulkanMaterial::AddTextureArray(std::shared_ptr<VulkanTexture2DArray> texArr) {
	textureArrays.push_back(texArr);
}

void VulkanMaterial::AddValue(MaterialOptions value) {
	//value_var = value;

	value_data.reset();
	value_data = std::make_shared<VulkanBufferUniform>(device);

	if (value.index() == 0) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(Phong_Material));
	}

	else if (value.index() == 1) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Mat_Value));
	}

	else if (value.index() == 2) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Mat_Tex));
	}

}




void VulkanMaterial::Setup() {
	//descriptor = renderer->GetVulkanDescriptor();
   // descriptor = VulkanDescriptor(device);

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	int index = 0;

	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));


	//is a contiguous set of bindings (index can't be repeated)
	for (; index < textures.size(); index++) {
		m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));
	}
	for (; index < textureArrays.size(); index++) {
		m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));
	}

	descriptor.SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;

	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

	for (int i = 0; i < textures.size(); i++) {
		poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	}
	for (int i = 0; i < textureArrays.size(); i++) {
		poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	}
	descriptor.SetupPool(poolSizes);

	descriptorSet = descriptor.CreateDescriptorSet();

	std::vector<DescriptorUse> writes;

	writes.push_back(DescriptorUse(0, 1, value_data->resource));

	//is a contiguous set of bindings (index can't be repeated)
	for (index = 1; index < textures.size(); index++) {
		writes.push_back(DescriptorUse(index, 1, textures.at(index)->resource));
	}
	for (; index < textureArrays.size(); index++) {
		writes.push_back(DescriptorUse(index, 1, textureArrays.at(index)->resource));
	}
	//writes.push_back(DescriptorUse(0, 1, uniformBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, materialBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	descriptor.UpdateDescriptorSet(descriptorSet, writes);
}

void VulkanMaterial::Bind(VkCommandBuffer cmdBuf) {



}