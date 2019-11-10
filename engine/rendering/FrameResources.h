#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "Wrappers.h"

class VulkanDevice;
class VulkanSwapChain;

class FrameObject
{
	public:
	FrameObject (VulkanDevice& device, VulkanSwapChain& swapChain, int frameD);
	~FrameObject ();

	VkResult AcquireNextSwapchainImage ();

	void PrepareFrame ();
	void Submit ();

	VkResult Present ();

	VkCommandBuffer GetPrimaryCmdBuf ();

	private:
	VulkanDevice& device;
	VulkanSwapChain& swapchain;
	uint32_t frameIndex; // which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	VulkanFence commandFence;
	CommandPool commandPool;
	CommandBuffer primary_command_buffer;

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
};