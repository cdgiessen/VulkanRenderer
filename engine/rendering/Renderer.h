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

#include "rendering/renderers/FrameData.h"
#include "rendering/renderers/Lighting.h"
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

	RenderSettings (std::filesystem::path file_name);

	void load ();
	void save ();

	private:
	std::filesystem::path file_name;
};

struct VulkanOpenXRInit
{
	VkInstance inst;
	VkPhysicalDevice phys_dev;
	VkDevice device;
	CommandQueue& queue;
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

	void recreate_swapchain ();

	void render_frame ();

	void device_wait ();

	void imgui_new_frame ();

	VulkanOpenXRInit get_openxr_init ();

	private:
	void imgui_setup ();
	void imgui_shutdown ();

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
	std::unique_ptr<FrameGraph> frame_graph;

	std::vector<FrameObject> frame_objects;


	uint32_t frame_index = 0; // which of the swapchain images the app is rendering to

	void construct_frame_graph ();

	VkDescriptorPool imgui_pool;

	// drawing functions
	void main_draw (VkCommandBuffer cmdBuf);
};
