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

#include "backend/BackEnd.h"

#include "FrameGraph.h"

#include "Lighting.h"
#include "ViewCamera.h"

#include "rendering/renderers/MeshRenderer.h"
#include "rendering/renderers/SkyboxRenderer.h"
#include "rendering/renderers/TerrainRenderer.h"


class Window;

namespace Resource
{
class ResourceManager;
}


class FrameGraph;

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
	VulkanRenderer (bool enableValidationLayer,
	    job::TaskManager& task_manager,
	    Window& window,
	    Resource::ResourceManager& resource_man);

	VulkanRenderer (VulkanRenderer const& other) = delete; // copy
	VulkanRenderer& operator= (VulkanRenderer const&) = delete;
	VulkanRenderer (VulkanRenderer&& other) = delete; // move
	VulkanRenderer& operator= (VulkanRenderer&&) = delete;
	~VulkanRenderer ();

	void RecreateSwapChain ();

	void RenderFrame ();

	void DeviceWaitTillIdle ();

	void ImGuiNewFrame ();

	private:
	void ImGuiSetup ();
	void ImGuiShutdown ();

	private:
	job::TaskManager& task_manager;

	RenderSettings settings;

	BackEnd back_end;

	public:
	ViewCameraManager camera_manager;
	LightingManager lighting_manager;

	MeshManager mesh_manager;

	private:
	std::unique_ptr<FrameGraph> frameGraph;

	std::vector<FrameObject> frameObjects;


	uint32_t frameIndex = 0; // which of the swapchain images the app is rendering to

	void ContrustFrameGraph ();

	VkDescriptorPool imgui_pool;
};
