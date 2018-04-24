#pragma once

#include <array>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>

#include "RenderTools.h"
#include "RenderStructs.h"
#include "Initializers.hpp"

#include "Device.hpp"
#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "Shader.hpp"
#include "SwapChain.hpp"
#include "Texture.hpp"
#include "Descriptor.hpp"

class Scene;

class VulkanRenderer
{
public:
	VulkanRenderer(bool enableValidationLayer, std::shared_ptr<Scene> scene);
	VulkanRenderer(const VulkanRenderer& other) = default; //copy
	VulkanRenderer(VulkanRenderer&& other) = default; //move
	VulkanRenderer& operator=(const VulkanRenderer&) = default;
	VulkanRenderer& operator=(VulkanRenderer&&) = default;
	~VulkanRenderer();

	void InitVulkanRenderer(GLFWwindow* window);
	void UpdateRenderResources(GlobalData globalData,
		CameraData cameraData, 
		std::vector<DirectionalLight> directionalLights, 
		std::vector<PointLight> pointLights,
		std::vector<SpotLight> spotLights);
	void RenderFrame();
	void CleanVulkanResources();

	//void InitSwapchain();
	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateDepthResources();

	void BuildCommandBuffers();
	void ReBuildCommandBuffers();
	
	void CreatePrimaryCommandBuffer(); //testing out multiple command buffers

	void CreateCommandBuffers();
	void CreateSemaphores();

	void PrepareFrame();
	void SubmitFrame();

	void PrepareResources();

	std::shared_ptr<VulkanDescriptor> GetVulkanDescriptor();
	void AddGlobalLayouts(std::vector<VkDescriptorSetLayout>& layouts);
	//std::vector<DescriptorPoolSize> GetGlobalPoolSize(int poolSize = 1);
	//std::vector<DescriptorUse> GetGlobalDescriptorUses();
	//DescriptorUse GetLightingDescriptorUses(uint32_t binding);

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	VkCommandBuffer GetGraphicsCommandBuffer();
	VkCommandBuffer GetSingleUseGraphicsCommandBuffer();
	void SubmitGraphicsCommandBufferAndWait(VkCommandBuffer buffer);

	VkCommandBuffer GetTransferCommandBuffer();

	void SubmitTransferCommandBufferAndWait(VkCommandBuffer buf);
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal);
	void SubmitTransferCommandBuffer(VkCommandBuffer buf, std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean);


	void SaveScreenshotNextFrame();
	void SetWireframe(bool wireframe);

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VulkanShader shaderManager;
	VulkanPipeline pipelineManager;
	VulkanTextureManager textureManager;

	std::shared_ptr<Scene> scene;

private:
	int cameraCount = 1;
	int directionalLightCount = 5;
	int pointLightCount = 16;
	int spotLightCount = 8;

	void SetupGlobalDescriptorSet();
	void SetupLightingDescriptorSet();

	VulkanBufferUniform globalVariableBuffer;
	VulkanBufferUniform cameraDataBuffer;
	VulkanBufferUniform sunBuffer;
	VulkanBufferUniform pointLightsBuffer;
	VulkanBufferUniform spotLightsBuffer;

	std::shared_ptr<VulkanDescriptor> frameDataDescriptor;
	std::shared_ptr<VulkanDescriptor> lightingDescriptor;

	DescriptorSet frameDataDescriptorSet;
	DescriptorSet lightingDescriptorSet;

	VkPipelineLayout frameDataDescriptorLayout;
	VkPipelineLayout lightingDescriptorLayout;

	VulkanBufferUniformDynamic entityPositions;

	//uint32_t frameIndex = 1; // which frame of the swapchain it is on

	std::shared_ptr<VulkanTextureDepthBuffer> depthBuffer;

	CommandPool graphicsPrimaryCommandPool;
	CommandPool singleUseGraphicsCommandPool;

	CommandPool computeCommandPool;
	CommandPool transferCommandPool;

	//Command buffer per frame
	std::vector<VkCommandBuffer> commandBuffers;

	//VkCommandPool graphics_queue_command_pool;
	//VkCommandPool compute_queue_command_pool;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	std::mutex presentMutex;


	std::vector<std::shared_ptr<VulkanDescriptor>> descriptors;

	uint32_t frameIndex; //which of the swapchain images the app is rendering to
	bool wireframe = false; //whether or not to use the wireframe pipeline for the scene.
	bool saveScreenshot = false;

	VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.1f, 1.0f } };
	//VkClearColorValue clearColor = {{ 0.2f, 0.3f, 0.3f, 1.0f }};
	VkClearDepthStencilValue depthClearColor = { 0.0f, 0 };

	std::array<VkClearValue, 2> GetFramebufferClearValues();

	void SaveScreenshot();
};

