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

#include "core/CoreTools.h"
#include "util/ConcurrentQueue.h"
#include "util/PackedArray.h"

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
class AssetManager;
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


class RenderSettings
{
	public:
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

class GPU_DoubleBuffer
{
	public:
	GPU_DoubleBuffer (VulkanDevice& device, RenderSettings& settings);

	~GPU_DoubleBuffer ();

	void Update (GlobalData& globalData,
	    std::vector<CameraData>& cameraData,
	    std::vector<DirectionalLight>& directionalLights,
	    std::vector<PointLight>& pointLights,
	    std::vector<SpotLight>& spotLights);

	int CurIndex ();
	void AdvanceFrameCounter ();

	VkDescriptorSetLayout GetFrameDataDescriptorLayout ();
	VkDescriptorSetLayout GetLightingDescriptorLayout ();

	void BindFrameDataDescriptorSet (int index, VkCommandBuffer cmdBuf);
	void BindLightingDataDescriptorSet (int index, VkCommandBuffer cmdBuf);

	private:
	struct Dynamic
	{
		std::unique_ptr<VulkanBufferUniformPersistant> globalVariableBuffer;
		std::unique_ptr<VulkanBufferUniformArrayPersistant<CameraData>> cameraDataBuffer;
		std::unique_ptr<VulkanBufferUniformArrayPersistant<DirectionalLight>> sunBuffer;
		std::unique_ptr<VulkanBufferUniformArrayPersistant<PointLight>> pointLightsBuffer;
		std::unique_ptr<VulkanBufferUniformArrayPersistant<SpotLight>> spotLightsBuffer;

		std::unique_ptr<VulkanBufferUniformDynamic> dynamicTransformBuffer;

		DescriptorSet frameDataDescriptorSet;
		DescriptorSet lightingDescriptorSet;

		DescriptorSet dynamicTransformDescriptorSet;
	};

	std::unique_ptr<VulkanDescriptor> frameDataDescriptor;
	std::unique_ptr<VulkanDescriptor> lightingDescriptor;

	std::unique_ptr<VulkanDescriptor> dynamicTransformDescriptor;

	VkPipelineLayout frameDataDescriptorLayout;
	VkPipelineLayout lightingDescriptorLayout;


	VulkanDevice& device;

	std::array<Dynamic, 2> d_buffers;

	int cur_index = 0;
};

class VulkanRenderer
{
	public:
	VulkanRenderer (bool enableValidationLayer, Window& window, Resource::AssetManager& resourceMan);

	VulkanRenderer (const VulkanRenderer& other) = delete; // copy
	VulkanRenderer (VulkanRenderer&& other) = delete;      // move
	VulkanRenderer& operator= (const VulkanRenderer&) = delete;
	VulkanRenderer& operator= (VulkanRenderer&&) = delete;
	~VulkanRenderer ();


	void UpdateRenderResources (GlobalData& globalData,
	    std::vector<CameraData>& cameraData,
	    std::vector<DirectionalLight>& directionalLights,
	    std::vector<PointLight>& pointLights,
	    std::vector<SpotLight>& spotLights);
	void RenderFrame ();

	void RecreateSwapChain ();

	void ContrustFrameGraph ();

	void CreateDepthResources ();
	void CreatePresentResources ();

	void PrepareFrame (int curFrameIndex);
	void SubmitFrame (int curFrameIndex);

	std::shared_ptr<VulkanDescriptor> GetVulkanDescriptor ();
	void AddGlobalLayouts (std::vector<VkDescriptorSetLayout>& layouts);
	std::vector<VkDescriptorSetLayout> GetGlobalLayouts ();


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

	void ToggleWireframe ();

	void DeviceWaitTillIdle ();

	RenderSettings settings;

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;
	std::unique_ptr<FrameGraph> frameGraph;
	VkRenderPass GetRelevantRenderpass (RenderableType type);

	ShaderManager shaderManager;
	VulkanTextureManager textureManager;

	Scene* scene;

	private:
	CommandPool graphicsPrimaryCommandPool;
	CommandPool transferPrimaryCommandPool;
	CommandPool computePrimaryCommandPool;

	std::vector<std::unique_ptr<FrameObject>> frameObjects;

	std::vector<std::shared_ptr<VulkanDescriptor>> descriptors;

	GPU_DoubleBuffer dynamic_data;

	std::vector<std::shared_ptr<VulkanTexture>> depthBuffers;

	std::vector<GraphicsCleanUpWork> finishQueue;
	std::mutex finishQueueLock;

	uint32_t frameIndex = 0; // which of the swapchain images the app is rendering to
	bool wireframe = false;  // whether or not to use the wireframe pipeline for the scene.
};
