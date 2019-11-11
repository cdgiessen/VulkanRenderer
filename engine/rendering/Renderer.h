#pragma once

#include <array>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <vulkan/vulkan.h>

#include "core/JobSystem.h"

#include "AsyncTask.h"
#include "Buffer.h"
#include "Descriptor.h"
#include "Device.h"
#include "DoubleBuffer.h"
#include "FrameGraph.h"
#include "FrameResources.h"
#include "Lighting.h"
#include "Model.h"
#include "Pipeline.h"
#include "RenderStructs.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "ViewCamera.h"
#include "Wrappers.h"

class Window;

namespace Resource
{
class ResourceManager;
}


class RenderSettings
{
	public:
	int max_directional_lights = 5;
	int max_point_lights = 16;
	int max_spot_lights = 8;

	bool memory_dump = false;

	RenderSettings (std::filesystem::path fileName);

	void Load ();
	void Save ();

	private:
	std::filesystem::path fileName;
};

class VulkanRenderer
{
	public:
	VulkanRenderer (
	    bool enableValidationLayer, job::TaskManager& task_manager, Window& window, Resource::ResourceManager& resource_man);

	VulkanRenderer (const VulkanRenderer& other) = delete; // copy
	VulkanRenderer (VulkanRenderer&& other) = delete;      // move
	VulkanRenderer& operator= (const VulkanRenderer&) = delete;
	VulkanRenderer& operator= (VulkanRenderer&&) = delete;
	~VulkanRenderer ();

	void RecreateSwapChain ();

	void RenderFrame ();

	void ToggleWireframe ();

	void DeviceWaitTillIdle ();

	private:
	job::TaskManager& task_manager;

	RenderSettings settings;

	VulkanDevice device;
	VulkanSwapChain vulkanSwapChain;

	AsyncTaskManager async_task_manager;
	DescriptorManager descriptor_manager;
	ShaderManager shader_manager;
	PipelineManager pipeline_manager;
	ModelManager model_manager;
	TextureManager texture_manager;

	ViewCameraManager camera_manager;
	LightingManager lighting_manager;
	// GPU_DoubleBuffer dynamic_data;

	std::unique_ptr<FrameGraph> frameGraph;

	std::vector<std::unique_ptr<FrameObject>> frameObjects;


	uint32_t frameIndex = 0; // which of the swapchain images the app is rendering to
	bool wireframe = false;  // whether or not to use the wireframe pipeline for the scene.

	void ContrustFrameGraph ();

	void PrepareFrame (int curFrameIndex);
	void SubmitFrame (int curFrameIndex);
};
