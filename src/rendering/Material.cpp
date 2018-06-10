#include "Material.h"

#include "Initializers.h"

#include "../core/CoreTools.h"

#include <json.hpp>

VulkanMaterial::VulkanMaterial(VulkanDevice& device) 
    :device(device),
    descriptor(device)
{



}

void VulkanMaterial::CleanUp(){
    descriptor.CleanUp();
}

//VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath) {
//	auto shaderCode = readFile(codePath);
//
//	VkShaderModuleCreateInfo createInfo = {};
//	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//	createInfo.codeSize = shaderCode.size();
//
//	std::vector<uint32_t> codeAligned(shaderCode.size() / 4 + 1);
//	memcpy(codeAligned.data(), shaderCode.data(), shaderCode.size());
//
//	createInfo.pCode = codeAligned.data();
//
//	VkShaderModule shaderModule;
//	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create shader module!");
//	}
//
//	return shaderModule;
//}



void VulkanMaterial::Setup(){
    //descriptor = renderer->GetVulkanDescriptor();
   // descriptor = VulkanDescriptor(device);

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	//m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));

	descriptor.SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	//poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor.SetupPool(poolSizes);

	descriptorSet = descriptor.CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	//writes.push_back(DescriptorUse(0, 1, uniformBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, materialBuffer->resource));
	//writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	descriptor.UpdateDescriptorSet(descriptorSet, writes);
}

void VulkanMaterial::Bind(VkCommandBuffer cmdBuf){



}