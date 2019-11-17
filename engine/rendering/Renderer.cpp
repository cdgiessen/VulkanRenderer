#include "Renderer.h"

#include <fstream>
#include <iomanip>

#include "Initializers.h"

#include "stb/stb_image_write.h"

#include "ImGuiImpl.h"

#include "core/Logger.h"
#include "core/Window.h"

#include "resources/Resource.h"

#include <nlohmann/json.hpp>

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
			Log.Debug (fmt::format ("Render Settings was incorrect, recreating\n"));
			Save ();
		}
	}
	else
	{
		Log.Debug (fmt::format ("Render Settings not found, creating one\n"));
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
  device (window, validationLayer),
  vulkanSwapChain (device, window),
  async_task_manager (task_manager, device),
  descriptor_manager (device),
  shader_manager (resource_man.shader_manager, device),
  pipeline_manager (device, async_task_manager),
  model_manager (resource_man.mesh_manager, device, async_task_manager),
  texture_manager (resource_man.texture_manager, device, async_task_manager),
  lighting_manager (device),
  camera_manager (device)
// dynamic_data (device, descriptor_manager, settings)

{
	for (size_t i = 0; i < vulkanSwapChain.GetChainCount (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, vulkanSwapChain, i));
	}

	ContrustFrameGraph ();


	PrepareImGui (&window, &device, frameGraph->GetPresentRenderPass ());
}

VulkanRenderer::~VulkanRenderer ()
{
	async_task_manager.CleanFinishQueue ();

	DeviceWaitTillIdle ();
	ImGui_ImplGlfwVulkan_Shutdown ();

	if (settings.memory_dump) device.LogMemory ();
}

void VulkanRenderer::DeviceWaitTillIdle () { vkDeviceWaitIdle (device.device); }

// VkRenderPass VulkanRenderer::GetRelevantRenderpass (RenderableType type)
// {
// 	return frameGraph->Get (0);
// }

void VulkanRenderer::RenderFrame ()
{
	PrepareFrame (frameIndex);
	frameGraph->SetCurrentFrameIndex (frameIndex);
	frameGraph->FillCommandBuffer (frameObjects.at (frameIndex)->GetPrimaryCmdBuf (), "main_work");
	// frameGraph->FillCommandBuffer (frameObjects.at (frameIndex)->GetPrimaryCmdBuf (),
	//    vulkanSwapChain.swapChainFramebuffers[frameIndex],
	//    { 0, 0 },
	//    vulkanSwapChain.swapChainExtent);
	SubmitFrame (frameIndex);

	frameIndex = (frameIndex + 1) % frameObjects.size ();
	// dynamic_data.AdvanceFrameCounter ();

	async_task_manager.CleanFinishQueue ();
}

void VulkanRenderer::RecreateSwapChain ()
{
	Log.Debug (fmt::format ("Recreating Swapchain\n"));
	DeviceWaitTillIdle ();

	frameIndex = 0;
	vulkanSwapChain.RecreateSwapChain ();
	frameGraph->RecreatePresentResources ();

	frameObjects.clear ();
	for (size_t i = 0; i < vulkanSwapChain.GetChainCount (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, vulkanSwapChain, i));
	}
}


void VulkanRenderer::ToggleWireframe () { wireframe = !wireframe; }

void VulkanRenderer::ContrustFrameGraph ()
{
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work ("main_work");

	frame_graph_builder.AddAttachment ({ "img_color", vulkanSwapChain.GetFormat () });
	frame_graph_builder.SetFinalOutputAttachmentName ("img_color");
	frame_graph_builder.AddAttachment ({ "img_depth",
	    device.FindSupportedFormat ({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	        VK_IMAGE_TILING_OPTIMAL,
	        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) });

	SubpassDescription color_subpass ("sub_color");
	color_subpass.AddColorOutput ("img_color");
	color_subpass.AddClearColor ("img_color", { 0.1f, 0.1f, 0.1f, 1.0f });
	color_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	color_subpass.AddClearColor ("img_depth", { 0.0f, 0 });

	color_subpass.SetFunction (std::move ([&](VkCommandBuffer cmdBuf) {
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
		ImGui_ImplGlfwVulkan_Render (cmdBuf);
	}));
	main_work.AddSubpass (color_subpass);

	frame_graph_builder.AddRenderPass (main_work);
	frame_graph_builder.SetFinalRenderPassName (main_work.name);

	frameGraph = std::make_unique<FrameGraph> (device, vulkanSwapChain, frame_graph_builder);
}

void VulkanRenderer::PrepareFrame (int curFrameIndex)
{
	VkResult result = frameObjects.at (curFrameIndex)->AcquireNextSwapchainImage ();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || (result == VK_SUBOPTIMAL_KHR))
	{
		RecreateSwapChain ();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error ("failed to acquire swap chain image!");
	}

	frameObjects.at (curFrameIndex)->PrepareFrame ();
}

void VulkanRenderer::SubmitFrame (int curFrameIndex)
{
	frameObjects.at (curFrameIndex)->Submit ();

	VkResult result = frameObjects.at (curFrameIndex)->Present ();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain ();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to present swap chain image!");
	}

	// device.PresentQueue ().QueueWaitIdle ();
}
