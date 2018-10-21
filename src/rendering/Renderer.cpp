#include "Renderer.h"

#include <fstream>

#include "Initializers.h"

#include "../../third-party/VulkanMemoryAllocator/vk_mem_alloc.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third-party/stb_image/stb_image_write.h"

#include "../gui/ImGuiImpl.h"

#include "../core/Logger.h"
#include "../core/Window.h"

#include "../scene/Scene.h"

#include "../resources/ResourceManager.h"

#include <json.hpp>

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
		catch (std::runtime_error e)
		{
			Log::Debug << "Render Settings was incorrect, creating one";
			Save ();
		}
	}
	else
	{
		Log::Debug << "Render Settings file didn't exist, creating one";
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

VulkanRenderer::VulkanRenderer (bool validationLayer, Window& window, Resource::ResourceManager& resourceMan)

: settings ("render_settings.json"),
  device (validationLayer, window),
  vulkanSwapChain (device, device.window),
  shaderManager (device),
  // pipelineManager (*this),
  textureManager (*this, resourceMan.texManager),
  graphicsPrimaryCommandPool (device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &device.GraphicsQueue ()),
  dynamic_data (device, settings)

{
	for (int i = 0; i < vulkanSwapChain.swapChainImages.size (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, i));
	}

	for (int i = 0; i < workerThreadCount; i++)
	{
		graphicsWorkers.push_back (
		    std::make_unique<GraphicsCommandWorker> (device, workQueue, finishQueue, finishQueueLock));
	}

	ContrustFrameGraph ();
	// CreateRenderPass();

	CreateDepthResources ();
	std::array<VkImageView, 3> depthImageViews = { depthBuffer.at (0)->textureImageView,
		depthBuffer.at (1)->textureImageView,
		depthBuffer.at (2)->textureImageView };

	auto imageViewOrder = frameGraph->OrderAttachments ({ "img_depth", "img_color" });

	vulkanSwapChain.CreateFramebuffers (
	    imageViewOrder, depthImageViews, GetRelevantRenderpass (RenderableType::opaque));

	// PrepareResources ();

	PrepareImGui (&window, this);
}

VulkanRenderer::~VulkanRenderer ()
{
	ImGui_ImplGlfwVulkan_Shutdown ();

	for (auto& worker : graphicsWorkers)
		worker->StopWork ();
	workQueue.notify_all ();
	for (auto& worker : graphicsWorkers)
	{
		while (!worker->IsFinishedWorking ())
		{
		}
	}
	{
		std::lock_guard<std::mutex> lk (finishQueueLock);
		for (auto& work : finishQueue)
		{
			work.fence->WaitTillTrue ();
			work.pool->FreeCommandBuffer (work.cmdBuf);
		}
	}
	graphicsWorkers.clear ();

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
	// TEMPORARY SCAFOLDING - needs to work first before expanding
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

	// PrepareDepthPass(frameIndex);
	// BuildDepthPass(frameObjects.at(frameIndex)->GetDepthCmdBuf());
	// SubmitDepthPass(frameIndex);

	// PrepareFrame (frameIndex);
	// BuildCommandBuffers (frameObjects.at (frameIndex)->GetPrimaryCmdBuf ());
	// SubmitFrame (frameIndex);

	PrepareFrame (frameIndex);
	frameGraph->FillCommandBuffer (frameObjects.at (frameIndex)->GetPrimaryCmdBuf (),
	    vulkanSwapChain.swapChainFramebuffers[frameIndex],
	    { 0, 0 },
	    vulkanSwapChain.swapChainExtent,
	    GetFramebufferClearValues ());
	SubmitFrame (frameIndex);

	frameIndex = (frameIndex + 1) % frameObjects.size ();
	dynamic_data.AdvanceFrameCounter ();

	SaveScreenshot ();

	std::vector<GraphicsCleanUpWork> nextFramesWork;
	{
		std::lock_guard<std::mutex> lk (finishQueueLock);
		for (auto& work : finishQueue)
		{
			if (work.fence->Check ())
			{
				for (auto& sig : work.signals)
				{
					if (sig != nullptr) *sig = true;
				}
				work.pool->FreeCommandBuffer (work.cmdBuf);
			}
			else
			{
				nextFramesWork.push_back (std::move (work));
			}
		}
		finishQueue.clear ();
		finishQueue = std::move (nextFramesWork);
	}
}

void VulkanRenderer::CreatePresentResources ()
{
	ContrustFrameGraph ();
	// CreateRenderPass();

	CreateDepthResources ();
	std::array<VkImageView, 3> depthImageViews = { depthBuffer.at (0)->textureImageView,
		depthBuffer.at (1)->textureImageView,
		depthBuffer.at (2)->textureImageView };

	auto imageViewOrder = frameGraph->OrderAttachments ({ "img_depth", "img_color" });

	vulkanSwapChain.CreateFramebuffers (
	    imageViewOrder, depthImageViews, GetRelevantRenderpass (RenderableType::opaque));
}

void VulkanRenderer::RecreateSwapChain ()
{
	Log::Debug << "Recreating SwapChain"
	           << "\n";

	for (int i = 0; i < 3; i++)
		depthBuffer.at (i).reset ();

	frameGraph.reset ();
	// renderPass.reset();

	frameIndex = 0;
	frameObjects.clear ();
	vulkanSwapChain.RecreateSwapChain ();

	for (int i = 0; i < vulkanSwapChain.swapChainImages.size (); i++)
	{
		frameObjects.push_back (std::make_unique<FrameObject> (device, i));
	}

	CreatePresentResources ();
}



void VulkanRenderer::ToggleWireframe () { wireframe = !wireframe; }

// 11
void VulkanRenderer::CreateDepthResources ()
{
	VkFormat depthFormat = FindDepthFormat ();
	// depthFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	for (int i = 0; i < 3; i++)
		depthBuffer.at (i) = textureManager.CreateDepthImage (
		    depthFormat, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
}

VkFormat VulkanRenderer::FindSupportedFormat (
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties (device.physical_device, format, &props);

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
	//	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
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

	// SubpassDescription depth_subpass("sub_depth");
	// depth_subpass.SetDepthStencil("img_depth",
	// SubpassDescription::DepthStencilAccess::read_write); main_work.AddSubpass(depth_subpass);

	SubpassDescription color_subpass ("sub_color");
	color_subpass.SetDepthStencil ("img_depth", SubpassDescription::DepthStencilAccess::read_write);
	color_subpass.AddColorOutput ("img_color");
	// color_subpass.AddSubpassDependency("sub_depth");
	main_work.AddSubpass (color_subpass);

	frame_graph_builder.AddRenderPass (main_work);
	frame_graph_builder.lastPass = main_work.name;

	frameGraph = std::make_unique<FrameGraph> (frame_graph_builder, device);

	// auto depth_pre = [&](VkCommandBuffer cmdBuf) {
	// 	vkCmdBindDescriptorSets(
	// 		cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
	// 		frameDataDescriptorLayout, 0, 1, &frameDataDescriptorSet.set, 0, nullptr);
	// 	vkCmdBindDescriptorSets(
	// 		cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
	// 		lightingDescriptorLayout, 1, 1, &lightingDescriptorSet.set, 0, nullptr);

	// 	VkViewport viewport = initializers::viewport(
	// 		(float)vulkanSwapChain.swapChainExtent.width,
	// 		(float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
	// 	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

	// 	VkRect2D scissor =
	// 		initializers::rect2D(vulkanSwapChain.swapChainExtent.width,
	// 			vulkanSwapChain.swapChainExtent.height, 0, 0);
	// 	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

	// 	scene->RenderDepthPrePass(cmdBuf);
	// };
	auto main_draw = [&](VkCommandBuffer cmdBuf) {
		dynamic_data.BindFrameDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);
		dynamic_data.BindLightingDataDescriptorSet (dynamic_data.CurIndex (), cmdBuf);

		VkViewport viewport = initializers::viewport (
		    (float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
		vkCmdSetViewport (cmdBuf, 0, 1, &viewport);

		VkRect2D scissor = initializers::rect2D (
		    vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, 0, 0);
		vkCmdSetScissor (cmdBuf, 0, 1, &scissor);

		scene->RenderScene (cmdBuf, wireframe);
		ImGui_ImplGlfwVulkan_Render (cmdBuf);
	};

	frameGraph->SetDrawFuncs (0, { main_draw });
}

// void VulkanRenderer::BuildDepthPass(VkCommandBuffer cmdBuf){
//
//	vkCmdBindDescriptorSets(
//		cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
//		frameDataDescriptorLayout, 0, 1, &frameDataDescriptorSet.set, 0, nullptr);
//	vkCmdBindDescriptorSets(
//		cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
//		lightingDescriptorLayout, 1, 1, &lightingDescriptorSet.set, 0, nullptr);
//
//
//	VkViewport viewport = initializers::viewport(
//		(float)vulkanSwapChain.swapChainExtent.width,
//		(float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
//	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
//
//	VkRect2D scissor =
//		initializers::rect2D(vulkanSwapChain.swapChainExtent.width,
//			vulkanSwapChain.swapChainExtent.height, 0, 0);
//	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
//
//	scene->RenderScene(cmdBuf, wireframe);
//
//
//}
//
// void VulkanRenderer::BuildCommandBuffers (VkCommandBuffer cmdBuf)
//{
//
//
//
//
//	 renderPass->BeginRenderPass(cmdBuf,
//		vulkanSwapChain.swapChainFramebuffers[frameIndex], { 0, 0 },
//		vulkanSwapChain.swapChainExtent, GetFramebufferClearValues(),
//		VK_SUBPASS_CONTENTS_INLINE);
//
//
//
//
//	vkCmdBindDescriptorSets (cmdBuf,
//	    VK_PIPELINE_BIND_POINT_GRAPHICS,
//	    frameDataDescriptorLayout,
//	    0,
//	    1,
//	    &frameDataDescriptorSet.set,
//	    0,
//	    nullptr);
//	vkCmdBindDescriptorSets (cmdBuf,
//	    VK_PIPELINE_BIND_POINT_GRAPHICS,
//	    lightingDescriptorLayout,
//	    1,
//	    1,
//	    &lightingDescriptorSet.set,
//	    0,
//	    nullptr);
//
//	VkViewport viewport = initializers::viewport (
//	    (float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height,
// 0.0f, 1.0f); 	vkCmdSetViewport (cmdBuf, 0, 1, &viewport);
//
//	VkRect2D scissor = initializers::rect2D (
//	    vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, 0, 0);
//	vkCmdSetScissor (cmdBuf, 0, 1, &scissor);
//
//	 scene->RenderDepthPrePass (cmdBuf);
//	 renderPass->NextSubPass (cmdBuf, VK_SUBPASS_CONTENTS_INLINE);
//	 scene->RenderScene(cmdBuf, wireframe);
//
//	// Imgui rendering
//	ImGui_ImplGlfwVulkan_Render (cmdBuf);
//
//	renderPass->EndRenderPass(cmdBuf);
//}

std::array<VkClearValue, 2> VulkanRenderer::GetFramebufferClearValues ()
{
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = depthClearColor;
	return clearValues;
}

void VulkanRenderer::PrepareDepthPass (int curFrameIndex)
{
	frameObjects.at (curFrameIndex)->PrepareDepthPass ();
}

void VulkanRenderer::SubmitDepthPass (int curFrameIndex)
{
	frameObjects.at (curFrameIndex)->EndDepthPass ();

	auto curSubmitInfo = frameObjects.at (curFrameIndex)->GetDepthSubmitInfo ();

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	curSubmitInfo.pWaitDstStageMask = &stageMasks;

	device.GraphicsQueue ().Submit (curSubmitInfo, frameObjects.at (curFrameIndex)->GetDepthFence ());
}

void VulkanRenderer::PrepareFrame (int curFrameIndex)
{
	VkResult result = frameObjects.at (curFrameIndex)->AquireNextSwapchainImage (vulkanSwapChain.swapChain);

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
	frameObjects.at (curFrameIndex)->SubmitFrame ();

	auto curSubmitInfo = frameObjects.at (curFrameIndex)->GetSubmitInfo ();

	VkPipelineStageFlags stageMasks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	curSubmitInfo.pWaitDstStageMask = &stageMasks;

	device.GraphicsQueue ().Submit (curSubmitInfo, frameObjects.at (curFrameIndex)->GetCommandFence ());

	auto curPresentInfo = frameObjects.at (curFrameIndex)->GetPresentInfo ();

	VkSwapchainKHR swapChains[] = { vulkanSwapChain.swapChain };
	curPresentInfo.swapchainCount = 1;
	curPresentInfo.pSwapchains = swapChains;

	VkResult result;
	{
		std::lock_guard<std::mutex> lock (device.PresentQueue ().GetQueueMutex ());
		result = vkQueuePresentKHR (device.PresentQueue ().GetQueue (), &curPresentInfo);
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain ();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error ("failed to present swap chain image!");
	}
	// FINALLY!!!! -- not quite...
	//std::lock_guard<std::mutex> lock (device.PresentQueue ().GetQueueMutex ());
	//vkQueueWaitIdle (device.PresentQueue ().GetQueue ());
}

std::shared_ptr<VulkanDescriptor> VulkanRenderer::GetVulkanDescriptor ()
{
	std::shared_ptr<VulkanDescriptor> descriptor = std::make_shared<VulkanDescriptor> (device);
	descriptors.push_back (descriptor);
	return descriptor;
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
	workQueue.push_back (
	    GraphicsWork (work, workType, device, waitSemaphores, signalSemaphores, buffersToClean, signals));
}


VkCommandBuffer VulkanRenderer::GetGraphicsCommandBuffer ()
{

	return graphicsPrimaryCommandPool.GetPrimaryCommandBuffer ();
}

void VulkanRenderer::SubmitGraphicsCommandBufferAndWait (VkCommandBuffer commandBuffer)
{
	if (commandBuffer == VK_NULL_HANDLE) return;

	VkFence fence;
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo ();
	VK_CHECK_RESULT (vkCreateFence (device.device, &fenceInfo, nullptr, &fence));
	graphicsPrimaryCommandPool.SubmitPrimaryCommandBuffer (commandBuffer, fence);

	device.GraphicsQueue ().WaitForFences (fence);

	vkDestroyFence (device.device, fence, nullptr);
	graphicsPrimaryCommandPool.FreeCommandBuffer (commandBuffer);
}

void WaitForSubmissionFinish (VkDevice device,
    CommandPool* pool,
    VkCommandBuffer buf,
    VkFence fence,
    std::vector<Signal> readySignal,
    std::vector<VulkanBuffer> bufsToClean)
{

	vkWaitForFences (device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	if (vkGetFenceStatus (device, fence) == VK_SUCCESS)
	{
		for (auto& sig : readySignal)
			*sig = true;
	}
	else if (vkGetFenceStatus (device, fence) == VK_NOT_READY)
	{
		Log::Error << "Transfer exeeded maximum fence timeout! Is too much stuff "
		              "happening?\n";
		vkWaitForFences (device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		if (vkGetFenceStatus (device, fence) == VK_SUCCESS)
		{
			for (auto& sig : readySignal)
				*sig = true;
		}
	}
	else if (vkGetFenceStatus (device, fence) == VK_ERROR_DEVICE_LOST)
	{
		Log::Error << "AAAAAAAAAAAHHHHHHHHHHHHH EVERYTHING IS ONE FIRE\n";
		throw std::runtime_error ("Fence lost device!\n");
	}

	vkDestroyFence (device, fence, nullptr);

	pool->FreeCommandBuffer (buf);

	// for (auto& buffer : bufsToClean) {
	// 	buffer.CleanBuffer();
	// }
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

	// Log::Debug << " HI " << "\n";
}

GPU_DoubleBuffer::GPU_DoubleBuffer (VulkanDevice& device, RenderSettings& settings)
: device (device)
{
	int num_buffers = 3;
	for (auto& data : d_buffers)
	{
		data.globalVariableBuffer =
		    std::make_unique<VulkanBufferUniformPersistant> (device, sizeof (GlobalData));
		data.cameraDataBuffer = std::make_unique<VulkanBufferUniformPersistant> (
		    device, sizeof (CameraData) * settings.cameraCount);
		data.sunBuffer = std::make_unique<VulkanBufferUniformPersistant> (
		    device, sizeof (DirectionalLight) * settings.directionalLightCount);
		data.pointLightsBuffer = std::make_unique<VulkanBufferUniformPersistant> (
		    device, sizeof (PointLight) * settings.pointLightCount);
		data.spotLightsBuffer = std::make_unique<VulkanBufferUniformPersistant> (
		    device, sizeof (SpotLight) * settings.spotLightCount);

		data.dynamicTransformBuffer = std::make_unique<VulkanBufferUniformDynamic> (
		    device, MaxTransformCount, sizeof (TransformMatrixData));
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
	/////Lighting
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
	//// Transformation Matrices -- Dynamic Uniform Buffer
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

void VulkanRenderer::SaveScreenshotNextFrame () { saveScreenshot = true; }

// Take a screenshot for the curretn swapchain image
// This is done using a blit from the swapchain image to a linear image whose
// memory content is then saved as a ppm image Getting the image date directly
// from a swapchain image wouldn't work as they're usually stored in an
// implementation dependant optimal tiling format Note: This requires the
// swapchain images to be created with the VK_IMAGE_USAGE_TRANSFER_SRC_BIT flag
// (see VulkanSwapChain::create)
void VulkanRenderer::SaveScreenshot ()
{
	if (saveScreenshot)
	{
		// std::string filename = "VulkanScreenshot.png";
		//// Get format properties for the swapchain color format
		// VkFormatProperties formatProps;

		// bool supportsBlit = true;

		//// Check blit support for source and destination

		//// Check if the device supports blitting from optimal images (the swapchain
		//// images are in optimal format)
		// vulkanSwapChain.swapChain;
		// vkGetPhysicalDeviceFormatProperties(device.physical_device,
		//	vulkanSwapChain.swapChainImageFormat,
		//	&formatProps);
		// if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		//	Log::Error << "Device does not support blitting from optimal tiled "
		//		"images, using copy instead of blit!"
		//		<< "\n";
		//	supportsBlit = false;
		//}

		//// Check if the device supports blitting to linear images
		// vkGetPhysicalDeviceFormatProperties(device.physical_device,
		//	VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
		// if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		//	Log::Error << "Device does not support blitting to linear tiled images, "
		//		"using copy instead of blit!"
		//		<< "\n";
		//	supportsBlit = false;
		//}

		//// Source for the copy is the last rendered swapchain image
		// VkImage srcImage =
		//	vulkanSwapChain.swapChainImages[vulkanSwapChain.currentBuffer];

		//// Create the linear tiled destination image to copy to and to read the
		//// memory from
		// VkImageCreateInfo imgCreateInfo(initializers::imageCreateInfo());
		// imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		//// Note that vkCmdBlitImage (if supported) will also do format conversions
		//// if the swapchain color format would differ
		// imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		// imgCreateInfo.extent.width = vulkanSwapChain.swapChainExtent.width;
		// imgCreateInfo.extent.height = vulkanSwapChain.swapChainExtent.height;
		// imgCreateInfo.extent.depth = 1;
		// imgCreateInfo.arrayLayers = 1;
		// imgCreateInfo.mipLevels = 1;
		// imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		// imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		// imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		//// Create the image
		// VkImage dstImage;
		// VK_CHECK_RESULT(
		//	vkCreateImage(device.device, &imgCreateInfo, nullptr, &dstImage));
		//// Create memory to back up the image
		// VkMemoryRequirements memRequirements;
		// VkMemoryAllocateInfo memAllocInfo(initializers::memoryAllocateInfo());
		// VkDeviceMemory dstImageMemory;
		// vkGetImageMemoryRequirements(device.device, dstImage, &memRequirements);
		// memAllocInfo.allocationSize = memRequirements.size;
		//// Memory must be host visible to copy from
		// memAllocInfo.memoryTypeIndex =
		//	device.getMemoryType(memRequirements.memoryTypeBits,
		//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		// VK_CHECK_RESULT(vkAllocateMemory(device.device, &memAllocInfo, nullptr,
		//	&dstImageMemory));
		// VK_CHECK_RESULT(
		//	vkBindImageMemory(device.device, dstImage, dstImageMemory, 0));

		//// Do the actual blit from the swapchain image to our host visible
		//// destination image
		// VkCommandBuffer copyCmd = GetGraphicsCommandBuffer();

		// VkImageMemoryBarrier imageMemoryBarrier =
		//	initializers::imageMemoryBarrier();

		//// Transition destination image to transfer destination layout
		// InsertImageMemoryBarrier(
		//	copyCmd, dstImage, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
		//	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// Transition swapchain image from present to transfer source layout
		// InsertImageMemoryBarrier(
		//	copyCmd, srcImage, VK_ACCESS_MEMORY_READ_BIT,
		//	VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// If source and destination support blit we'll blit as this also does
		//// automatic format conversion (e.g. from BGR to RGB)
		// if (supportsBlit) {
		//	// Define the region to blit (we will blit the whole swapchain image)
		//	VkOffset3D blitSize;
		//	blitSize.x = vulkanSwapChain.swapChainExtent.width;
		//	blitSize.y = vulkanSwapChain.swapChainExtent.height;
		//	blitSize.z = 1;
		//	VkImageBlit imageBlitRegion{};
		//	imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//	imageBlitRegion.srcSubresource.layerCount = 1;
		//	imageBlitRegion.srcOffsets[1] = blitSize;
		//	imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//	imageBlitRegion.dstSubresource.layerCount = 1;
		//	imageBlitRegion.dstOffsets[1] = blitSize;

		//	// Issue the blit command
		//	vkCmdBlitImage(copyCmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		//		&imageBlitRegion, VK_FILTER_NEAREST);
		//}
		// else {
		//	// Otherwise use image copy (requires us to manually flip components)
		//	VkImageCopy imageCopyRegion{};
		//	imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//	imageCopyRegion.srcSubresource.layerCount = 1;
		//	imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//	imageCopyRegion.dstSubresource.layerCount = 1;
		//	imageCopyRegion.extent.width = vulkanSwapChain.swapChainExtent.width;
		//	imageCopyRegion.extent.height = vulkanSwapChain.swapChainExtent.height;
		//	imageCopyRegion.extent.depth = 1;

		//	// Issue the copy command
		//	vkCmdCopyImage(copyCmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		//		&imageCopyRegion);
		//}

		//// Transition destination image to general layout, which is the required
		//// layout for mapping the image memory later on
		// InsertImageMemoryBarrier(
		//	copyCmd, dstImage, VK_ACCESS_TRANSFER_WRITE_BIT,
		//	VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// Transition back the swap chain image after the blit is done
		// InsertImageMemoryBarrier(
		//	copyCmd, srcImage, VK_ACCESS_TRANSFER_READ_BIT,
		//	VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		// SubmitGraphicsCommandBufferAndWait(copyCmd);

		//// Get layout of the image (including row pitch)
		// VkImageSubresource subResource{};
		// subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// VkSubresourceLayout subResourceLayout;

		// vkGetImageSubresourceLayout(device.device, dstImage, &subResource,
		//	&subResourceLayout);

		//// Map image memory so we can start copying from it
		// const char *data;
		// const char *dataForSTB;
		// vkMapMemory(device.device, dstImageMemory, 0, VK_WHOLE_SIZE, 0,
		//	(void **)&data);
		// dataForSTB = data;
		// data += subResourceLayout.offset;

		// std::ofstream file(filename, std::ios::out | std::ios::binary);
		//
		//// ppm header
		// file << "P6\n" << vulkanSwapChain.swapChainExtent.width << "\n" <<
		// vulkanSwapChain.swapChainExtent.height << "\n" << 255 << "\n";
		//
		//// If source is BGR (destination is always RGB) and we can't use blit
		///(which does automatic conversion), we'll have to manually swizzle color
		/// components
		// bool colorSwizzle = false;
		//// Check if source is BGR
		//// Note: Not complete, only contains most common and basic BGR surface
		/// formats for demonstation purposes
		// if (!supportsBlit)
		//{
		//	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB,
		// VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM }; 	colorSwizzle =
		//(std::find(formatsBGR.begin(), formatsBGR.end(),
		// vulkanSwapChain.swapChainImageFormat) != formatsBGR.end());
		//}
		//
		//// ppm binary pixel data
		// for (uint32_t y = 0; y < vulkanSwapChain.swapChainExtent.height; y++)
		//{
		//	unsigned int *row = (unsigned int*)data;
		//	for (uint32_t x = 0; x < vulkanSwapChain.swapChainExtent.width; x++)
		//	{
		//		if (colorSwizzle)
		//		{
		//			file.write((char*)row + 2, 1);
		//			file.write((char*)row + 1, 1);
		//			file.write((char*)row, 1);
		//		}
		//		else
		//		{
		//			file.write((char*)row, 3);
		//		}
		//		row++;
		//	}
		//	data += subResourceLayout.rowPitch;
		//}
		// file.close();

		// int err = stbi_write_png(
		//	filename.c_str(), vulkanSwapChain.swapChainExtent.width,
		//	vulkanSwapChain.swapChainExtent.height, STBI_rgb_alpha, dataForSTB,
		//	vulkanSwapChain.swapChainExtent.width * STBI_rgb_alpha);
		// if (err == 0) {
		//	Log::Debug << "Screenshot saved to disk"
		//		<< "\n";
		//}
		// else {
		//	Log::Debug << "Failed to save screenshot!\nError code = " << err << "\n";
		//}

		//// Clean up resources
		// vkUnmapMemory(device.device, dstImageMemory);
		// vkFreeMemory(device.device, dstImageMemory, nullptr);
		// vkDestroyImage(device.device, dstImage, nullptr);

		saveScreenshot = false;
	}
}
