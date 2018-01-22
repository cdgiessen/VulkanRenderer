#include "VulkanDescriptor.hpp"


#include "../rendering/VulkanInitializers.hpp"
#include <vulkan/vulkan.h>

DescriptorResource::DescriptorResource( VkDescriptorType type,
	VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	: type(type)
{
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;
}

DescriptorResource::DescriptorResource(VkDescriptorType type,
	VkSampler sampler, VkImageView imageView, VkImageLayout layout)
	: type(type)
{
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = layout;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;
	info = imageInfo;

}

Descriptor::Descriptor(uint32_t bindPoint, uint32_t count, DescriptorResource resource)
	: bindPoint(bindPoint), count(count), resource(resource)
{

}


VkWriteDescriptorSet Descriptor::GetWriteDescriptorSet(VkDescriptorSet set) {
	if (resource.info.index() == 0)
		return initializers::writeDescriptorSet(set, resource.type, bindPoint, std::get_if<VkDescriptorBufferInfo>(&resource.info), count);
	else
		return initializers::writeDescriptorSet(set, resource.type, bindPoint, std::get_if<VkDescriptorImageInfo>(&resource.info), count);
}



VulkanDescriptor::VulkanDescriptor(VulkanDevice& device) : device(device) {



}


VkDescriptorSetLayoutBinding CreateBinding(VkDescriptorType type, VkShaderStageFlags stages, uint32_t binding, uint32_t descriptorCount) {

	return initializers::descriptorSetLayoutBinding(type, stages, binding, descriptorCount);
}

void VulkanDescriptor::SetupLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.push_back(CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1));
	bindings.push_back(CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1));
	bindings.push_back(CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	bindings.push_back(CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));

	VkDescriptorSetLayoutCreateInfo layoutInfo =
		initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(device.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

}

void VulkanDescriptor::SetupPool() {


	// setup pool
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);

	if (vkCreateDescriptorPool(device.device, &poolInfo, nullptr,
		&pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanDescriptor::SetupDescriptorSet(std::vector<std::shared_ptr<Descriptor>> descriptors) {

	// setup descriptor set
	VkDescriptorSetLayout layouts[] = { layout };
	VkDescriptorSetAllocateInfo allocInfo =
		initializers::descriptorSetAllocateInfo(pool, layouts, 1);

	if (vkAllocateDescriptorSets(device.device, &allocInfo,	&set) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	std::vector<VkWriteDescriptorSet> writes;
	for (auto des : descriptors) {
		writes.push_back(des->GetWriteDescriptorSet(set));
	}

	vkUpdateDescriptorSets(device.device,
		static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanDescriptor::BindDescriptorSet(VkCommandBuffer cmdBuf, VkPipelineLayout layout) {
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
}