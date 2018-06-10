#include "Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third-party/stb_image/stb_image_write.h"

#include "../gui/ImGuiImpl.h"
#include "../scene/Scene.h"

#include "../core/Logger.h"

#include <json.hpp>
#include <fstream>

RenderSettings::RenderSettings(std::string fileName) : fileName(fileName) {
	Load();
}

void RenderSettings::Load() {
	if (fileExists(fileName)) {
		std::ifstream input(fileName);
		nlohmann::json j;
		input >> j;

		graphicsSetupWorkerCount = j["graphics_worker_count"];
		transferWorkerCount = j["transfer_worker_count"];
		directionalLightCount = j["directional_light_count"];
		pointLightCount = j["point_light_count"];
		spotLightCount = j["spot_light_count"];
	}
	else {
		Log::Debug << "Render Settings file didn't exist, creating one";
		Save();
	}
}
void RenderSettings::Save() {
	nlohmann::json j;

	j["graphics_worker_count"] = graphicsSetupWorkerCount;
	j["transfer_worker_count"] = transferWorkerCount;

	j["directional_light_count"] = directionalLightCount;
	j["point_light_count"] = pointLightCount;
	j["spot_light_count"] = spotLightCount;

	std::ofstream outFile(fileName);
	outFile << std::setw(4) << j;
	outFile.close();
}

VulkanRenderer::VulkanRenderer(bool validationLayer,
	GLFWwindow* window)

	:settings("render_settings.json"),
	device(validationLayer),
	vulkanSwapChain(device),
	shaderManager(device),
	pipelineManager(device),
	textureManager(device),
	globalVariableBuffer(device),
	cameraDataBuffer(device),
	sunBuffer(device),
	pointLightsBuffer(device),
	spotLightsBuffer(device),
	entityPositions(device),
	graphicsPrimaryCommandPool(device)

{
	device.window = window;

	device.InitVulkanDevice(vulkanSwapChain.surface);

	graphicsPrimaryCommandPool.Setup(
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &device.GraphicsQueue());

	vulkanSwapChain.InitSwapChain(device.window);

	pipelineManager.InitPipelineCache();

	if (settings.graphicsSetupWorkerCount < 1)
		settings.graphicsSetupWorkerCount = 1;
	if (settings.transferWorkerCount < 1)
		settings.transferWorkerCount = 1;

	for (int i = 0; i < settings.graphicsSetupWorkerCount; i++) {
		graphicsSetupWorkers.push_back(
			std::make_unique<CommandBufferWorker<CommandBufferWork>>(
				device, &device.GraphicsQueue(), graphicsSetupWorkQueue));
	}

	for (int i = 0; i < settings.transferWorkerCount; i++) {
		transferWorkers.push_back(
			std::make_unique<CommandBufferWorker<TransferCommandWork>>(
				device, &device.TransferQueue(), transferWorkQueue));
	}

	CreateRenderPass();

	CreateDepthResources();
	vulkanSwapChain.CreateFramebuffers(depthBuffer->textureImageView, renderPass);

	CreateCommandBuffers();

	PrepareResources();

	imageAvailableSemaphore = std::make_unique<VulkanSemaphore>(device);
	renderFinishedSemaphore = std::make_unique<VulkanSemaphore>(device);


	PrepareImGui(window, this);
}

VulkanRenderer::~VulkanRenderer() {
	ImGui_ImplGlfwVulkan_Shutdown();

	globalVariableBuffer.CleanBuffer();
	cameraDataBuffer.CleanBuffer();
	sunBuffer.CleanBuffer();
	pointLightsBuffer.CleanBuffer();
	spotLightsBuffer.CleanBuffer();

	vkDestroyPipelineLayout(device.device, frameDataDescriptorLayout, nullptr);
	vkDestroyPipelineLayout(device.device, lightingDescriptorLayout, nullptr);

	for (auto& descriptor : descriptors)
		descriptor->CleanUp();

	depthBuffer->destroy();

	for (auto& worker : graphicsSetupWorkers)
		worker->CleanUp();

	for (auto& worker : transferWorkers)
		worker->CleanUp();

	graphicsPrimaryCommandPool.CleanUp();

	vkDestroyRenderPass(device.device, renderPass, nullptr);
	vulkanSwapChain.CleanUp();

	renderFinishedSemaphore->CleanUp(device);
	imageAvailableSemaphore->CleanUp(device);

	shaderManager.CleanUp();

	pipelineManager.CleanUp();

	device.Cleanup(vulkanSwapChain.surface);

	Log::Debug << "renderer deleted\n";
}

void VulkanRenderer::DeviceWaitTillIdle() { vkDeviceWaitIdle(device.device); }

void VulkanRenderer::UpdateRenderResources(
	GlobalData globalData, CameraData cameraData,
	std::vector<DirectionalLight> directionalLights,
	std::vector<PointLight> pointLights, std::vector<SpotLight> spotLights) {
	globalVariableBuffer.CopyToBuffer(&globalData, sizeof(GlobalData));
	cameraDataBuffer.CopyToBuffer(&cameraData, sizeof(CameraData));
	sunBuffer.CopyToBuffer(directionalLights.data(),
		directionalLights.size() * sizeof(DirectionalLight));
	pointLightsBuffer.CopyToBuffer(pointLights.data(),
		pointLights.size() * sizeof(PointLight));
	spotLightsBuffer.CopyToBuffer(spotLights.data(),
		spotLights.size() * sizeof(SpotLight));
}

void VulkanRenderer::RenderFrame() {

	PrepareFrame();
	BuildCommandBuffers();
	SubmitFrame();
	SaveScreenshot();
}

void VulkanRenderer::CreateWorkerThreads() {

	if (settings.graphicsSetupWorkerCount <= 0)
		settings.graphicsSetupWorkerCount = 1;
	if (settings.transferWorkerCount <= 0)
		settings.transferWorkerCount = 1;

	for (int i = 0; i < settings.graphicsSetupWorkerCount; i++) {
		graphicsSetupWorkers.push_back(
			std::make_unique<CommandBufferWorker<CommandBufferWork>>(
				device, &device.GraphicsQueue(), graphicsSetupWorkQueue));
	}

	for (int i = 0; i < settings.transferWorkerCount; i++) {
		transferWorkers.push_back(
			std::make_unique<CommandBufferWorker<TransferCommandWork>>(
				device, &device.TransferQueue(), transferWorkQueue));
	}
}

void VulkanRenderer::DestroyWorkerThreads() {

	for (auto& thread : graphicsSetupWorkers) {
		thread->StopWork();
	}

	for (auto& thread : transferWorkers) {
		thread->StopWork();
	}

	for (auto& thread : graphicsSetupWorkers) {
		thread->StopWork();
	}
}

void VulkanRenderer::RecreateSwapChain() {
	Log::Debug << "Recreating SwapChain"
		<< "\n";

	depthBuffer->destroy();

	vkDestroyRenderPass(device.device, renderPass, nullptr);

	vulkanSwapChain.RecreateSwapChain(device.window);

	CreateRenderPass();
	CreateDepthResources();
	vulkanSwapChain.CreateFramebuffers(depthBuffer->textureImageView, renderPass);

	// frameIndex = 1; //cause it needs it to be synced back to zero (yes I know
	// it says one, thats intended, build command buffers uses the "next" frame
	// index since it has to sync with the swapchain so it starts at one....)
	ReBuildCommandBuffers();
}

void VulkanRenderer::ReBuildCommandBuffers() {
	// vkFreeCommandBuffers(device.device, device.graphics_queue_command_pool,
	// static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	// vkResetCommandPool(device.device, device.graphics_queue_command_pool, 0);

	graphicsPrimaryCommandPool.ResetPool();

	CreateCommandBuffers();
	BuildCommandBuffers();
}

void VulkanRenderer::SetWireframe(bool wireframe) {
	this->wireframe = wireframe;
}

void VulkanRenderer::CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = vulkanSwapChain.swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT; // findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment,
														  depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device.device, &renderPassInfo, nullptr,
		&renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

// 11
void VulkanRenderer::CreateDepthResources() {
	VkFormat depthFormat = FindDepthFormat();
	depthFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	depthBuffer = std::make_unique<VulkanTextureDepthBuffer>(device);
	depthBuffer->CreateDepthImage(*this, depthFormat,
		vulkanSwapChain.swapChainExtent.width,
		vulkanSwapChain.swapChainExtent.height);
}

VkFormat
VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat> &candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanRenderer::FindDepthFormat() {
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
		 VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

// 20
void VulkanRenderer::CreateCommandBuffers() {

	commandBuffers.resize(vulkanSwapChain.swapChainFramebuffers.size());

	for (auto& cmdBuf : commandBuffers) {
		cmdBuf = graphicsPrimaryCommandPool.GetPrimaryCommandBuffer(false);
	}

	/*VkCommandBufferAllocateInfo allocInfo =
	initializers::commandBufferAllocateInfo(device.graphics_queue_command_pool,
	VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());

	std::lock_guard<std::mutex> lock(device.graphics_command_pool_lock);
	if (vkAllocateCommandBuffers(device.device, &allocInfo, commandBuffers.data())
	!= VK_SUCCESS) { throw std::runtime_error("failed to allocate command
	buffers!");
	}*/
}

void VulkanRenderer::BuildCommandBuffers() {

	// for (size_t i = 0; i < commandBuffers.size(); i++) {

	VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(commandBuffers[frameIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo(
		renderPass, vulkanSwapChain.swapChainFramebuffers[frameIndex], { 0, 0 },
		vulkanSwapChain.swapChainExtent, GetFramebufferClearValues());

	vkCmdBeginRenderPass(commandBuffers[frameIndex], &renderPassInfo,
		VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(
		commandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
		frameDataDescriptorLayout, 0, 1, &frameDataDescriptorSet.set, 0, nullptr);
	vkCmdBindDescriptorSets(
		commandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
		lightingDescriptorLayout, 1, 1, &lightingDescriptorSet.set, 0, nullptr);

	VkViewport viewport = initializers::viewport(
		(float)vulkanSwapChain.swapChainExtent.width,
		(float)vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffers[frameIndex], 0, 1, &viewport);

	VkRect2D scissor =
		initializers::rect2D(vulkanSwapChain.swapChainExtent.width,
			vulkanSwapChain.swapChainExtent.height, 0, 0);
	vkCmdSetScissor(commandBuffers[frameIndex], 0, 1, &scissor);

	VkDeviceSize offsets[] = { 0 };

	scene->RenderScene(commandBuffers[frameIndex], wireframe);

	// Imgui rendering
	ImGui_ImplGlfwVulkan_Render(commandBuffers[frameIndex]);

	vkCmdEndRenderPass(commandBuffers[frameIndex]);

	if (vkEndCommandBuffer(commandBuffers[frameIndex]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
	//}
}

// Meant to be used in conjunction with secondary command buffers
void VulkanRenderer::CreatePrimaryCommandBuffer() {
	commandBuffers.resize(vulkanSwapChain.swapChainFramebuffers.size());

	/*VkCommandBufferAllocateInfo allocInfo =
	initializers::commandBufferAllocateInfo(device.graphics_queue_command_pool,
	VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());
	{
			std::lock_guard<std::mutex> lock(device.graphics_command_pool_lock);
			if (vkAllocateCommandBuffers(device.device, &allocInfo,
	commandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("failed to
	allocate command buffers!");
			}
	}*/
	for (auto& cmdBuf : commandBuffers) {
		cmdBuf = graphicsPrimaryCommandPool.GetPrimaryCommandBuffer(false);
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo(
			renderPass, vulkanSwapChain.swapChainFramebuffers[i], { 0, 0 },
			vulkanSwapChain.swapChainExtent, GetFramebufferClearValues());

		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
			VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		// Render stuff

		// Contains the list of secondary command buffers to be executed
		std::vector<VkCommandBuffer> secondaryCommandBuffers;

		vkCmdExecuteCommands(commandBuffers[i], (uint32_t)commandBuffers.size(),
			commandBuffers.data());

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

//void VulkanRenderer::CreateSemaphores() {
//	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo();
//
//	if (vkCreateSemaphore(device.device, &semaphoreInfo, nullptr,
//		&imageAvailableSemaphore) != VK_SUCCESS ||
//		vkCreateSemaphore(device.device, &semaphoreInfo, nullptr,
//			&renderFinishedSemaphore) != VK_SUCCESS) {
//
//		throw std::runtime_error("failed to create semaphores!");
//	}
//}

std::array<VkClearValue, 2> VulkanRenderer::GetFramebufferClearValues() {
	std::array<VkClearValue, 2> clearValues;
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = depthClearColor;
	return clearValues;
}

void VulkanRenderer::PrepareFrame() {
	VkResult result = vkAcquireNextImageKHR(
		device.device, vulkanSwapChain.swapChain,
		std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore->Get(),
		VK_NULL_HANDLE, &frameIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || (result == VK_SUBOPTIMAL_KHR)) {
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
}

void VulkanRenderer::SubmitFrame() {
	std::vector<VkSemaphore> waitSemaphores = { imageAvailableSemaphore->Get() };
	std::vector<VkSemaphore> signalSemaphores = { renderFinishedSemaphore->Get() };

	const auto stageMasks =
		std::vector<VkPipelineStageFlags>{ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.signalSemaphoreCount = (uint32_t)signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = stageMasks.data();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];
	// VkPipelineStageFlags waitStages[] = {
	// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	// submitInfo.waitSemaphoreCount = 1;
	// submitInfo.pWaitSemaphores = waitSemaphores;
	// submitInfo.pWaitDstStageMask = waitStages;

	// submitInfo.commandBufferCount = 1;
	// submitInfo.pCommandBuffers = &commandBuffers[frameIndex];

	device.GraphicsQueue().Submit(submitInfo, VK_NULL_HANDLE);
	//{
	//	std::lock_guard<std::mutex> lock(device.graphics_lock);
	//	if (vkQueueSubmit(device.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE)
	//!= VK_SUCCESS) { 		throw std::runtime_error("failed to submit draw command
	//buffer!");
	//	}
	//}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();

	VkSwapchainKHR swapChains[] = { vulkanSwapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &frameIndex;
	VkResult result;
	{
		std::lock_guard<std::mutex> lock(device.PresentQueue().GetQueueMutex());
		result = vkQueuePresentKHR(device.PresentQueue().GetQueue(), &presentInfo);
	}
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	std::lock_guard<std::mutex> lock(device.PresentQueue().GetQueueMutex());
	vkQueueWaitIdle(device.PresentQueue().GetQueue());
}

void VulkanRenderer::PrepareResources() {
	// globalVariableBuffer.CreateUniformBuffer(sizeof(GlobalVariableUniformBuffer));
	// pointLightsBuffer.CreateUniformBuffer(sizeof(PointLight) * 5);

	globalVariableBuffer.CreateUniformBuffer(sizeof(GlobalData));
	cameraDataBuffer.CreateUniformBuffer(sizeof(CameraData) * settings.cameraCount);
	sunBuffer.CreateUniformBuffer(sizeof(DirectionalLight) *
		settings.directionalLightCount);
	pointLightsBuffer.CreateUniformBuffer(sizeof(PointLight) * settings.pointLightCount);
	spotLightsBuffer.CreateUniformBuffer(sizeof(SpotLight) * settings.spotLightCount);

	SetupGlobalDescriptorSet();
	SetupLightingDescriptorSet();
}

std::shared_ptr<VulkanDescriptor> VulkanRenderer::GetVulkanDescriptor() {
	std::shared_ptr<VulkanDescriptor> descriptor =
		std::make_shared<VulkanDescriptor>(device);
	descriptors.push_back(descriptor);
	return descriptor;
}

void VulkanRenderer::AddGlobalLayouts(
	std::vector<VkDescriptorSetLayout> &layouts) {
	layouts.push_back(frameDataDescriptor->GetLayout());
	layouts.push_back(lightingDescriptor->GetLayout());
}


void VulkanRenderer::SetupGlobalDescriptorSet() {
	frameDataDescriptor = GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 0, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, 1, 1));
	frameDataDescriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	frameDataDescriptor->SetupPool(poolSizes);

	frameDataDescriptorSet = frameDataDescriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(0, 1, globalVariableBuffer.resource));
	writes.push_back(DescriptorUse(1, 1, cameraDataBuffer.resource));
	frameDataDescriptor->UpdateDescriptorSet(frameDataDescriptorSet, writes);

	auto desLayout = frameDataDescriptor->GetLayout();
	auto pipelineLayoutInfo =
		initializers::pipelineLayoutCreateInfo(&desLayout, 1);

	if (vkCreatePipelineLayout(device.device, &pipelineLayoutInfo, nullptr,
		&frameDataDescriptorLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void VulkanRenderer::SetupLightingDescriptorSet() {
	lightingDescriptor = GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	lightingDescriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	lightingDescriptor->SetupPool(poolSizes);

	lightingDescriptorSet = lightingDescriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(0, 1, sunBuffer.resource));
	writes.push_back(DescriptorUse(1, 1, pointLightsBuffer.resource));
	writes.push_back(DescriptorUse(2, 1, spotLightsBuffer.resource));
	lightingDescriptor->UpdateDescriptorSet(lightingDescriptorSet, writes);

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.push_back(frameDataDescriptor->GetLayout());
	layouts.push_back(lightingDescriptor->GetLayout());

	auto pipelineLayoutInfo =
		initializers::pipelineLayoutCreateInfo(layouts.data(), 2);

	if (vkCreatePipelineLayout(device.device, &pipelineLayoutInfo, nullptr,
		&lightingDescriptorLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkImageSubresourceRange subresourceRange) {
	VkImageMemoryBarrier imageMemoryBarrier = initializers::imageMemoryBarrier();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0,
		nullptr, 1, &imageMemoryBarrier);

	// Log::Debug << " HI " << "\n";
}

void VulkanRenderer::SubmitTransferWork(TransferCommandWork &&data) {
	transferWorkQueue.AddWork(data);
}

void VulkanRenderer::SubmitGraphicsSetupWork(CommandBufferWork &&data) {
	graphicsSetupWorkQueue.AddWork(data);
}

VkCommandBuffer VulkanRenderer::GetGraphicsCommandBuffer() {
	// VkCommandBufferAllocateInfo cmdBufAllocateInfo =
	//	initializers::commandBufferAllocateInfo(graphics_queue_command_pool,
	//VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	// VkCommandBuffer cmdBuffer;
	//{
	//	std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
	//	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo,
	//&cmdBuffer));
	//}
	//// If requested, also start recording for the new command buffer

	// VkCommandBufferBeginInfo cmdBufInfo =
	// initializers::commandBufferBeginInfo();
	////{
	////	std::lock_guard<std::mutex> lock(graphics_lock);
	// VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
	////}

	return graphicsPrimaryCommandPool.GetPrimaryCommandBuffer();
}

void VulkanRenderer::SubmitGraphicsCommandBufferAndWait(
	VkCommandBuffer commandBuffer) {
	if (commandBuffer == VK_NULL_HANDLE)
		return;

	VkFence fence;
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence));
	graphicsPrimaryCommandPool.SubmitPrimaryCommandBuffer(commandBuffer, fence);

	device.GraphicsQueue().WaitForFences(fence);

	vkDestroyFence(device.device, fence, nullptr);
	graphicsPrimaryCommandPool.FreeCommandBuffer(commandBuffer);
	// if (commandBuffer == VK_NULL_HANDLE)
	//	return;

	// VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	// VkSubmitInfo submitInfo = initializers::submitInfo();
	// submitInfo.commandBufferCount = 1;
	// submitInfo.pCommandBuffers = &commandBuffer;

	//// Create fence to ensure that the command buffer has finished executing
	// VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	// VkFence fence;
	// VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));

	//{
	//	std::lock_guard<std::mutex> lock(graphics_lock);
	//	VK_CHECK_RESULT(vkQueueSubmit(graphics_queue, 1, &submitInfo, fence));
	//}

	// VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE,
	// DEFAULT_FENCE_TIMEOUT));

	// vkDestroyFence(device, fence, nullptr);

	//{
	//	std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
	//	vkFreeCommandBuffers(device, graphics_queue_command_pool, 1,
	//&commandBuffer);
	//}
}

VkCommandBuffer VulkanRenderer::GetSingleUseGraphicsCommandBuffer() {

	return graphicsPrimaryCommandPool.GetOneTimeUseCommandBuffer();
	/*VkCommandBuffer buf;
	VkCommandBufferAllocateInfo allocInfo =
			initializers::commandBufferAllocateInfo(graphics_queue_command_pool,
	VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	{
			std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
			vkAllocateCommandBuffers(device, &allocInfo, &buf);
	}

	VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(buf, &beginInfo);
	return buf;*/
}

// VkCommandBuffer VulkanRenderer::GetTransferCommandBuffer() {

// 	if (device.separateTransferQueue) {
// 		return transferCommandPool.GetOneTimeUseCommandBuffer();
// 		// return GetTransferCommandBuffer();
// 	}
// 	else {
// 		return graphicsPrimaryCommandPool.GetOneTimeUseCommandBuffer();
// 		/*VkCommandBuffer buf;
// 		VkCommandBufferAllocateInfo allocInfo =
// 				initializers::commandBufferAllocateInfo(graphics_queue_command_pool,
// 		VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

// 		{
// 				std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
// 				vkAllocateCommandBuffers(device, &allocInfo, &buf);
// 		}

// 		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
// 		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

// 		vkBeginCommandBuffer(buf, &beginInfo);
// 		return buf;
// 	*/
// 	}
// }

void WaitForSubmissionFinish(VkDevice device, CommandPool *pool,
	VkCommandBuffer buf, VkFence fence,
	std::vector<Signal> readySignal,
	std::vector<VulkanBuffer> bufsToClean) {

	vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
		for (auto& sig : readySignal)
			*sig = true;
	}
	else if (vkGetFenceStatus(device, fence) == VK_NOT_READY) {
		Log::Error << "Transfer exeeded maximum fence timeout! Is too much stuff "
			"happening?\n";
		vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
			for (auto& sig : readySignal)
				*sig = true;
		}
	}
	else if (vkGetFenceStatus(device, fence) == VK_ERROR_DEVICE_LOST) {
		Log::Error << "AAAAAAAAAAAHHHHHHHHHHHHH EVERYTHING IS ONE FIRE\n";
		throw std::runtime_error("Fence lost device!\n");
	}

	vkDestroyFence(device, fence, nullptr);

	pool->FreeCommandBuffer(buf);

	for (auto& buffer : bufsToClean) {
		buffer.CleanBuffer();
	}
}

// void VulkanRenderer::SubmitTransferCommandBufferAndWait(VkCommandBuffer buf) {
// 	if (device.separateTransferQueue) {
// 		// transferQueue->SubmitTransferCommandBufferAndWait(buf);

// 		VkFence fence;
// 		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

// 			transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 		vkWaitForFences(device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
// 		vkDestroyFence(device.device, fence, nullptr);

// 		transferCommandPool.FreeCommandBuffer(buf);
// 	}
// 	else {
// 		SubmitGraphicsCommandBufferAndWait(buf);
// 		/*vkEndCommandBuffer(buf);

// 		VkSubmitInfo submitInfo = initializers::submitInfo();
// 		submitInfo.commandBufferCount = 1;
// 		submitInfo.pCommandBuffers = &buf;

// 		{
// 				std::lock_guard<std::mutex> lock(graphics_lock);

// 				vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
// 		}

// 		{
// 				std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
// 				vkFreeCommandBuffers(device, graphics_queue_command_pool, 1, &buf);
// 		}*/
// 	}
//}

// void VulkanRenderer::SubmitTransferCommandBuffer(
// 	VkCommandBuffer buf, std::vector<Signal> readySignal,
// 	std::vector<VulkanBuffer> bufsToClean) {
// 	if (device.separateTransferQueue) {
// 		// transferQueue->SubmitTransferCommandBuffer(buf, readySignal,
// 		// bufsToClean);

// 		VkFence fence;
// 		VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 		VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr, &fence))

// 			transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 		std::thread submissionCompletion =
// 			std::thread(WaitForSubmissionFinish, device.device,
// 				&transferCommandPool, buf, fence, readySignal, bufsToClean);
// 		submissionCompletion.detach();
// 	}
// 	else {
// 		vkEndCommandBuffer(buf);

// 		// VkSubmitInfo submitInfo = initializers::submitInfo();
// 		// submitInfo.commandBufferCount = 1;
// 		// submitInfo.pCommandBuffers = &buf;

// 		graphicsPrimaryCommandPool.SubmitOneTimeUseCommandBuffer(buf);
// 		// vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
// 		// vkQueueWaitIdle(graphics_queue);

// 		for (auto& buffer : bufsToClean) {
// 			buffer.CleanBuffer();
// 		}
// 		graphicsPrimaryCommandPool.FreeCommandBuffer(buf);
// 		//{
// 		//	std::lock_guard<std::mutex> lock(graphics_command_pool_lock);
// 		//	vkFreeCommandBuffers(device, graphicsPrimaryCommandPool, 1, &buf);
// 		//}
// 		for (auto& sig : readySignal)
// 			*sig = true;
// 	}
// }

// void VulkanRenderer::SubmitTransferCommandBuffer(
// 	VkCommandBuffer buf, std::vector<Signal> readySignal) {
// 	SubmitTransferCommandBuffer(buf, readySignal, {});
// }

// void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf,
// std::vector<Signal> readySignal, std::vector<VulkanBuffer> bufsToClean) {

// 	VkFence fence;
// 	VkFenceCreateInfo fenceInfo =
// initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr,
// &fence))

// 	transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 	std::thread submissionCompletion = std::thread(WaitForSubmissionFinish,
// device.device, this, buf, fence, readySignal, bufsToClean);
// 	submissionCompletion.detach();
// }

// void TransferQueue::SubmitTransferCommandBuffer(VkCommandBuffer buf,
// std::vector<Signal> readySignal) { 	SubmitTransferCommandBuffer(buf,
// readySignal, {});
// }

// void TransferQueue::SubmitTransferCommandBufferAndWait(VkCommandBuffer buf) {

// 	VkFence fence;
// 	VkFenceCreateInfo fenceInfo =
// initializers::fenceCreateInfo(VK_FLAGS_NONE);
// 	VK_CHECK_RESULT(vkCreateFence(device.device, &fenceInfo, nullptr,
// &fence))

// 	transferCommandPool.SubmitOneTimeUseCommandBuffer(buf, fence);

// 	vkWaitForFences(device.device, 1, &fence, VK_TRUE,
// DEFAULT_FENCE_TIMEOUT); 	vkDestroyFence(device.device, fence, nullptr);

// 	transferCommandPool.FreeCommandBuffer(buf);
// }

void VulkanRenderer::SaveScreenshotNextFrame() { saveScreenshot = true; }

// Take a screenshot for the curretn swapchain image
// This is done using a blit from the swapchain image to a linear image whose
// memory content is then saved as a ppm image Getting the image date directly
// from a swapchain image wouldn't work as they're usually stored in an
// implementation dependant optimal tiling format Note: This requires the
// swapchain images to be created with the VK_IMAGE_USAGE_TRANSFER_SRC_BIT flag
// (see VulkanSwapChain::create)
void VulkanRenderer::SaveScreenshot() {
	if (saveScreenshot) {
		//std::string filename = "VulkanScreenshot.png";
		//// Get format properties for the swapchain color format
		//VkFormatProperties formatProps;

		//bool supportsBlit = true;

		//// Check blit support for source and destination

		//// Check if the device supports blitting from optimal images (the swapchain
		//// images are in optimal format)
		//vulkanSwapChain.swapChain;
		//vkGetPhysicalDeviceFormatProperties(device.physical_device,
		//	vulkanSwapChain.swapChainImageFormat,
		//	&formatProps);
		//if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		//	Log::Error << "Device does not support blitting from optimal tiled "
		//		"images, using copy instead of blit!"
		//		<< "\n";
		//	supportsBlit = false;
		//}

		//// Check if the device supports blitting to linear images
		//vkGetPhysicalDeviceFormatProperties(device.physical_device,
		//	VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
		//if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		//	Log::Error << "Device does not support blitting to linear tiled images, "
		//		"using copy instead of blit!"
		//		<< "\n";
		//	supportsBlit = false;
		//}

		//// Source for the copy is the last rendered swapchain image
		//VkImage srcImage =
		//	vulkanSwapChain.swapChainImages[vulkanSwapChain.currentBuffer];

		//// Create the linear tiled destination image to copy to and to read the
		//// memory from
		//VkImageCreateInfo imgCreateInfo(initializers::imageCreateInfo());
		//imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		//// Note that vkCmdBlitImage (if supported) will also do format conversions
		//// if the swapchain color format would differ
		//imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		//imgCreateInfo.extent.width = vulkanSwapChain.swapChainExtent.width;
		//imgCreateInfo.extent.height = vulkanSwapChain.swapChainExtent.height;
		//imgCreateInfo.extent.depth = 1;
		//imgCreateInfo.arrayLayers = 1;
		//imgCreateInfo.mipLevels = 1;
		//imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		//imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		//imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		//// Create the image
		//VkImage dstImage;
		//VK_CHECK_RESULT(
		//	vkCreateImage(device.device, &imgCreateInfo, nullptr, &dstImage));
		//// Create memory to back up the image
		//VkMemoryRequirements memRequirements;
		//VkMemoryAllocateInfo memAllocInfo(initializers::memoryAllocateInfo());
		//VkDeviceMemory dstImageMemory;
		//vkGetImageMemoryRequirements(device.device, dstImage, &memRequirements);
		//memAllocInfo.allocationSize = memRequirements.size;
		//// Memory must be host visible to copy from
		//memAllocInfo.memoryTypeIndex =
		//	device.getMemoryType(memRequirements.memoryTypeBits,
		//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//VK_CHECK_RESULT(vkAllocateMemory(device.device, &memAllocInfo, nullptr,
		//	&dstImageMemory));
		//VK_CHECK_RESULT(
		//	vkBindImageMemory(device.device, dstImage, dstImageMemory, 0));

		//// Do the actual blit from the swapchain image to our host visible
		//// destination image
		//VkCommandBuffer copyCmd = GetGraphicsCommandBuffer();

		//VkImageMemoryBarrier imageMemoryBarrier =
		//	initializers::imageMemoryBarrier();

		//// Transition destination image to transfer destination layout
		//InsertImageMemoryBarrier(
		//	copyCmd, dstImage, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
		//	VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// Transition swapchain image from present to transfer source layout
		//InsertImageMemoryBarrier(
		//	copyCmd, srcImage, VK_ACCESS_MEMORY_READ_BIT,
		//	VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		//	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// If source and destination support blit we'll blit as this also does
		//// automatic format conversion (e.g. from BGR to RGB)
		//if (supportsBlit) {
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
		//else {
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
		//InsertImageMemoryBarrier(
		//	copyCmd, dstImage, VK_ACCESS_TRANSFER_WRITE_BIT,
		//	VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//// Transition back the swap chain image after the blit is done
		//InsertImageMemoryBarrier(
		//	copyCmd, srcImage, VK_ACCESS_TRANSFER_READ_BIT,
		//	VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		//SubmitGraphicsCommandBufferAndWait(copyCmd);

		//// Get layout of the image (including row pitch)
		//VkImageSubresource subResource{};
		//subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//VkSubresourceLayout subResourceLayout;

		//vkGetImageSubresourceLayout(device.device, dstImage, &subResource,
		//	&subResourceLayout);

		//// Map image memory so we can start copying from it
		//const char *data;
		//const char *dataForSTB;
		//vkMapMemory(device.device, dstImageMemory, 0, VK_WHOLE_SIZE, 0,
		//	(void **)&data);
		//dataForSTB = data;
		//data += subResourceLayout.offset;

		// std::ofstream file(filename, std::ios::out | std::ios::binary);
		//
		//// ppm header
		// file << "P6\n" << vulkanSwapChain.swapChainExtent.width << "\n" <<
		// vulkanSwapChain.swapChainExtent.height << "\n" << 255 << "\n";
		//
		//// If source is BGR (destination is always RGB) and we can't use blit
		///(which does automatic conversion), we'll have to manually swizzle color
		///components
		// bool colorSwizzle = false;
		//// Check if source is BGR
		//// Note: Not complete, only contains most common and basic BGR surface
		///formats for demonstation purposes
		// if (!supportsBlit)
		//{
		//	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB,
		//VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM }; 	colorSwizzle =
		//(std::find(formatsBGR.begin(), formatsBGR.end(),
		//vulkanSwapChain.swapChainImageFormat) != formatsBGR.end());
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

		//int err = stbi_write_png(
		//	filename.c_str(), vulkanSwapChain.swapChainExtent.width,
		//	vulkanSwapChain.swapChainExtent.height, STBI_rgb_alpha, dataForSTB,
		//	vulkanSwapChain.swapChainExtent.width * STBI_rgb_alpha);
		//if (err == 0) {
		//	Log::Debug << "Screenshot saved to disk"
		//		<< "\n";
		//}
		//else {
		//	Log::Debug << "Failed to save screenshot!\nError code = " << err << "\n";
		//}

		//// Clean up resources
		//vkUnmapMemory(device.device, dstImageMemory);
		//vkFreeMemory(device.device, dstImageMemory, nullptr);
		//vkDestroyImage(device.device, dstImage, nullptr);

		saveScreenshot = false;
	}
}
