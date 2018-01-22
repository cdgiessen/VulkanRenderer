#pragma once

#include <vector>
#include <string>
#include <variant>
#include <memory>

#include <vulkan/vulkan.h>

#include "VulkanDevice.hpp"

struct DescriptorResource {
	std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo> info;
	VkDescriptorType type;

	DescriptorResource(VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
	DescriptorResource(VkDescriptorType type, VkSampler sampler, VkImageView imageView, VkImageLayout layout);

};

class Descriptor {
public:
	Descriptor(uint32_t bindPoint, uint32_t count, DescriptorResource resource);

	VkWriteDescriptorSet GetWriteDescriptorSet(VkDescriptorSet set);

private:
	uint32_t bindPoint;
	uint32_t count;

	DescriptorResource resource;
};

class VulkanDescriptor {
public:
	VulkanDescriptor(VulkanDevice& device);

	void SetupLayout();
	void SetupPool();
	void SetupDescriptorSet(std::vector < std::shared_ptr<Descriptor>> descriptors);


	void BindDescriptorSet(VkCommandBuffer cmdBuf, VkPipelineLayout layout);

private:
	VulkanDevice & device;

	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
};

