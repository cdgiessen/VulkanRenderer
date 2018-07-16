#include "Material.h"

#include <numeric>
#include <algorithm>

#include "Initializers.h"

#include "../core/CoreTools.h"

#include <json.hpp>


VkDescriptorType GetVulkanDescriptorType(ResourceType type) {
	switch (type) {
	case(ResourceType::uniform): return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case(ResourceType::texture2D): return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case(ResourceType::textureArray): return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case(ResourceType::cubemap): return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}
}

VkShaderStageFlags GetVulkanShaderStageFlags(ResourceStages stage) {
	switch (stage) {
	case(ResourceStages::vertex_only): return VK_SHADER_STAGE_VERTEX_BIT;
	case(ResourceStages::fragment_only): return VK_SHADER_STAGE_FRAGMENT_BIT;
	case(ResourceStages::vertex_fragment): return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	}
}

VariableUniformController::VariableUniformController(
	std::vector<VariableUniformSlot> input) :
	data(input)
{

}

void VariableUniformController::Set(int8_t index, DataTypeVar var) {

}

//VariableUniformController::DataTypeVar VariableUniformController::Get(int8_t index) {
//	char* src = data.data() + offsets.at(index);
//	
//}


VulkanMaterial::VulkanMaterial(VulkanDevice& device)
	:device(device),
	descriptor(device)
{

}

VulkanMaterial::~VulkanMaterial() {

}

void VulkanMaterial::SetShaders(ShaderModuleSet set) {
	shaderModules = set;
}

void VulkanMaterial::AddTexture(std::shared_ptr<VulkanTexture> tex) {
	//textures.push_back(tex);
}

void VulkanMaterial::AddTextureArray(std::shared_ptr<VulkanTexture> texArr) {
	textureArrays.push_back(texArr);
}

void VulkanMaterial::AddValue(MaterialOptions value) {
	//value_var = value;

	//value_data.reset();
	/*value_data = std::make_shared<VulkanBufferUniform>(device);

	if (value.index() == 0) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(Phong_Material));
	}

	else if (value.index() == 1) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Mat_Value));
	}

	else if (value.index() == 2) {
		value_data->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Mat_Tex));
	}*/

}

void VulkanMaterial::AddMaterialDataSlot(MaterialDataSlot slot) {
	dataSlots.push_back(slot);
}


void VulkanMaterial::Setup() {
	//descriptor = renderer->GetVulkanDescriptor();
	//descriptor = VulkanDescriptor(device);

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	int index = 0;

	for (int i = 0; i < dataSlots.size(); i++) {
		m_bindings.push_back(VulkanDescriptor::CreateBinding(
			GetVulkanDescriptorType(dataSlots.at(i).type),
			GetVulkanShaderStageFlags(dataSlots.at(i).stage),
			i,
			dataSlots.at(i).count));
	}

	descriptor.SetupLayout(m_bindings);

	// m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));


	// //is a contiguous set of bindings (index can't be repeated)
	// for (; index < textures.size(); index++) {
	// 	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));
	// }
	// for (; index < textureArrays.size(); index++) {
	// 	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, index, 1));
	// }

	std::vector<DescriptorPoolSize> poolSizes;

	for (int i = 0; i < dataSlots.size(); i++) {
		poolSizes.push_back(DescriptorPoolSize(
			GetVulkanDescriptorType(dataSlots.at(i).type),
			dataSlots.at(i).count));
	}

	descriptor.SetupPool(poolSizes);

	// poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

	// for (int i = 0; i < textures.size(); i++) {
	// 	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	// }
	// for (int i = 0; i < textureArrays.size(); i++) {
	// 	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	// }

	descriptorSet = descriptor.CreateDescriptorSet();

	std::vector<DescriptorUse> writes;

	for (int i = 0; i < dataSlots.size(); i++) {
		writes.push_back(DescriptorUse(i, dataSlots.at(i).count, dataSlots.at(i).resource));

	}


	//writes.push_back(DescriptorUse(0, 1, value_data->resource));

	//is a contiguous set of bindings (index can't be repeated)
	// for (index = 1; index < textures.size(); index++) {
	// 	writes.push_back(DescriptorUse(index, 1, textures.at(index)->resource));
	// }
	// for (; index < textureArrays.size(); index++) {
	// 	writes.push_back(DescriptorUse(index, 1, textureArrays.at(index)->resource));
	// }
	//writes.push_back(DescriptorUse(0, 1, uniformBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, materialBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	descriptor.UpdateDescriptorSet(descriptorSet, writes);
}

VkDescriptorSetLayout VulkanMaterial::GetDescriptorSetLayout() {
	return descriptor.GetLayout();
}

void VulkanMaterial::Bind(VkCommandBuffer cmdBuf, VkPipelineLayout layout) {

	descriptorSet.BindDescriptorSet(cmdBuf, layout);

}