#include "VulkanApp.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanInitializers.hpp"

VulkanApp::VulkanApp()
{
	camera = new Camera(glm::vec3(-2,2,0), glm::vec3(0,1,0), 0, -45);

	initWindow();
	initVulkan();
	prepareScene();
	prepareImGui();
	createCommandBuffers();

	startTime = std::chrono::high_resolution_clock::now();
	mainLoop();
	cleanup();
}


VulkanApp::~VulkanApp()
{
}

static void onWindowResized(GLFWwindow* window, int width, int height) {
	if (width == 0 || height == 0) return;

	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->recreateSwapChain();
}

void onMouseMoved(GLFWwindow* window, double xpos, double ypos)
{
	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->MouseMoved(xpos, ypos);
}

void onMouseClicked(GLFWwindow* window, int button, int action, int mods) {
	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->MouseClicked(button, action, mods);
}

void onKeyboardEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
	VulkanApp* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
	app->KeyboardEvent(key, scancode, action, mods);
}

void VulkanApp::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	vulkanDevice.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(vulkanDevice.window, this);
	glfwSetWindowSizeCallback(vulkanDevice.window, onWindowResized);
	glfwSetCursorPosCallback(vulkanDevice.window, onMouseMoved);
	glfwSetMouseButtonCallback(vulkanDevice.window, onMouseClicked);
	glfwSetKeyCallback(vulkanDevice.window, onKeyboardEvent);
	SetMouseControl(true);
}

void VulkanApp::initVulkan() {
	vulkanDevice.initVulkanDevice(vulkanSwapChain.surface);

	vulkanSwapChain.initSwapChain(vulkanDevice.instance, vulkanDevice.physical_device, vulkanDevice.device, vulkanDevice.window);

	//createImageViews();
	createRenderPass();

	createDepthResources();
	createFramebuffers();

}

void VulkanApp::prepareScene(){

	pointLights.resize(5);
	pointLights[0] = PointLight(glm::vec4(0, 10, 0, 1),	  glm::vec4(1, 1, 1, 1), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[1] = PointLight(glm::vec4(10, 10, 50, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[2] = PointLight(glm::vec4(50, 10, 10, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[3] = PointLight(glm::vec4(50, 10, 50, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[4] = PointLight(glm::vec4(75, 10, 75, 1), glm::vec4(1, 1, 1, 1), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));

	createUniformBuffers();
	createDescriptorSets();

	skybox = new Skybox();
	skybox->InitSkybox(&vulkanDevice, "Resources/Textures/Skybox2", ".png", renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	
	cubeObject = new GameObject();
	cubeObject->LoadModel(createCube());
	cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	cubeObject->InitGameObject(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);

	int terWidth = 100;
	terrain = new Terrain(200, 0, 0, terWidth, terWidth);
	terrain->LoadTexture("Resources/Textures/lowPolyScatter.png");
	terrain->InitTerrain(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);

	water = new Water(terWidth, 0, 0, terWidth, terWidth);
	water->LoadTexture("Resources/Textures/TileableWaterTexture.jpg");
	water->InitWater(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);

	createSemaphores();
}

void VulkanApp::mainLoop() {
	while (!glfwWindowShouldClose(vulkanDevice.window)) {
		glfwPollEvents();
		HandleInputs();
		updateUniformBuffers();
		drawFrame();
		frameCount++;
		//std::cout << frameCount << std::endl;
	}

	vkDeviceWaitIdle(vulkanDevice.device);
}

void VulkanApp::cleanup() {
	//delete imGui;

	skybox->CleanUp();
	cubeObject->CleanUp();
	terrain->CleanUp();
	water->CleanUp();

	cleanupSwapChain();

	globalVariableBuffer.cleanBuffer();
	lightsInfoBuffer.cleanBuffer();
	
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
	
	skybox->ReinitSkybox(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	cubeObject->ReinitGameObject(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);
	terrain->ReinitTerrain(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);
	water->ReinitWater(&vulkanDevice, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);

	reBuildCommandBuffers();
}

void VulkanApp::reBuildCommandBuffers() {
	vkFreeCommandBuffers(vulkanDevice.device, vulkanDevice.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	createCommandBuffers();
	terrain->RebuildCommandBuffer(&vulkanSwapChain, &renderPass);
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
	depthAttachment.format = findDepthFormat();
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

	createImage(vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(vulkanDevice.device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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

bool VulkanApp::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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

//13
void VulkanApp::createTextureImage(VkImage image, VkDeviceMemory imageMemory) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("Resources/Textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkanDevice.device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vulkanDevice.device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vulkanDevice.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanDevice.device, stagingBufferMemory, nullptr);
}

//15
void VulkanApp::createTextureSampler(VkSampler* textureSampler) {
	VkSamplerCreateInfo samplerInfo = initializers::samplerCreateInfo();
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(vulkanDevice.device, &samplerInfo, nullptr, textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
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

		if (hasStencilComponent(format)) {
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

void VulkanApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

//17
void VulkanApp::createUniformBuffers() {
	vulkanDevice.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &globalVariableBuffer, sizeof(GlobalVariableUniformBuffer));
	vulkanDevice.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &lightsInfoBuffer, sizeof(PointLight) * pointLights.size());

	for (int i = 0; i < pointLights.size(); i++)
	{
		PointLight lbo;
		lbo.lightPos = pointLights[i].lightPos;
		lbo.color = pointLights[i].color;
		lbo.attenuation = pointLights[i].attenuation;

		lightsInfoBuffer.map(vulkanDevice.device, sizeof(PointLight), i * sizeof(PointLight));
		lightsInfoBuffer.copyTo(&lbo, sizeof(lbo));
		lightsInfoBuffer.unmap();
	}
}

//19
void VulkanApp::createDescriptorSets() {
	globalVariableBuffer.setupDescriptor();
	lightsInfoBuffer.setupDescriptor();
}

void VulkanApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo(usage,size);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(vulkanDevice.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vulkanDevice.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = initializers::memoryAllocateInfo();
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(vulkanDevice.physical_device, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vulkanDevice.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(vulkanDevice.device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(vulkanDevice.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

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

	vkFreeCommandBuffers(vulkanDevice.device, vulkanDevice.commandPool, 1, &commandBuffer);
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
	terrain->BuildCommandBuffer(&vulkanSwapChain, &renderPass);


	commandBuffers.resize(vulkanSwapChain.swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(vulkanDevice.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());

	if (vkAllocateCommandBuffers(vulkanDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

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
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkDeviceSize offsets[] = { 0 };

		//terrain mesh
		
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? terrain->wireframe : terrain->pipeline);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, terrain->pipelineLayout, 0, 1, &terrain->descriptorSet, 0, nullptr);
		
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &terrain->terrainModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], terrain->terrainModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(terrain->terrainModel.indexCount), 1, 0, 0, 0);
		
		//water
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, water->pipelineLayout, 0, 1, &water->descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? water->wireframe : water->pipeline);
		
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &water->WaterModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], water->WaterModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(water->WaterModel.indexCount), 1, 0, 0, 0);

		//cubeObject
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, cubeObject->pipelineLayout, 0, 1, &cubeObject->descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? cubeObject->wireframe : cubeObject->pipeline);

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &cubeObject->gameObjectModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], cubeObject->gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(cubeObject->gameObjectModel.indexCount), 1, 0, 0, 0);


		//skybox
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->pipelineLayout, 0, 1, &skybox->descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->pipeline);

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &skybox->model.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], skybox->model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(skybox->model.indexCount), 1, 0, 0, 0);


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

void  VulkanApp::prepareImGui()
{
	//imGui = new ImGUI();
	//imGui->init((float)WIDTH, (float)HEIGHT);
	//imGui->initResources(renderPass, vulkanDevice.graphics_queue);
}

void VulkanApp::updateImGui() {
	//ImGuiIO& io = ImGui::GetIO();
	//
	//io.DisplaySize = ImVec2((float)WIDTH, (float)HEIGHT);
	//io.DeltaTime = deltaTime;
	//
	//io.MousePos = ImVec2(lastX, lastY);
	//io.MouseDown[0] = (((glfwGetMouseButton(vulkanDevice.window,GLFW_MOUSE_BUTTON_1)) != 0));
	//io.MouseDown[1] = (((glfwGetMouseButton(vulkanDevice.window,GLFW_MOUSE_BUTTON_2)) != 0));
}

void VulkanApp::newGuiFrame() {
	// Starts a new imGui frame and sets up windows and ui elements
	
	
	//ImGui::NewFrame();

	// Init imGui windows and elements

	//ImVec4 clear_color = ImColor(114, 144, 154);
	//static float f = 0.0f;
	//ImGui::Text("window title");
	//ImGui::Text("graphics card");
	//
	//// Update frame time display
	//if (false) {
	//	/*std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
	//	float frameTime = 1000.0f / (example->frameTimer * 1000.0f);
	//	uiSettings.frameTimes.back() = frameTime;
	//	if (frameTime < uiSettings.frameTimeMin) {
	//		uiSettings.frameTimeMin = frameTime;
	//	}
	//	if (frameTime > uiSettings.frameTimeMax) {
	//		uiSettings.frameTimeMax = frameTime;
	//	}*/
	//}
	//
	////ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));
	//
	//ImGui::Text("Camera");
	//ImGui::InputFloat3("position", &camera->Position.x, 2);
	//ImGui::InputFloat3("rotation", &camera->Front.x, 2);
	//
	//ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
	//ImGui::Begin("Example settings");
	////ImGui::Checkbox("Render models", &uiSettings.displayModels);
	////ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
	////ImGui::Checkbox("Display background", &uiSettings.displayBackground);
	////ImGui::Checkbox("Animate light", &uiSettings.animateLight);
	////ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
	//ImGui::End();
	//
	//ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
	//ImGui::ShowTestWindow();
	//
	//// Render to generate draw buffers
	//ImGui::Render();
	
}

void VulkanApp::updateUniformBuffers() {
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	framesOverTime = (float)frameCount / time;
	//std::cout << "Frames over time: " << (float)frameCount/time << " at time = " << time << std::endl;

	GlobalVariableUniformBuffer cbo = {};
	cbo.view = camera->GetViewMatrix();
	cbo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChain.swapChainExtent.width / (float)vulkanSwapChain.swapChainExtent.height, 0.1f, 1000.0f);
	cbo.proj[1][1] *= -1;
	cbo.cameraDir = camera->Front;
	cbo.time = time;

	globalVariableBuffer.map(vulkanDevice.device);
	globalVariableBuffer.copyTo(&cbo, sizeof(cbo));
	globalVariableBuffer.unmap();

	cubeObject->UpdateUniformBuffer(time);

	skybox->UpdateUniform(cbo.proj, camera->GetViewMatrix());
	water->UpdateUniformBuffer(time, camera->GetViewMatrix());
}

void VulkanApp::HandleInputs() {
	//std::cout << camera->Position.x << " " << camera->Position.y << " " << camera->Position.z << std::endl;

	if (keys[GLFW_KEY_W])
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera->ProcessKeyboard(RIGHT, deltaTime);
	if (keys[GLFW_KEY_SPACE])
		camera->ProcessKeyboard(UP, deltaTime);
	if (keys[GLFW_KEY_LEFT_SHIFT])
		camera->ProcessKeyboard(DOWN, deltaTime);

}

void VulkanApp::KeyboardEvent(int key, int scancode, int action, int mods) {
	
	if (key == GLFW_KEY_ESCAPE  && action == GLFW_PRESS)
		glfwSetWindowShouldClose(vulkanDevice.window, true);
	if (key == GLFW_KEY_ENTER  && action == GLFW_PRESS)
		SetMouseControl(!mouseControlEnabled);

	if (key == GLFW_KEY_E)
		camera->ChangeCameraSpeed(UP);
	if (key == GLFW_KEY_Q )
		camera->ChangeCameraSpeed(DOWN);
	//std::cout << camera->MovementSpeed << std::endl;
	if (key == GLFW_KEY_X  && action == GLFW_PRESS) {
		wireframe = !wireframe;
		reBuildCommandBuffers();
		std::cout << "wireframe toggled" << std::endl;
	}

	if (action == GLFW_PRESS)
		keys[key] = true;
	if (action == GLFW_RELEASE)
		keys[key] = false;

}

void VulkanApp::MouseMoved(double xpos, double ypos) {
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = float(xpos - lastX);
	float yoffset = float(lastY - ypos); // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;
	if(mouseControlEnabled)
		camera->ProcessMouseMovement(xoffset, yoffset);
}

void VulkanApp::MouseClicked(int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		SetMouseControl(true);
	
}

void VulkanApp::SetMouseControl(bool value) {
	mouseControlEnabled = value;
	if(mouseControlEnabled)
		glfwSetInputMode(vulkanDevice.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode(vulkanDevice.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void VulkanApp::drawFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vulkanDevice.device, vulkanSwapChain.swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

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

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(vulkanDevice.present_queue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(vulkanDevice.present_queue);
}
