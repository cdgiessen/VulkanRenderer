#pragma once

#include <vector>
#include <string>
#include <variant>
#include <memory>

#include <vulkan/vulkan.h>

// foward declarations
class VulkanDevice;

class DescriptorPoolSize {
public:
	DescriptorPoolSize(VkDescriptorType type, uint32_t count);


	VkDescriptorPoolSize GetPoolSize();
	
	VkDescriptorType type;
	uint32_t count;
};

class DescriptorResource {
public:
	std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> info;
	VkDescriptorType type;

	DescriptorResource(VkDescriptorType type);

	void FillResource(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	void FillResource(VkSampler sampler, VkImageView imageView, VkImageLayout layout);

};

class DescriptorUse {
public:
	DescriptorUse(uint32_t bindPoint, uint32_t count, DescriptorResource resource);

	VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorSet set);

private:
	uint32_t bindPoint;
	uint32_t count;

	DescriptorResource resource;
};

class DescriptorSet {
public:
	VkDescriptorSet set;

	void BindDescriptorSet(VkCommandBuffer cmdBuf, VkPipelineLayout layout);
};

class VulkanDescriptor {
public:
	VulkanDescriptor(VulkanDevice& device);
	
	void SetupLayout(std::vector<VkDescriptorSetLayoutBinding> bindings);
	void SetupPool(std::vector<DescriptorPoolSize> poolSizes, int maxSets = 1);
	
	void CleanUpResources();

	static VkDescriptorSetLayoutBinding CreateBinding(VkDescriptorType type, VkShaderStageFlags stages, uint32_t binding, uint32_t descriptorCount);

	DescriptorSet CreateDescriptorSet();
	void UpdateDescriptorSet(DescriptorSet set, std::vector<DescriptorUse> descriptors);
	void FreeDescriptorSet(DescriptorSet set);

	VkDescriptorSetLayout GetLayout();

private:
	VulkanDevice& device;
	
	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
};

