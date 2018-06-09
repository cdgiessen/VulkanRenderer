#pragma once

#include <array>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <vulkan/vulkan.h>

#include "../core/CoreTools.h"

#include "RenderTools.h"
#include "RenderStructs.h"
#include "Initializers.h"
#include "Device.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "Descriptor.h"

////Custom Rendering Implementation
//class RenderableCustomImpl {
//public:
//	RenderableCustomImpl(std::function<void(VkCommandBuffer cmdBuff, bool wireframe)> callable);
//	void WriteToCmdBuffer(VkCommandBuffer cmdBuff, bool wireframe);
//
//};
//
//class RenderableGameObject {
//public:
//	RenderableGameObject(std::shared_ptr<ManagedVulkanPipeline> pipeline, std::shared_ptr<VulkanModel> model);
//	void WriteToCmdBuffer(VkCommandBuffer cmdBuff, bool wireframe);
//private:
//	std::shared_ptr<ManagedVulkanPipeline> mvp;
//	std::shared_ptr<VulkanModel> model;
//};

class Scene;

class RenderSettings {
public:
	int graphicsSetupWorkerCount = 1;
	int transferWorkerCount = 1;

	//Lighting?
	int cameraCount = 1;
	int directionalLightCount = 5;
	int pointLightCount = 16;
	int spotLightCount = 8;

	RenderSettings(std::string fileName);

	void Load();
	void Save();
private:
	std::string fileName;
};

class VulkanRenderer
{
public:
	VulkanRenderer(bool enableValidationLayer, GLFWwindow *window);
	
	VulkanRenderer(const VulkanRenderer& other) = delete; //copy
	VulkanRenderer(VulkanRenderer&& other) = delete; //move
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(VulkanRenderer&&) = delete;
	~VulkanRenderer();

	//void LoadRenderSettings();
	//void SaveRenderSettings();

	void UpdateRenderResources(GlobalData globalData,
		CameraData cameraData,
		std::vector<DirectionalLight> directionalLights,
		std::vector<PointLight> pointLights,
		std::vector<SpotLight> spotLights);
	void RenderFrame();

	//void ReloadRenderer(GLFWwindow *window);

	void CreateWorkerThreads();
	void DestroyWorkerThreads();

	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateDepthResources();

	void BuildCommandBuffers();
	void ReBuildCommandBuffers();

	void CreatePrimaryCommandBuffer(); //testing out multiple command buffers

	void CreateCommandBuffers();
	//void CreateSemaphores();

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

	void SubmitGraphicsSetupWork(CommandBufferWork&& data);
	void SubmitTransferWork(TransferCommandWork&& data);

	VkCommandBuffer GetGraphicsCommandBuffer();
	VkCommandBuffer GetSingleUseGraphicsCommandBuffer();
	void SubmitGraphicsCommandBufferAndWait(VkCommandBuffer buffer);

	void SaveScreenshotNextFrame();
	void SetWireframe(bool wireframe);

	void DeviceWaitTillIdle();

	RenderSettings settings;

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	VkRenderPass renderPass;

	VulkanShader shaderManager;
	VulkanPipeline pipelineManager;
	VulkanTextureManager textureManager;

	Scene* scene;

private:
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

	std::unique_ptr<VulkanTextureDepthBuffer> depthBuffer;

	CommandBufferWorkQueue<CommandBufferWork> graphicsSetupWorkQueue;
	std::vector<std::unique_ptr<CommandBufferWorker<CommandBufferWork>>> graphicsSetupWorkers;

	CommandBufferWorkQueue<TransferCommandWork> transferWorkQueue;
	std::vector<std::unique_ptr<CommandBufferWorker<TransferCommandWork>>> transferWorkers;

	CommandPool graphicsPrimaryCommandPool;

	//Command buffer per frame
	std::vector<VkCommandBuffer> commandBuffers;

	std::unique_ptr<VulkanSemaphore> imageAvailableSemaphore;
	std::unique_ptr<VulkanSemaphore> renderFinishedSemaphore;

	std::vector<std::shared_ptr<VulkanDescriptor>> descriptors;

	uint32_t frameIndex = 0; //which of the swapchain images the app is rendering to
	bool wireframe = false; //whether or not to use the wireframe pipeline for the scene.
	bool saveScreenshot = false;

	VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.1f, 1.0f } };
	//VkClearColorValue clearColor = {{ 0.2f, 0.3f, 0.3f, 1.0f }};
	VkClearDepthStencilValue depthClearColor = { 0.0f, 0 };

	std::array<VkClearValue, 2> GetFramebufferClearValues();

	void SaveScreenshot();
};

