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
#include "ViewCamera.h"
#include "FrameData.h"
#include "Lighting.h"

#include "rendering/renderers/MeshRenderer.h"
#include "rendering/renderers/SkyboxRenderer.h"
#include "rendering/renderers/TerrainRenderer.h"


class Window;

namespace Resource
{
class Resources;
}


class FrameGraph;

class RenderSettings
{
	public:
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
	    bool enableValidationLayer, job::ThreadPool& thread_pool, Window& window, Resource::Resources& resource_man);

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
	job::ThreadPool& thread_pool;

	RenderSettings settings;

	BackEnd back_end;

	public:
	RenderCameras render_cameras;
	FrameData frame_data;
	Lighting lighting;
	MeshRenderer mesh_renderer;

	private:
	std::unique_ptr<FrameGraph> frameGraph;

	std::vector<FrameObject> frameObjects;


	uint32_t frameIndex = 0; // which of the swapchain images the app is rendering to

	void ContrustFrameGraph ();

	VkDescriptorPool imgui_pool;

	// drawing functions
	void MainDraw (VkCommandBuffer cmdBuf);
};
