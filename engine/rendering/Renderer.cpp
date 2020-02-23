#include "Renderer.h"

#include <fstream>
#include <iomanip>

#include "imgui/imgui.h"
#include "stb/stb_image_write.h"
#include <nlohmann/json.hpp>

#include "core/Logger.h"
#include "core/Window.h"
#include "resources/Resource.h"

#include "FrameGraph.h"

#include "Initializers.h"
#include "backend/ImGuiImpl.h"
#include "backend/ImGuiImplGLFW.h"
#include "backend/RenderTools.h"


RenderSettings::RenderSettings (std::filesystem::path fileName) : fileName (fileName) { Load (); }

void RenderSettings::Load ()
{
	if (std::filesystem::exists (fileName))
	{
		try
		{
			std::ifstream input (fileName);
			nlohmann::json j;
			input >> j;


			memory_dump = j["memory_dump_on_exit"];
			max_directional_lights = j["max_directional_lights"];
			max_point_lights = j["max_point_lights"];
			max_spot_lights = j["max_spot_lights"];
		}
		catch (std::runtime_error& e)
		{
			Log.Debug (fmt::format ("Render Settings was incorrect, recreating: {}", e.what ()));
			Save ();
		}
	}
	else
	{
		Log.Debug (fmt::format ("Render Settings not found, creating one"));
		Save ();
	}
}
void RenderSettings::Save ()
{
	nlohmann::json j;

	j["memory_dump_on_exit"] = memory_dump;

	j["max_directional_lights"] = max_directional_lights;
	j["max_point_lights"] = max_point_lights;
	j["max_spot_lights"] = max_spot_lights;

	std::ofstream outFile (fileName);
	outFile << std::setw (4) << j;
	outFile.close ();
}

VulkanRenderer::VulkanRenderer (
    bool validationLayer, job::TaskManager& task_manager, Window& window, Resource::ResourceManager& resource_man)

: settings ("render_settings.json"),
  task_manager (task_manager),
  back_end (validationLayer, task_manager, window, resource_man),
  lighting_manager (back_end.device),
  camera_manager (back_end.device),
  mesh_manager (back_end)

{
	frameObjects.reserve (back_end.vulkanSwapChain.GetChainCount ());
	for (size_t i = 0; i < back_end.vulkanSwapChain.GetChainCount (); i++)
	{
		frameObjects.emplace_back (back_end.device, back_end.vulkanSwapChain);
	}

	ContrustFrameGraph ();

	ImGuiSetup ();
}

VulkanRenderer::~VulkanRenderer ()
{
	back_end.async_task_manager.CleanFinishQueue ();
	if (settings.memory_dump) back_end.device.LogMemory ();

	DeviceWaitTillIdle ();
	ImGuiShutdown ();
}

void VulkanRenderer::DeviceWaitTillIdle () { vkDeviceWaitIdle (back_end.device.device); }

void VulkanRenderer::RenderFrame ()
{
	VkResult result = frameObjects.at (frameIndex).AcquireNextSwapchainImage ();
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain ();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error ("failed to acquire swap chain image!");
	}

	frameObjects.at (frameIndex).PrepareFrame ();


	ImGui::Render ();

	frameGraph->SetCurrentFrameIndex (frameIndex);
	frameGraph->FillCommandBuffer (frameObjects.at (frameIndex).GetPrimaryCmdBuf (), "main_work");
	frameObjects.at (frameIndex).Submit ();

	result = frameObjects.at (frameIndex).Present ();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// RecreateSwapChain ();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to present swap chain image!");
	}
	frameIndex = (frameIndex + 1) % frameObjects.size ();

	back_end.async_task_manager.CleanFinishQueue ();
}

void VulkanRenderer::RecreateSwapChain ()
{
	Log.Debug (fmt::format ("Recreating Swapchain"));
	DeviceWaitTillIdle ();

	frameGraph->DestroyPresentResources ();
	back_end.vulkanSwapChain.RecreateSwapChain ();
	frameGraph->CreatePresentResources ();
	ImGui_ImplVulkan_SetMinImageCount (back_end.vulkanSwapChain.GetChainCount ());
}


void VulkanRenderer::ToggleWireframe () { wireframe = !wireframe; }

void VulkanRenderer::ContrustFrameGraph ()
{
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work ("main_work");

	frame_graph_builder.AddAttachment ({ "img_color", back_end.vulkanSwapChain.GetFormat () });
	frame_graph_builder.SetFinalOutputAttachmentName ("img_color");
	frame_graph_builder.AddAttachment ({ "img_depth",
	    back_end.device.FindSupportedFormat ({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	        VK_IMAGE_TILING_OPTIMAL,
	        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) });

	SubpassDescription color_subpass ("sub_color");
	color_subpass.AddColorOutput ("img_color");
	color_subpass.AddClearColor ("img_color", { 0.1f, 0.1f, 0.1f, 1.0f });
	color_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	color_subpass.AddClearColor ("img_depth", { 0.0f, 0 });

	color_subpass.SetFunction (std::move ([&] (VkCommandBuffer cmdBuf) {
		/*
		dynamic_data.BindFrameDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);
		dynamic_data.BindLightingDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);

		    VkViewport viewport = initializers::viewport (
		        (float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
		    vkCmdSetViewport (cmdBuf, 0, 1, &viewport);

		    VkRect2D scissor = initializers::rect2D (
		        vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, 0, 0);
		    vkCmdSetScissor (cmdBuf, 0, 1, &scissor);

		    // scene->RenderOpaque (cmdBuf, wireframe);
		    // scene->RenderTransparent (cmdBuf, wireframe);
		    // scene->RenderSkybox (cmdBuf);
		*/
		auto draw_data = ImGui::GetDrawData ();
		if (draw_data)
		{
			ImGui_ImplVulkan_RenderDrawData (draw_data, cmdBuf);
		}
	}));
	main_work.AddSubpass (color_subpass);

	frame_graph_builder.AddRenderPass (main_work);
	frame_graph_builder.SetFinalRenderPassName (main_work.name);

	frameGraph = std::make_unique<FrameGraph> (back_end.device, back_end.vulkanSwapChain, frame_graph_builder);
}


void VulkanRenderer::ImGuiSetup ()
{
	ImGui::CreateContext ();

	ImGui_ImplGlfw_InitForVulkan (back_end.device.GetWindow ().getWindowContext (), false);

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
	VkDescriptorPoolCreateInfo pool_info = initializers::descriptorPoolCreateInfo (pool_sizes, 1000);
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	VK_CHECK_RESULT (vkCreateDescriptorPool (back_end.device.device, &pool_info, nullptr, &imgui_pool));

	ImGui_ImplVulkan_InitInfo info{};
	info.Instance = back_end.device.instance.instance;
	info.PhysicalDevice = back_end.device.physical_device.physical_device;
	info.Device = back_end.device.device;
	info.QueueFamily = back_end.device.GraphicsQueue ().GetQueueFamily ();
	info.Queue = back_end.device.GraphicsQueue ().GetQueue ();
	info.PipelineCache = back_end.pipeline_manager.GetCache ();
	info.MinImageCount = 2;
	info.ImageCount = back_end.vulkanSwapChain.GetChainCount ();
	info.DescriptorPool = imgui_pool;

	ImGui_ImplVulkan_Init (&info, frameGraph->GetPresentRenderPass ());

	CommandPool font_pool (back_end.device.device, back_end.device.GraphicsQueue ());
	CommandBuffer cmd_buf (font_pool);
	cmd_buf.Allocate ().Begin ();
	ImGui_ImplVulkan_CreateFontsTexture (cmd_buf.Get ());
	cmd_buf.End ().Submit ().Wait ();
	ImGui_ImplVulkan_DestroyFontUploadObjects ();
}

void VulkanRenderer ::ImGuiShutdown ()
{
	vkDestroyDescriptorPool (back_end.device.device, imgui_pool, nullptr);

	ImGui_ImplGlfw_Shutdown ();
	ImGui_ImplVulkan_Shutdown ();
}

void VulkanRenderer ::ImGuiNewFrame ()
{
	ImGui_ImplGlfw_NewFrame ();
	ImGui_ImplVulkan_NewFrame ();
	ImGui::NewFrame ();
}