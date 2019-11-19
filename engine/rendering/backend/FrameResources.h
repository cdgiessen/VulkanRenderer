#pragma once

#include <memory>

#include <vulkan/vulkan.h>

#include "Wrappers.h"

class VulkanDevice;
class VulkanSwapChain;

class FrameObject
{
	public:
	FrameObject (VulkanDevice& device, VulkanSwapChain& swapChain);
	~FrameObject ();
	FrameObject (FrameObject const& fb) = delete;
	FrameObject& operator= (FrameObject const& fb) = delete;
	FrameObject (FrameObject&& fb);
	FrameObject& operator= (FrameObject&& fb);


	VkResult AcquireNextSwapchainImage ();

	void PrepareFrame ();
	void Submit ();

	VkResult Present ();

	VkCommandBuffer GetPrimaryCmdBuf ();

	private:
	VulkanDevice* device;
	VulkanSwapChain* swapchain;
	uint32_t swapChainIndex; // which frame to render to

	VulkanSemaphore imageAvailSem;
	VulkanSemaphore renderFinishSem;

	CommandPool commandPool;
	CommandBuffer primary_command_buffer;

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
};