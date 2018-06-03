#include "Descriptor.h"

#include "Device.h"
#include "../rendering/Initializers.h"

DescriptorPoolSize::DescriptorPoolSize(VkDescriptorType type, uint32_t count) : type(type), count(count) {
};

VkDescriptorPoolSize DescriptorPoolSize::GetPoolSize() {
	return initializers::descriptorPoolSize(type, count);
};

DescriptorResource::DescriptorResource(VkDescriptorType type) : type(type) {

}
void DescriptorResource::FillResource(
	VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;
	info = bufferInfo;
}

void DescriptorResource::FillResource(
	VkSampler sampler, VkImageView imageView, VkImageLayout layout)
{
	info = initializers::descriptorImageInfo(sampler, imageView, layout);
}

DescriptorUse::DescriptorUse(uint32_t bindPoint, uint32_t count, DescriptorResource resource)
	: bindPoint(bindPoint), count(count), resource(resource)
{

}


VkWriteDescriptorSet DescriptorUse::GetWriteDescriptorSet(VkDescriptorSet set) {
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = set;
	writeDescriptorSet.descriptorType = resource.type;
	writeDescriptorSet.dstBinding = bindPoint;
	writeDescriptorSet.descriptorCount = count;
	
	if (resource.info.index() == 0)
		writeDescriptorSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&resource.info);
	else
		writeDescriptorSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&resource.info);
	
	return writeDescriptorSet;
}

void DescriptorSet::BindDescriptorSet(VkCommandBuffer cmdBuf, VkPipelineLayout layout) {
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
}

VulkanDescriptor::VulkanDescriptor(VulkanDevice& device) : device(device) {



}

void VulkanDescriptor::CleanUpResources() {
	vkDestroyDescriptorSetLayout(device.device, layout, nullptr);
	vkDestroyDescriptorPool(device.device, pool, nullptr);
}

void VulkanDescriptor::SetupLayout(std::vector<VkDescriptorSetLayoutBinding> bindings)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo =
		initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(device.device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}

}

void VulkanDescriptor::SetupPool(std::vector<DescriptorPoolSize> poolSizes, int maxSets) {

	std::vector<VkDescriptorPoolSize> poolMembers;
	for (auto& member : poolSizes) {
		poolMembers.push_back(member.GetPoolSize());
	}

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(
		static_cast<uint32_t>(poolMembers.size()), poolMembers.data(), maxSets);

	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device.device, &poolInfo, nullptr,
		&pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}


VkDescriptorSetLayoutBinding VulkanDescriptor::CreateBinding(VkDescriptorType type, VkShaderStageFlags stages, uint32_t binding, uint32_t descriptorCount) {

	return initializers::descriptorSetLayoutBinding(type, stages, binding, descriptorCount);
}

DescriptorSet VulkanDescriptor::CreateDescriptorSet() {
	DescriptorSet set;
	VkDescriptorSetLayout layouts[] = { layout };
	VkDescriptorSetAllocateInfo allocInfo =
		initializers::descriptorSetAllocateInfo(pool, layouts, 1);

	if (vkAllocateDescriptorSets(device.device, &allocInfo,	&(set.set)) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	return set;
}

void VulkanDescriptor::UpdateDescriptorSet(DescriptorSet set, std::vector<DescriptorUse> descriptors) {

	std::vector<VkWriteDescriptorSet> writes;
	for (auto& descriptor : descriptors) {
		writes.push_back(descriptor.GetWriteDescriptorSet(set.set));
	}

	vkUpdateDescriptorSets(device.device,
		static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

}

void VulkanDescriptor::FreeDescriptorSet(DescriptorSet set) {

	vkFreeDescriptorSets(device.device, pool, 1, &set.set);
}

VkDescriptorSetLayout VulkanDescriptor::GetLayout() {
	return layout;
}