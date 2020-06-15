#include "Renderer.h"

#include <fstream>
#include <iomanip>

#include "imgui.hpp"
#include "stb_image_write.h"
#include <nlohmann/json.hpp>

#include "core/Logger.h"
#include "core/Window.h"
#include "resources/Resource.h"

#include "FrameGraph.h"

#include "Initializers.h"
#include "backend/ImGuiImpl.h"
#include "backend/ImGuiImplGLFW.h"
#include "backend/RenderTools.h"


RenderSettings::RenderSettings (std::filesystem::path file_name) : file_name (file_name)
{
	load ();
}

void RenderSettings::load ()
{
	if (std::filesystem::exists (file_name))
	{
		try
		{
			std::ifstream input (file_name);
			nlohmann::json j;
			input >> j;


			memory_dump = j["memory_dump_on_exit"];
		}
		catch (std::runtime_error& e)
		{
			Log.debug (fmt::format ("Render Settings was incorrect, recreating: {}", e.what ()));
			save ();
		}
	}
	else
	{
		Log.debug (fmt::format ("Render Settings not found, creating one"));
		save ();
	}
}
void RenderSettings::save ()
{
	nlohmann::json j;

	j["memory_dump_on_exit"] = memory_dump;

	std::ofstream outFile (file_name);
	outFile << std::setw (4) << j;
	outFile.close ();
}

VulkanRenderer::VulkanRenderer (
    bool validationLayer, job::ThreadPool& thread_pool, Window& window, Resource::Resources& resource_man)

: settings ("render_settings.json"),
  thread_pool (thread_pool),
  back_end (validationLayer, thread_pool, window, resource_man),
  render_cameras (back_end.device),
  frame_data (back_end.device),
  lighting (back_end.device, back_end.textures, frame_data),
  mesh_renderer (back_end)

{
	frame_objects.reserve (back_end.vulkanSwapChain.GetChainCount ());
	for (size_t i = 0; i < back_end.vulkanSwapChain.GetChainCount (); i++)
	{
		frame_objects.emplace_back (back_end.device, back_end.vulkanSwapChain);
	}

	construct_frame_graph ();

	imgui_setup ();
}

VulkanRenderer::~VulkanRenderer ()
{
	back_end.async_task_queue.CleanFinishQueue ();
	if (settings.memory_dump) back_end.device.LogMemory ();

	device_wait ();
	imgui_shutdown ();
}

void VulkanRenderer::device_wait () { vkDeviceWaitIdle (back_end.device.device); }

void VulkanRenderer::render_frame ()
{
	VkResult result = frame_objects.at (frame_index).AcquireNextSwapchainImage ();
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreate_swapchain ();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error ("failed to acquire swap chain image!");
	}

	frame_objects.at (frame_index).PrepareFrame ();


	ImGui::Render ();

	frame_graph->set_current_frame_index (frame_index);
	frame_graph->fill_command_buffer (frame_objects.at (frame_index).GetPrimaryCmdBuf (), "main_work");
	frame_objects.at (frame_index).submit ();

	result = frame_objects.at (frame_index).Present ();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreate_swapchain ();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to present swap chain image!");
	}
	frame_index = (frame_index + 1) % frame_objects.size ();

	back_end.async_task_queue.CleanFinishQueue ();
}

void VulkanRenderer::recreate_swapchain ()
{
	Log.debug (fmt::format ("Recreating Swapchain"));
	device_wait ();

	frame_graph->destroy_present_resources ();
	back_end.vulkanSwapChain.recreate_swapchain ();
	frame_graph->create_present_resources ();
	ImGui_ImplVulkan_SetMinImageCount (back_end.vulkanSwapChain.GetChainCount ());
}

void VulkanRenderer::construct_frame_graph ()
{
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work ("main_work");

	frame_graph_builder.add_attachment ({ "img_color", back_end.vulkanSwapChain.GetFormat () });
	frame_graph_builder.set_final_output_attachment_name ("img_color");
	frame_graph_builder.add_attachment ({ "img_depth",
	    back_end.device.find_supported_format ({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	        VK_IMAGE_TILING_OPTIMAL,
	        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) });

	SubpassDescription color_subpass ("sub_color");
	color_subpass.add_color_output ("img_color");
	color_subpass.add_clear_color ("img_color", { { 0.1f, 0.1f, 0.1f, 1.0f } });
	color_subpass.set_depth_stencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	color_subpass.add_clear_color ("img_depth", { { 0.0f, 0 } });

	color_subpass.set_function ([&] (VkCommandBuffer cmdBuf) { main_draw (cmdBuf); });
	main_work.add_subpass (color_subpass);

	frame_graph_builder.add_render_pass (main_work);
	frame_graph_builder.set_final_render_pass_name (main_work.name);

	frame_graph = std::make_unique<FrameGraph> (back_end.device, back_end.vulkanSwapChain, frame_graph_builder);
}

void VulkanRenderer::main_draw (VkCommandBuffer cmdBuf)
{

	auto extent = back_end.vulkanSwapChain.GetImageExtent ();
	VkViewport viewport = initializers::viewport (
	    static_cast<float> (extent.width), static_cast<float> (extent.height), 0.0f, 1.0f);
	vkCmdSetViewport (cmdBuf, 0, 1, &viewport);

	VkRect2D scissor = initializers::rect2D (extent.width, extent.height, 0, 0);
	vkCmdSetScissor (cmdBuf, 0, 1, &scissor);

	frame_data.bind (cmdBuf);
	lighting.bind (cmdBuf);

	auto draw_data = ImGui::GetDrawData ();
	if (draw_data)
	{
		ImGui_ImplVulkan_RenderDrawData (draw_data, cmdBuf);
	}
};

void VulkanRenderer::imgui_setup ()
{
	ImGui::CreateContext ();

	ImGui_ImplGlfw_InitForVulkan (back_end.device.get_window ().get_window_context (), false);

	std::vector<VkDescriptorPoolSize> pool_sizes = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };
	VkDescriptorPoolCreateInfo pool_info = initializers::descriptor_pool_create_info (pool_sizes, 1000);
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	VK_CHECK_RESULT (vkCreateDescriptorPool (back_end.device.device, &pool_info, nullptr, &imgui_pool));

	ImGui_ImplVulkan_InitInfo info{};
	info.Instance = back_end.device.vkb_instance.instance;
	info.PhysicalDevice = back_end.device.phys_device.physical_device;
	info.Device = back_end.device.device;
	info.QueueFamily = back_end.device.graphics_queue ().queue_family ();
	info.Queue = back_end.device.graphics_queue ().get ();
	info.PipelineCache = back_end.pipeline_cache.get ();
	info.MinImageCount = 2;
	info.ImageCount = back_end.vulkanSwapChain.GetChainCount ();
	info.DescriptorPool = imgui_pool;

	ImGui_ImplVulkan_Init (&info, frame_graph->get_present_render_pass ());

	CommandPool font_pool (back_end.device.device, back_end.device.graphics_queue ());
	CommandBuffer cmd_buf (font_pool);
	cmd_buf.allocate ().begin ();
	ImGui_ImplVulkan_CreateFontsTexture (cmd_buf.get ());
	cmd_buf.end ().submit ().wait ();
	ImGui_ImplVulkan_DestroyFontUploadObjects ();
}

void VulkanRenderer::imgui_shutdown ()
{
	vkDestroyDescriptorPool (back_end.device.device, imgui_pool, nullptr);

	ImGui_ImplGlfw_Shutdown ();
	ImGui_ImplVulkan_Shutdown ();
}

void VulkanRenderer::imgui_new_frame ()
{
	ImGui_ImplGlfw_NewFrame ();
	ImGui_ImplVulkan_NewFrame ();
	ImGui::NewFrame ();
}


VulkanOpenXRInit VulkanRenderer::get_openxr_init ()
{
	return VulkanOpenXRInit{ back_end.device.vkb_instance.instance,
		back_end.device.phys_device.physical_device,
		back_end.device.device,
		back_end.device.graphics_queue () };
}