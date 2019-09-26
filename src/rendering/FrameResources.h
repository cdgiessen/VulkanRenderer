#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "Device.h"
#include "SwapChain.h"
#include "Wrappers.h"

class FrameObject
{
	public:
	FrameObject (VulkanDevice& device, int frameD);
	~FrameObject ();

	VkResult AcquireNextSwapchainImage (VkSwapchainKHR swapchain);

	void PrepareFrame ();
	void Submit (CommandQueue& queue);

	VkResult Present (VulkanSwapChain& swapChain, CommandQueue& presentQueue);

	VkCommandBuffer GetPrimaryCmdBuf ();

	private:
	VulkanDevice& device;
	uint32_t frameIndex; // which frame in the queue it is

	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	std::shared_ptr<VulkanFence> commandFence;
	CommandPool commandPool;
	CommandBuffer primary_command_buffer;

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
};