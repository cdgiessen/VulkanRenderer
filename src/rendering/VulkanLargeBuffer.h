#pragma once

#include <vulkan/vulkan.h>


  
  

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

class VulkanLargeBuffer
{
public:
	VulkanLargeBuffer(std::shared_ptr<VulkanDevice> device, VkBufferUsageFlagBits usageFlags, VkDeviceSize size);
	~VulkanLargeBuffer();

	VkDeviceMemory StageResource();

	bool TransferBuffers();

private:
	std::shared_ptr<VulkanDevice> device;

	VulkanBuffer buffer;
	VulkanBuffer stagingBuffer;

	VkMemoryPropertyFlags hostMemoryPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkMemoryPropertyFlags deviceMemoryPropertyFlags = (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
};

