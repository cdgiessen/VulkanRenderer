#pragma once

#include <array>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>


#include <vulkan/vulkan.h>

#include "../core/CoreTools.h"
#include "../util/ConcurrentQueue.h"
#include "../util/PackedArray.h"

#include "Buffer.h"
#include "Descriptor.h"
#include "Device.h"
#include "FrameGraph.h"
#include "Pipeline.h"
#include "RenderStructs.h"
#include "RenderTools.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "Wrappers.h"


#include <glm/gtc/quaternion.hpp>

class Window;
class Scene;

namespace Resource
{
class ResourceManager;
}

struct Transform
{
	glm::vec3 pos = glm::vec3 (0, 0, 0);
	glm::vec3 scale = glm::vec3 (1, 1, 1);
	glm::quat rot = glm::quat (1, 0, 0, 0);
};


enum class RenderableType
{
	opaque,
	transparent,
	post_process,
	overlay
};

// struct RendererData
// {
// 	PackedArrayPool<VulkanModel> models;
// 	PackedArrayPool<VulkanMaterial> materials;
// 	PackedArrayPool<VulkanMaterialInstance> materialInstance;
// 	PackedArrayPool<Transform> transforms;
// };


// class RenderableModel
// {
// 	VulkanModel* model;
// 	VulkanMaterialInstance* material;
// 	Transform* transform;
// };

class RenderSettings
{
	public:
	// Lighting?
	int cameraCount = 1;
	int directionalLightCount = 5;
	int pointLightCount = 16;
	int spotLightCount = 8;

	bool memory_dump = false;

	RenderSettings (std::string fileName);

	void Load ();
	void Save ();

	private:
	std::string fileName;
};

struct

    class VulkanRenderer
{
	public:
	VulkanRenderer (bool enableValidationLayer, Window& window, Resource::ResourceManager& resourceMan);

	VulkanRenderer (const VulkanRenderer& other) = delete; // copy
	VulkanRenderer (VulkanRenderer&& other) = delete;      // move
	VulkanRenderer& operator= (const VulkanRenderer&) = delete;
	VulkanRenderer& operator= (VulkanRenderer&&) = delete;
	~VulkanRenderer ();


	void UpdateRenderResources (GlobalData globalData,
	    CameraData cameraData,
	    std::vector<DirectionalLight> directionalLights,
	    std::vector<PointLight> pointLights,
	    std::vector<SpotLight> spotLights);
	void RenderFrame ();

	void RecreateSwapChain ();

	void ContrustFrameGraph ();

	void CreateDepthResources ();
	void CreatePresentResources ();

	void PrepareDepthPass (int curFrameIndex);
	void SubmitDepthPass (int curFrameIndex);

	// void BuildDepthPass (VkCommandBuffer cmdBuf);
	// void BuildCommandBuffers (VkCommandBuffer cmdBuf);

	void PrepareFrame (int curFrameIndex);
	void SubmitFrame (int curFrameIndex);

	void PrepareResources ();

	std::shared_ptr<VulkanDescriptor> GetVulkanDescriptor ();
	void AddGlobalLayouts (std::vector<VkDescriptorSetLayout>& layouts);
	std::vector<VkDescriptorSetLayout> GetGlobalLayouts ();

	// std::vector<DescriptorPoolSize> GetGlobalPoolSize(int poolSize = 1);
	// std::vector<DescriptorUse> GetGlobalDescriptorUses();
	// DescriptorUse GetLightingDescriptorUses(uint32_t binding);

	VkFormat FindSupportedFormat (
	    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat ();

	void SubmitWork (WorkType workType,
	    std::function<void(const VkCommandBuffer)> work,
	    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores,
	    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores,
	    std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean,
	    std::vector<Signal> signals);

	VkCommandBuffer GetGraphicsCommandBuffer ();
	void SubmitGraphicsCommandBufferAndWait (VkCommandBuffer buffer);

	void SaveScreenshotNextFrame ();
	void ToggleWireframe ();

	void DeviceWaitTillIdle ();

	RenderSettings settings;

	VulkanDevice device;

	public:
	VulkanSwapChain vulkanSwapChain;
	std::unique_ptr<FrameGraph> frameGraph;
	VkRenderPass GetRelevantRenderpass (RenderableType type);


	ShaderManager shaderManager;
	// VulkanPipelineManager pipelineManager;
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

	std::unique_ptr<VulkanBufferUniformPersistant> transformDataBuffer;

	std::unique_ptr<VulkanDescriptor> frameDataDescriptor;
	std::unique_ptr<VulkanDescriptor> lightingDescriptor;

	DescriptorSet frameDataDescriptorSet;
	DescriptorSet lightingDescriptorSet;

	VkPipelineLayout frameDataDescriptorLayout;
	VkPipelineLayout lightingDescriptorLayout;

	std::unique_ptr<VulkanDescriptor> dynamicTransformDescriptor;
	DescriptorSet dynamicTransformDescriptorSet;
	std::unique_ptr<VulkanBufferUniformDynamic> dynamicTransformBuffer;

	std::array<std::shared_ptr<VulkanTexture>, 3> depthBuffer;

	int workerThreadCount = 3;

	ConcurrentQueue<GraphicsWork> workQueue;
	std::vector<GraphicsCleanUpWork> finishQueue;
	std::mutex finishQueueLock;
	std::vector<std::unique_ptr<GraphicsCommandWorker>> graphicsWorkers;

	// CommandBufferWorkQueue<CommandBufferWork> graphicsSetupWorkQueue;

	// CommandBufferWorkQueue<TransferCommandWork> transferWorkQueue;
	// std::vector<std::unique_ptr<CommandBufferWorker<TransferCommandWork>>> transferWorkers;

	uint32_t frameIndex = 0; // which of the swapchain images the app is rendering to
	bool wireframe = false;  // whether or not to use the wireframe pipeline for the scene.
	bool saveScreenshot = false;

	VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.1f, 1.0f } };
	// VkClearColorValue clearColor = {{ 0.2f, 0.3f, 0.3f, 1.0f }};
	VkClearDepthStencilValue depthClearColor = { 0.0f, 0 };

	std::array<VkClearValue, 2> GetFramebufferClearValues ();

	void SetupGlobalDescriptorSet ();
	void UpdateGlobalDescriptorSet ();

	void SetupLightingDescriptorSet ();
	void SaveScreenshot ();
};
