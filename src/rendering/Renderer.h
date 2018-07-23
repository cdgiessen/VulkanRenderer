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
#include "../util/ConcurrentQueue.h"

#include "RenderTools.h"
#include "RenderStructs.h"
#include "Wrappers.h"
#include "Buffer.h"
#include "Device.h"
#include "SwapChain.h"
#include "Shader.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Descriptor.h"
#include "RenderPass.h"

class Window;
class Scene;

namespace Resource { class ResourceManager; }

class RenderSettings {
public:

	//Lighting?
	int cameraCount = 1;
	int directionalLightCount = 5;
	int pointLightCount = 16;
	int spotLightCount = 8;

	bool memory_dump = false;

	RenderSettings(std::string fileName);

	void Load();
	void Save();
private:
	std::string fileName;
};

class VulkanRenderer
{
public:
	VulkanRenderer(bool enableValidationLayer,
		Window& window, Resource::ResourceManager& resourceMan);

	VulkanRenderer(const VulkanRenderer& other) = delete; //copy
	VulkanRenderer(VulkanRenderer&& other) = delete; //move
	VulkanRenderer& operator=(const VulkanRenderer&) = delete;
	VulkanRenderer& operator=(VulkanRenderer&&) = delete;
	~VulkanRenderer();


	void UpdateRenderResources(GlobalData globalData,
		CameraData cameraData,
		std::vector<DirectionalLight> directionalLights,
		std::vector<PointLight> pointLights,
		std::vector<SpotLight> spotLights);
	void RenderFrame();

	//void ReloadRenderer(GLFWwindow *window);

	void RecreateSwapChain();

	void CreateRenderPass();
	void CreateDepthResources();

	void PrepareDepthPass(int curFrameIndex);
	void SubmitDepthPass(int curFrameIndex);

	void BuildDepthPass(VkCommandBuffer cmdBuf);
	void BuildCommandBuffers(VkCommandBuffer cmdBuf);

	void PrepareFrame(int curFrameIndex);
	void SubmitFrame(int curFrameIndex);

	void PrepareResources();

	std::shared_ptr<VulkanDescriptor> GetVulkanDescriptor();
	void AddGlobalLayouts(std::vector<VkDescriptorSetLayout>& layouts);
	//std::vector<DescriptorPoolSize> GetGlobalPoolSize(int poolSize = 1);
	//std::vector<DescriptorUse> GetGlobalDescriptorUses();
	//DescriptorUse GetLightingDescriptorUses(uint32_t binding);

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	void SubmitWork(WorkType workType,
		std::function<void(const VkCommandBuffer)> work,
		std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores,
		std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores,
		std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean,
		std::vector<Signal> signals);

	VkCommandBuffer GetGraphicsCommandBuffer();
	void SubmitGraphicsCommandBufferAndWait(VkCommandBuffer buffer);

	void SaveScreenshotNextFrame();
	void ToggleWireframe();

	void DeviceWaitTillIdle();

	RenderSettings settings;

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	std::unique_ptr<RenderPass> renderPass;

	ShaderManager shaderManager;
	VulkanPipeline pipelineManager;
	VulkanTextureManager textureManager;

	Scene* scene;

private:

	CommandPool graphicsPrimaryCommandPool;

	std::vector<std::unique_ptr<FrameObject>> frameObjects;

	std::vector<std::shared_ptr<VulkanDescriptor>> descriptors;

	std::unique_ptr<VulkanBufferUniformPersistant> globalVariableBuffer;
	std::unique_ptr<VulkanBufferUniformPersistant> cameraDataBuffer;
	std::unique_ptr<VulkanBufferUniformPersistant> sunBuffer;
	std::unique_ptr<VulkanBufferUniformPersistant> pointLightsBuffer;
	std::unique_ptr<VulkanBufferUniformPersistant> spotLightsBuffer;

	std::unique_ptr<VulkanDescriptor> frameDataDescriptor;
	std::unique_ptr<VulkanDescriptor> lightingDescriptor;

	DescriptorSet frameDataDescriptorSet;
	DescriptorSet lightingDescriptorSet;

	VkPipelineLayout frameDataDescriptorLayout;
	VkPipelineLayout lightingDescriptorLayout;

	std::unique_ptr<VulkanDescriptor> dynamicTransformDescriptor;
	DescriptorSet dynamicTransformDescriptorSet;
	std::unique_ptr<VulkanBufferUniformDynamic> dynamicTransformBuffer;

	std::shared_ptr<VulkanTexture> depthBuffer;

	int workerThreadCount = 3;

	ConcurrentQueue<GraphicsWork> workQueue;
	std::vector<GraphicsCleanUpWork> finishQueue;
	std::mutex finishQueueLock;
	std::vector < std::unique_ptr<GraphicsCommandWorker>> graphicsWorkers;

	//CommandBufferWorkQueue<CommandBufferWork> graphicsSetupWorkQueue;

	//CommandBufferWorkQueue<TransferCommandWork> transferWorkQueue;
	//std::vector<std::unique_ptr<CommandBufferWorker<TransferCommandWork>>> transferWorkers;

	uint32_t frameIndex = 0; //which of the swapchain images the app is rendering to
	bool wireframe = false; //whether or not to use the wireframe pipeline for the scene.
	bool saveScreenshot = false;

	VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.1f, 1.0f } };
	//VkClearColorValue clearColor = {{ 0.2f, 0.3f, 0.3f, 1.0f }};
	VkClearDepthStencilValue depthClearColor = { 0.0f, 0 };

	std::array<VkClearValue, 2> GetFramebufferClearValues();


	void SetupGlobalDescriptorSet();
	void SetupLightingDescriptorSet();
	void SaveScreenshot();
};

