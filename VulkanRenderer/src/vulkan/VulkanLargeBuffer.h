#pragma once

#include <vulkan\vulkan.h>

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

class VulkanLargeBuffer
{
public:
	VulkanLargeBuffer(VulkanDevice* device, VkBufferUsageFlagBits usageFlags, VkDeviceSize size);
	~VulkanLargeBuffer();

	VkDeviceMemory* StageResource();

	bool TransferBuffers();

private:
	VulkanDevice *device;

	VulkanBuffer buffer;

	VkMemoryPropertyFlags memoryPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
};

