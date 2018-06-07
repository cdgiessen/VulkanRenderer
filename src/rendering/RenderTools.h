#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "../core/Logger.h"

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

struct VmaBuffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
	VmaAllocator* allocator = nullptr;
};

struct VmaImage {
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo;
	VmaAllocator* allocator = nullptr;
};

VkShaderModule loadShaderModule(VkDevice device, const std::string& codePath);

void setTransferBarrier(VkCommandBuffer cmdbuffer, VkBuffer bufferr,
	VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

void SetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange,
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

// Fixed sub resource on first mip level and layer
void SetImageLayout(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

void SetMemoryBarrier(
	VkCommandBuffer cmdBuffer,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkPipelineStageFlags srcStageFlags,
	VkPipelineStageFlags dstStageFlags
);

/** @brief Returns an error code as a string */
std::string errorString(const VkResult errorCode);

#define VK_CHECK_RESULT(f)																										\
{																																\
	VkResult res = (f);																											\
	if (res != VK_SUCCESS)																										\
	{																															\
		Log::Debug << "Fatal : VkResult is \"" << errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n";	\
		Log::Error << "Fatal : VkResult is \"" << errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n";	\
		assert(res == VK_SUCCESS);																								\
	}																															\
}

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000
