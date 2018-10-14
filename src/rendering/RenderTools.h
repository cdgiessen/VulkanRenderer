#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "../core/Logger.h"

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

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

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDeviceProperties properties);

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