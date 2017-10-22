#include "VulkanApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "..\vulkan\VulkanInitializers.hpp"

VulkanApp::VulkanApp()
{
	//camera = new Camera(glm::vec3(-2,2,0), glm::vec3(0,1,0), 0, -45);
	timeManager = new TimeManager();

	window = new Window();
	window->createWindow(glm::uvec2(WIDTH, HEIGHT));
	vulkanDevice.window = window->getWindowContext();
	SetMouseControl(true);

	initVulkan();
	pipelineManager = new VulkanPipeline(&vulkanDevice);
	
	scene = new Scene(&vulkanDevice);
	scene->PrepareScene(*pipelineManager, renderPass, vulkanSwapChain);

	PrepareImGui();
	createSemaphores();

	mainLoop();
	cleanup(); //all resources
}


VulkanApp::~VulkanApp()
{
}

void VulkanApp::initVulkan() {
	vulkanDevice.initVulkanDevice(vulkanSwapChain.surface);

	vulkanSwapChain.initSwapChain(vulkanDevice.instance, vulkanDevice.physical_device, vulkanDevice.device, vulkanDevice.window);

	//createImageViews();
	createRenderPass();

	createDepthResources();
	createFramebuffers();

	createCommandBuffers();
}

void VulkanApp::mainLoop() {
	int i;
	while (!window->CheckForWindowClose()) {
		timeManager->StartFrameTimer();

		InputDirector::GetInstance().UpdateInputs();
		HandleInputs();

		//updateScene();
		scene->UpdateScene(*pipelineManager, renderPass, vulkanSwapChain, timeManager);
		BuildImgui();

		buildCommandBuffers();
		drawFrame();
		
		InputDirector::GetInstance().ResetReleasedInput();
		timeManager->EndFrameTimer();
		//std::cout << "main loop breaker. Break me if you want to stop after every frame!" << std::endl;
	}

	vkDeviceWaitIdle(vulkanDevice.device);

}

void VulkanApp::cleanup() {
	CleanUpImgui();

	scene->CleanUpScene();

	cleanupSwapChain();

	vkDestroySemaphore(vulkanDevice.device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(vulkanDevice.device, imageAvailableSemaphore, nullptr);

	for (auto& shaderModule : shaderModules)
	{
		vkDestroyShaderModule(vulkanDevice.device, shaderModule, nullptr);
	}
	
	vulkanDevice.cleanup(vulkanSwapChain.surface);
}

void VulkanApp::cleanupSwapChain() {
	

	vkDestroyImageView(vulkanDevice.device, depthImageView, nullptr);
	vkDestroyImage(vulkanDevice.device, depthImage, nullptr);
	vkFreeMemory(vulkanDevice.device, depthImageMemory, nullptr);

	for (size_t i = 0; i < vulkanSwapChain.swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(vulkanDevice.device, vulkanSwapChain.swapChainFramebuffers[i], nullptr);
	}

	vkDestroyRenderPass(vulkanDevice.device, renderPass, nullptr);

	for (size_t i = 0; i < vulkanSwapChain.swapChainImageViews.size(); i++) {
		vkDestroyImageView(vulkanDevice.device, vulkanSwapChain.swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(vulkanDevice.device, vulkanSwapChain.swapChain, nullptr);
}

void VulkanApp::recreateSwapChain() {
	vkDeviceWaitIdle(vulkanDevice.device);

	cleanupSwapChain();

	vulkanSwapChain.recreateSwapChain(vulkanDevice.window);
	
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	
	scene->ReInitScene(*pipelineManager, renderPass, vulkanSwapChain);

	//frameIndex = 1; //cause it needs it to be synced back to zero (yes I know it says one, thats intended, build command buffers uses the "next" frame index since it has to sync with the swapchain so it starts at one....)
	reBuildCommandBuffers();
}

void VulkanApp::reBuildCommandBuffers() {
	vkFreeCommandBuffers(vulkanDevice.device, vulkanDevice.graphics_queue_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkResetCommandPool(vulkanDevice.device, vulkanDevice.graphics_queue_command_pool, 0);

	createCommandBuffers();
	buildCommandBuffers();
}


//8
void VulkanApp::createRenderPass() {
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
	depthAttachment.format = VK_FORMAT_D32_SFLOAT_S8_UINT; //findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = initializers::renderPassCreateInfo();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(vulkanDevice.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

//11
void VulkanApp::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	depthFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;

	createImage(vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(vulkanDevice.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	
	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	//VkCommandBuffer copyBuf = vulkanDevice.createCommandBuffer(vulkanDevice.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	//setImageLayout(copyBuf, depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIONAL, 
	//vulkanDevice.flushCommandBuffer(copyBuf, vulkanDevice.graphics_queue, true);
}

VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vulkanDevice.physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanApp::findDepthFormat() {
	return findSupportedFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

//12
void VulkanApp::createFramebuffers() {
	vulkanSwapChain.swapChainFramebuffers.resize(vulkanSwapChain.swapChainImageViews.size());

	for (size_t i = 0; i < vulkanSwapChain.swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			vulkanSwapChain.swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = initializers::framebufferCreateInfo();
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = vulkanSwapChain.swapChainExtent.width;
		framebufferInfo.height = vulkanSwapChain.swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(vulkanDevice.device, &framebufferInfo, nullptr, &vulkanSwapChain.swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}


void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo = initializers::imageCreateInfo();
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(vulkanDevice.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(vulkanDevice.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = initializers::memoryAllocateInfo();
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(vulkanDevice.physical_device, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vulkanDevice.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(vulkanDevice.device, image, imageMemory, 0);
}

void VulkanApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = initializers::imageMemoryBarrier();
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) { //has stencil component
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(vulkanDevice.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(vulkanDevice.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(vulkanDevice.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vulkanDevice.graphics_queue);

	vkFreeCommandBuffers(vulkanDevice.device, vulkanDevice.graphics_queue_command_pool, 1, &commandBuffer);
}

void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}


//20
void VulkanApp::createCommandBuffers() {

	commandBuffers.resize(vulkanSwapChain.swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(vulkanDevice.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());

	if (vkAllocateCommandBuffers(vulkanDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

}

void VulkanApp::buildCommandBuffers(){
	
	for (size_t i = 0; i < commandBuffers.size(); i++) {

		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = vulkanSwapChain.swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vulkanSwapChain.swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
		clearValues[1].depthStencil = { 0.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkDeviceSize offsets[] = { 0 };

		scene->RenderScene(commandBuffers[i], wireframe);

		//Imgui rendering
		ImGui_ImplGlfwVulkan_Render(commandBuffers[i]);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

//Meant to be used in conjunction with secondary command buffers
void VulkanApp::CreatePrimaryCommandBuffer() {
	commandBuffers.resize(vulkanSwapChain.swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(vulkanDevice.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());

	if (vkAllocateCommandBuffers(vulkanDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
	
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
		clearValues[1].depthStencil = { 0.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = vulkanSwapChain.swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vulkanSwapChain.swapChainExtent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		//Render stuff

		// Contains the list of secondary command buffers to be executed
		std::vector<VkCommandBuffer> commandBuffers;



		vkCmdExecuteCommands(commandBuffers[i], (uint32_t)commandBuffers.size(), commandBuffers.data());
		
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

}

//21
void VulkanApp::createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = initializers::semaphoreCreateInfo();

	if (vkCreateSemaphore(vulkanDevice.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(vulkanDevice.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

VkPipelineShaderStageCreateInfo VulkanApp::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	shaderStage.module = loadShaderModule(vulkanDevice.device, fileName.c_str());

	shaderStage.pName = "main"; 
	assert(shaderStage.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

static void imgui_check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

void  VulkanApp::PrepareImGui()
{
	//Creates a descriptor pool for imgui
	{	VkDescriptorPoolSize pool_size[11] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * 11;
		pool_info.poolSizeCount = 11;
		pool_info.pPoolSizes = pool_size;
		VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice.device, &pool_info, VK_NULL_HANDLE, &imgui_descriptor_pool));
	}

	ImGui_ImplGlfwVulkan_Init_Data init_data = {};
	init_data.allocator = VK_NULL_HANDLE;
	init_data.gpu = vulkanDevice.physical_device;
	init_data.device = vulkanDevice.device;
	init_data.render_pass = renderPass;
	init_data.pipeline_cache = VK_NULL_HANDLE;
	init_data.descriptor_pool = imgui_descriptor_pool;
	init_data.check_vk_result = imgui_check_vk_result;
	
	ImGui_ImplGlfwVulkan_Init(vulkanDevice.window, false, &init_data);

	VkCommandBuffer fontUploader = vulkanDevice.createCommandBuffer(vulkanDevice.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	ImGui_ImplGlfwVulkan_CreateFontsTexture(fontUploader);
	vulkanDevice.flushCommandBuffer(fontUploader, vulkanDevice.graphics_queue, true);
}

// Build imGui windows and elements
void VulkanApp::BuildImgui() {
	imGuiTimer.StartTimer();

	ImGui_ImplGlfwVulkan_NewFrame();
	
	bool show_test_window = true;
	bool show_log_window = true;

	//Application Debuf info
	{
		ImGui::Begin("Application Debug Information", &show_test_window);
		static float f = 0.0f;
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("Delta Time: %f(s)", timeManager->GetDeltaTime());
		ImGui::Text("Start Time: %f(s)", timeManager->GetRunningTime());
		ImGui::Text("Time for last frame %f(s)", timeManager->GetPreviousFrameTime());
		ImGui::PlotLines("Frame Times", &timeManager->GetFrameTimeHistory()[0], 50, 0, "", timeManager->GetFrameTimeMin(), timeManager->GetFrameTimeMax(), ImVec2(0, 80));
		ImGui::Text("Camera");
		ImGui::InputFloat3("position", &scene->GetCamera()->Position.x, 2);
		ImGui::InputFloat3("rotation", &scene->GetCamera()->Front.x, 2);
		ImGui::Text("Camera Movement Speed");
		ImGui::SliderFloat("float", &scene->GetCamera()->MovementSpeed, 0.1f, 100.0f);
		//ImGui::ColorEdit3("clear color", (float*)&clear_color);
		//if (ImGui::Button("Test Window")) show_test_window ^= 1;
		//if (ImGui::Button("Another Window")) show_another_window ^= 1;
		//ImGui::Text("Frame index (of swap chain) : %u", (frameIndex));
		ImGui::End();
	}

	scene->UpdateSceneGUI();
	
	{ //simple app log - taken from imgui exampels
		appLog.Draw("Example: Log", &show_log_window);
	}

	nodeGraph_terrain.DrawGraph();

	imGuiTimer.EndTimer();
	//std::cout << imGuiTimer.GetElapsedTimeNanoSeconds() << std::endl;
}

//Release associated resources and shutdown imgui
void VulkanApp::CleanUpImgui() {
	ImGui_ImplGlfwVulkan_Shutdown();
	vkDestroyDescriptorPool(vulkanDevice.device, imgui_descriptor_pool, VK_NULL_HANDLE);
}

void VulkanApp::HandleInputs() {
	//std::cout << camera->Position.x << " " << camera->Position.y << " " << camera->Position.z << std::endl;

	if (Input::GetKey(GLFW_KEY_W))
		scene->GetCamera()->ProcessKeyboard(FORWARD, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_S))
		scene->GetCamera()->ProcessKeyboard(BACKWARD, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_A))
		scene->GetCamera()->ProcessKeyboard(LEFT, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_D))
		scene->GetCamera()->ProcessKeyboard(RIGHT, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_SPACE))
		scene->GetCamera()->ProcessKeyboard(UP, (float)timeManager->GetDeltaTime());
	if (Input::GetKey(GLFW_KEY_LEFT_SHIFT))
		scene->GetCamera()->ProcessKeyboard(DOWN, (float)timeManager->GetDeltaTime());

	if (Input::GetKeyDown(GLFW_KEY_0))
		appLog.AddLog("ZERO WAS HIT REPEAT ZERO WAS HIT\n");

	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		window->SetWindowToClose();
	if (Input::GetKeyDown(GLFW_KEY_ENTER))
		SetMouseControl(!mouseControlEnabled);

	if (Input::GetKey(GLFW_KEY_E))
		scene->GetCamera()->ChangeCameraSpeed(UP);
	if (Input::GetKey(GLFW_KEY_Q))
		scene->GetCamera()->ChangeCameraSpeed(DOWN);

	if (Input::GetKeyDown(GLFW_KEY_N))
		scene->drawNormals = !scene->drawNormals;
	if (Input::GetKeyDown(GLFW_KEY_X  )) {
		wireframe = !wireframe;
		reBuildCommandBuffers();
		std::cout << "wireframe toggled" << std::endl;
	}

	if (Input::GetKeyDown(GLFW_KEY_F)) {
		//walkOnGround = !walkOnGround;
		//std::cout << "flight mode toggled " << std::endl;
	}

	if (mouseControlEnabled)
		scene->GetCamera()->ProcessMouseMovement(Input::GetMouseChangeInPosition().x, Input::GetMouseChangeInPosition().y);

	if (Input::GetMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		if (!ImGui::IsMouseHoveringAnyWindow()) {
			SetMouseControl(true);
		}
	}
}

void VulkanApp::SetMouseControl(bool value) {
	mouseControlEnabled = value;
	if(mouseControlEnabled)
		glfwSetInputMode(vulkanDevice.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode(vulkanDevice.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void VulkanApp::drawFrame() {
	uint32_t frameIndex; //which of the swapchain images the app is rendering to
	VkResult result = vkAcquireNextImageKHR(vulkanDevice.device, vulkanSwapChain.swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &frameIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = initializers::submitInfo();

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(vulkanDevice.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { vulkanSwapChain.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &frameIndex;

	result = vkQueuePresentKHR(vulkanDevice.present_queue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(vulkanDevice.present_queue);
}
