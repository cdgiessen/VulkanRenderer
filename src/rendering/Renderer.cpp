#include "Renderer.h"

#include <fstream>
#include <iomanip>

#include "Initializers.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include "stb_image/stb_image_write.h"

#include "gui/ImGuiImpl.h"

#include "core/Logger.h"
#include "core/Window.h"

#include "scene/Scene.h"

#include "resources/ResourceManager.h"

#include <nlohmann/json.hpp>

RenderSettings::RenderSettings (std::string fileName) : fileName (fileName) { Load (); }

void RenderSettings::Load ()
{
	if (fileExists (fileName))
	{
		try
		{

			std::ifstream input (fileName);
			nlohmann::json j;
			input >> j;


			memory_dump = j["memory_dump_on_exit"];
			directionalLightCount = j["directional_light_count"];
			pointLightCount = j["point_light_count"];
			spotLightCount = j["spot_light_count"];
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

	j["directional_light_count"] = directionalLightCount;
	j["point_light_count"] = pointLightCount;
	j["spot_light_count"] = spotLightCount;

	std::ofstream outFile (fileName);
	outFile << std::setw (4) << j;
	outFile.close ();
}

VulkanRenderer::VulkanRenderer (bool validationLayer, Window& window, Resource::AssetManager& resourceMan)

: settings ("render_settings.json"),
  device (validationLayer, window),
  vulkanSwapChain (device, window),
  shaderManager (device),
  textureManager (*this, resourceMan.texManager),
  graphicsPrimaryCommandPool (device, device.GraphicsQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  transferPrimaryCommandPool (device, device.TransferQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  computePrimaryCommandPool (device, device.ComputeQueue (), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
  dynamic_data (device, settings)

{
	for (size_t i = 0; i < vulkanSwapChain.swapChainImages.size (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, i));
	}

	CreatePresentResources ();

	PrepareImGui (&window, this);
}

VulkanRenderer::~VulkanRenderer ()
{
	DeviceWaitTillIdle ();
	ImGui_ImplGlfwVulkan_Shutdown ();

	if (settings.memory_dump) device.LogMemory ();
}

void VulkanRenderer::DeviceWaitTillIdle () { vkDeviceWaitIdle (device.device); }

void VulkanRenderer::UpdateRenderResources (GlobalData& globalData,
    std::vector<CameraData>& cameraData,
    std::vector<DirectionalLight>& directionalLights,
    std::vector<PointLight>& pointLights,
    std::vector<SpotLight>& spotLights)
{
	dynamic_data.Update (globalData, cameraData, directionalLights, pointLights, spotLights);
}

VkRenderPass VulkanRenderer::GetRelevantRenderpass (RenderableType type)
{
	return frameGraph->Get (0);

	switch (type)
	{
		case (RenderableType::opaque):
			return frameGraph->Get (0);
		case (RenderableType::transparent):
			return frameGraph->Get (1);
		case (RenderableType::post_process):
			return frameGraph->Get (2);
		case (RenderableType::overlay):
			return frameGraph->Get (3);
	}
	return nullptr;
}

void VulkanRenderer::RenderFrame ()
{
	PrepareFrame (frameIndex);
	frameGraph->FillCommandBuffer (frameObjects.at (frameIndex)->GetPrimaryCmdBuf (),
	    vulkanSwapChain.swapChainFramebuffers[frameIndex],
	    { 0, 0 },
	    vulkanSwapChain.swapChainExtent);
	SubmitFrame (frameIndex);

	frameIndex = (frameIndex + 1) % frameObjects.size ();
	dynamic_data.AdvanceFrameCounter ();

	std::lock_guard<std::mutex> lk (finishQueueLock);

	for (auto it = finishQueue.begin (); it != finishQueue.end ();)
	{
		if (it->cmdBuf.GetFence ().Check ())
		{
			for (auto& sig : it->signals)
			{
				if (sig != nullptr) *sig = true;
			}
			it->cmdBuf.Free ();
			it = finishQueue.erase (it);
		}
		else
		{
			++it;
		}
	}
}

void VulkanRenderer::CreatePresentResources ()
{
	ContrustFrameGraph ();

	CreateDepthResources ();
	std::vector<VkImageView> depthImageViews;
	for (auto& d : depthBuffers)
		depthImageViews.push_back (d->textureImageView);

	auto imageViewOrder = frameGraph->OrderAttachments ({ "img_depth", "img_color" });

	vulkanSwapChain.CreateFramebuffers (
	    imageViewOrder, depthImageViews, GetRelevantRenderpass (RenderableType::opaque));
}

void VulkanRenderer::RecreateSwapChain ()
{
	Log.Debug (fmt::format ("Recreating Swapchain\n"));

	depthBuffers.clear ();

	frameGraph.reset ();

	frameIndex = 0;
	frameObjects.clear ();
	vulkanSwapChain.RecreateSwapChain ();

	for (size_t i = 0; i < vulkanSwapChain.swapChainImages.size (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, i));
	}

	CreatePresentResources ();
}


void VulkanRenderer::ToggleWireframe () { wireframe = !wireframe; }

void VulkanRenderer::CreateDepthResources ()
{
	VkFormat depthFormat = FindDepthFormat ();
	TexCreateDetails texCreateDetails;
	texCreateDetails.format = depthFormat;
	texCreateDetails.desiredWidth = vulkanSwapChain.swapChainExtent.width;
	texCreateDetails.desiredHeight = vulkanSwapChain.swapChainExtent.height;

	for (int i = 0; i < 3; i++)
	{
		depthBuffers.push_back (std::make_unique<VulkanTexture> (*this, texCreateDetails));
	}
}

VkFormat VulkanRenderer::FindSupportedFormat (
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties (device.physical_device.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error ("failed to find supported format!");
}

VkFormat VulkanRenderer::FindDepthFormat ()
{
	return FindSupportedFormat ({ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanRenderer::ContrustFrameGraph ()
{
	FrameGraphBuilder frame_graph_builder;

	RenderPassDescription main_work ("main_work");

	frame_graph_builder.AddAttachment (RenderPassAttachment ("img_color", vulkanSwapChain.swapChainImageFormat));
	frame_graph_builder.AddAttachment (RenderPassAttachment ("img_depth", FindDepthFormat ()));

	SubpassDescription color_subpass ("sub_color");
	color_subpass.AddColorOutput ("img_color");
	color_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	color_subpass.AddClearColor ("img_color", { 0.1f, 0.1f, 0.1f, 1.0f });
	color_subpass.AddClearColor ("img_depth", { 0.0f, 0 });
	main_work.AddSubpass (color_subpass);

	// SubpassDescription transparent_subpass ("sub_trans");
	// transparent_subpass.AddSubpassDependency ("sub_color");
	// transparent_subpass.SetDepthStencil ("img_depth",
	// SubpassDescription::DepthStencilAccess::read_only); transparent_subpass.AddColorOutput
	// ("img_color"); main_work.AddSubpass (transparent_subpass);


	frame_graph_builder.AddRenderPass (main_work);
	frame_graph_builder.lastPass = main_work.name;

	frameGraph = std::make_unique<FrameGraph> (frame_graph_builder, device);

	auto main_draw = [&](VkCommandBuffer cmdBuf) {
		dynamic_data.BindFrameDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);
		dynamic_data.BindLightingDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);

		VkViewport viewport = initializers::viewport (
		    (float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
		vkCmdSetViewport (cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = initializers::rect2D (
		    vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, 0, 0);
		vkCmdSetScissor (cmdBuf, 0, 1, &scissor);

		scene->RenderOpaque (cmdBuf, wireframe);
		scene->RenderTransparent (cmdBuf, wireframe);

		scene->RenderSkybox (cmdBuf);

		ImGui_ImplGlfwVulkan_Render (cmdBuf);
	};

	/*auto transparent_draw = [&](VkCommandBuffer cmdBuf) {
	    dynamic_data.BindFrameDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);
	    dynamic_data.BindLightingDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);

	    VkViewport viewport = initializers::viewport (
	        (float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
	    vkCmdSetViewport (cmdBuf, 0, 1, &viewport);

	    VkRect2D scissor = initializers::rect2D (
	        vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, 0, 0);
	    vkCmdSetScissor (cmdBuf, 0, 1, &scissor);

	    scene->RenderTransparent (cmdBuf, wireframe);
	};*/

	frameGraph->SetDrawFuncs (0, { main_draw });
	// frameGraph->SetDrawFuncs (0, { transparent_draw });
}

void VulkanRenderer::PrepareFrame (int curFrameIndex)
{
	VkResult result = frameObjects.at (curFrameIndex)->AcquireNextSwapchainImage (vulkanSwapChain.swapChain);

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
	frameObjects.at (curFrameIndex)->Submit (device.GraphicsQueue ());

	VkResult result = frameObjects.at (curFrameIndex)->Present (vulkanSwapChain, device.PresentQueue ());

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain ();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to present swap chain image!");
	}

	device.PresentQueue ().QueueWaitIdle ();
}

// TODO: refactor this out
std::shared_ptr<VulkanDescriptor> VulkanRenderer::GetVulkanDescriptor ()
{
	return std::make_shared<VulkanDescriptor> (device);
}

void VulkanRenderer::AddGlobalLayouts (std::vector<VkDescriptorSetLayout>& layouts)
{
	layouts.push_back (dynamic_data.GetFrameDataDescriptorLayout ());
	layouts.push_back (dynamic_data.GetLightingDescriptorLayout ());
}

std::vector<VkDescriptorSetLayout> VulkanRenderer::GetGlobalLayouts ()
{
	return { dynamic_data.GetFrameDataDescriptorLayout (), dynamic_data.GetLightingDescriptorLayout () };
}

void VulkanRenderer::SubmitWork (WorkType workType,
    std::function<void(const VkCommandBuffer)> work,
    std::vector<std::shared_ptr<VulkanSemaphore>> waitSemaphores,
    std::vector<std::shared_ptr<VulkanSemaphore>> signalSemaphores,
    std::vector<std::shared_ptr<VulkanBuffer>> buffersToClean,
    std::vector<Signal> signals)
{
	CommandBuffer cmdBuf = CommandBuffer (graphicsPrimaryCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	std::shared_ptr<VulkanFence> fence = std::make_shared<VulkanFence> (device);

	// Can't copy fence, so can't reassign CommandBuffer...
	switch (workType)
	{
		case (WorkType::graphics):
			cmdBuf = CommandBuffer (graphicsPrimaryCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			break;
		case (WorkType::transfer):
			cmdBuf = CommandBuffer (transferPrimaryCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			break;
		case (WorkType::compute):
			cmdBuf = CommandBuffer (computePrimaryCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			break;
	}

	cmdBuf.Allocate ();
	cmdBuf.Begin (VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	cmdBuf.WriteTo (work);
	cmdBuf.SetFence (fence);
	cmdBuf.End ();
	cmdBuf.Submit (waitSemaphores, signalSemaphores);

	{
		std::lock_guard<std::mutex> lk (finishQueueLock);
		finishQueue.push_back (GraphicsCleanUpWork (cmdBuf, buffersToClean, signals));
	}
}


VkCommandBuffer VulkanRenderer::GetGraphicsCommandBuffer ()
{

	return graphicsPrimaryCommandPool.GetCommandBuffer (VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void VulkanRenderer::SubmitGraphicsCommandBufferAndWait (VkCommandBuffer commandBuffer)
{
	if (commandBuffer == VK_NULL_HANDLE) return;

	VulkanFence fence = VulkanFence (device);
	graphicsPrimaryCommandPool.ReturnCommandBuffer (commandBuffer, fence);

	fence.Wait ();

	graphicsPrimaryCommandPool.FreeCommandBuffer (commandBuffer);
}

void InsertImageMemoryBarrier (VkCommandBuffer cmdbuffer,
    VkImage image,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier imageMemoryBarrier = initializers::imageMemoryBarrier ();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier (cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

GPU_DoubleBuffer::GPU_DoubleBuffer (VulkanDevice& device, RenderSettings& settings)
: device (device)
{
	for (auto& data : d_buffers)
	{
		auto globalDetails = uniform_details (sizeof (GlobalData));
		globalDetails.persistentlyMapped = true;

		auto cameraDetails = uniform_details (sizeof (CameraData) * settings.cameraCount);
		auto dirLightDetails = uniform_details (sizeof (DirectionalLight) * settings.directionalLightCount);
		auto pointLightDetails = uniform_details (sizeof (PointLight) * settings.pointLightCount);
		auto spotLightDetails = uniform_details (sizeof (SpotLight) * settings.spotLightCount);

		data.globalVariableBuffer = std::make_unique<VulkanBuffer> (device, globalDetails);

		data.cameraDataBuffer = std::make_unique<VulkanBuffer> (device, cameraDetails);
		data.sunBuffer = std::make_unique<VulkanBuffer> (device, dirLightDetails);
		data.pointLightsBuffer = std::make_unique<VulkanBuffer> (device, pointLightDetails);
		data.spotLightsBuffer = std::make_unique<VulkanBuffer> (device, spotLightDetails);


		auto transformDetails = uniform_dynamic_details (MaxTransformCount * sizeof (TransformMatrixData));
		data.dynamicTransformBuffer = std::make_unique<VulkanBuffer> (device, transformDetails);
	}

	// Frame data
	{
		frameDataDescriptor = std::make_unique<VulkanDescriptor> (device);

		std::vector<VkDescriptorSetLayoutBinding> m_bindings;
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 0, 1));
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1, 1));
		frameDataDescriptor->SetupLayout (m_bindings);

		std::vector<DescriptorPoolSize> poolSizes;
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2));
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2));
		frameDataDescriptor->SetupPool (poolSizes, 2);
		for (auto& data : d_buffers)
		{
			data.frameDataDescriptorSet = frameDataDescriptor->CreateDescriptorSet ();

			std::vector<DescriptorUse> writes;
			writes.push_back (DescriptorUse (0, 1, data.globalVariableBuffer->resource));
			writes.push_back (DescriptorUse (1, 1, data.cameraDataBuffer->resource));
			frameDataDescriptor->UpdateDescriptorSet (data.frameDataDescriptorSet, writes);
		}
		auto desLayout = frameDataDescriptor->GetLayout ();
		auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo (&desLayout, 1);

		if (vkCreatePipelineLayout (device.device, &pipelineLayoutInfo, nullptr, &frameDataDescriptorLayout) != VK_SUCCESS)
		{
			throw std::runtime_error ("failed to create pipeline layout!");
		}
	}
	// Lighting
	{
		lightingDescriptor = std::make_unique<VulkanDescriptor> (device);

		std::vector<VkDescriptorSetLayoutBinding> m_bindings;
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1));
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
		lightingDescriptor->SetupLayout (m_bindings);

		std::vector<DescriptorPoolSize> poolSizes;
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2));
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2));
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2));
		lightingDescriptor->SetupPool (poolSizes, 2);
		for (auto& data : d_buffers)
		{
			data.lightingDescriptorSet = lightingDescriptor->CreateDescriptorSet ();

			std::vector<DescriptorUse> writes;
			writes.push_back (DescriptorUse (0, 1, data.sunBuffer->resource));
			writes.push_back (DescriptorUse (1, 1, data.pointLightsBuffer->resource));
			writes.push_back (DescriptorUse (2, 1, data.spotLightsBuffer->resource));
			lightingDescriptor->UpdateDescriptorSet (data.lightingDescriptorSet, writes);
		}

		std::vector<VkDescriptorSetLayout> layouts;
		layouts.push_back (frameDataDescriptor->GetLayout ());
		layouts.push_back (lightingDescriptor->GetLayout ());

		auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo (layouts.data (), 2);

		if (vkCreatePipelineLayout (device.device, &pipelineLayoutInfo, nullptr, &lightingDescriptorLayout) != VK_SUCCESS)
		{
			throw std::runtime_error ("failed to create pipeline layout!");
		}
	}
	// Transformation Matrices -- Dynamic Uniform Buffer
	{

		dynamicTransformDescriptor = std::make_unique<VulkanDescriptor> (device);
		std::vector<VkDescriptorSetLayoutBinding> m_bindings;
		m_bindings.push_back (VulkanDescriptor::CreateBinding (
		    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
		dynamicTransformDescriptor->SetupLayout (m_bindings);

		std::vector<DescriptorPoolSize> poolSizes;
		poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2));
		dynamicTransformDescriptor->SetupPool (poolSizes, 2);
		for (auto& data : d_buffers)
		{
			data.dynamicTransformDescriptorSet = dynamicTransformDescriptor->CreateDescriptorSet ();

			std::vector<DescriptorUse> writes;
			writes.push_back (DescriptorUse (0, 1, data.dynamicTransformBuffer->resource));
			dynamicTransformDescriptor->UpdateDescriptorSet (data.dynamicTransformDescriptorSet, writes);
		}
	}
}

GPU_DoubleBuffer::~GPU_DoubleBuffer ()
{
	vkDestroyPipelineLayout (device.device, frameDataDescriptorLayout, nullptr);
	vkDestroyPipelineLayout (device.device, lightingDescriptorLayout, nullptr);
}

void GPU_DoubleBuffer::Update (GlobalData& globalData,
    std::vector<CameraData>& cameraData,
    std::vector<DirectionalLight>& directionalLights,
    std::vector<PointLight>& pointLights,
    std::vector<SpotLight>& spotLights)
{
	d_buffers.at (cur_index).globalVariableBuffer->CopyToBuffer (&globalData, sizeof (GlobalData));
	d_buffers.at (cur_index).cameraDataBuffer->CopyToBuffer (
	    cameraData.data (), cameraData.size () * sizeof (CameraData));
	d_buffers.at (cur_index).sunBuffer->CopyToBuffer (
	    directionalLights.data (), directionalLights.size () * sizeof (DirectionalLight));
	d_buffers.at (cur_index).pointLightsBuffer->CopyToBuffer (
	    pointLights.data (), pointLights.size () * sizeof (PointLight));
	d_buffers.at (cur_index).spotLightsBuffer->CopyToBuffer (
	    spotLights.data (), spotLights.size () * sizeof (SpotLight));
}


int GPU_DoubleBuffer::CurIndex () { return cur_index; }

void GPU_DoubleBuffer::AdvanceFrameCounter ()
{
	cur_index = (cur_index + 1) % 2; // alternate between 0 & 1
}

VkDescriptorSetLayout GPU_DoubleBuffer::GetFrameDataDescriptorLayout ()
{
	return frameDataDescriptor->GetLayout ();
}
VkDescriptorSetLayout GPU_DoubleBuffer::GetLightingDescriptorLayout ()
{
	return lightingDescriptor->GetLayout ();
}

void GPU_DoubleBuffer::BindFrameDataDescriptorSet (int index, VkCommandBuffer cmdBuf)
{
	vkCmdBindDescriptorSets (cmdBuf,
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    frameDataDescriptorLayout,
	    0,
	    1,
	    &d_buffers.at (index).frameDataDescriptorSet.set,
	    0,
	    nullptr);
}
void GPU_DoubleBuffer::BindLightingDataDescriptorSet (int index, VkCommandBuffer cmdBuf)
{
	vkCmdBindDescriptorSets (cmdBuf,
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    lightingDescriptorLayout,
	    1,
	    1,
	    &d_buffers.at (index).lightingDescriptorSet.set,
	    0,
	    nullptr);
}
