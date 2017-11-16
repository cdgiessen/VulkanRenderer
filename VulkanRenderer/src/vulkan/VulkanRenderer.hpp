#pragma once

#include "vulkan\vulkan.h"

#include "VulkanDevice.hpp"
#include "VulkanInitializers.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanShader.hpp"
#include "VulkanSwapChain.hpp"

class Scene;

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	void InitVulkanRenderer(GLFWwindow* window);
	void CleanVulkanResources();

	void InitSwapchain();
	void ReInitSwapchain(std::shared_ptr<Scene> scene, bool wireframe);
	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateDepthResources();
	void CreateFramebuffers();

	void BuildCommandBuffers(std::shared_ptr<Scene> scene, bool wireframe);
	void ReBuildCommandBuffers(std::shared_ptr<Scene> scene, bool wireframe);
	
	void CreatePrimaryCommandBuffer(); //testing out multiple command buffers

	void CreateCommandBuffers();
	void CreateSemaphores();

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VulkanPipeline pipelineManager;
	VulkanShader shaderManager;

	//uint32_t frameIndex = 1; // which frame of the swapchain it is on

	//Depth buffer
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	//Command buffer per frame
	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
};

