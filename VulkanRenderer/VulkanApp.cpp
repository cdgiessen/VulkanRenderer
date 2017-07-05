#include "VulkanApp.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}



VulkanApp::VulkanApp()
{
	myMesh = createFlatPlane();
	mySecondMesh = createCube();
	camera = new Camera(glm::vec3(-2,2,0), glm::vec3(0,1,0), 0, -45);


	initWindow();
	initVulkan();
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

void VulkanApp::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	vulkanDevice.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(vulkanDevice.window, this);
	glfwSetWindowSizeCallback(vulkanDevice.window, onWindowResized);
	glfwSetCursorPosCallback(vulkanDevice.window, onMouseMoved);
	glfwSetMouseButtonCallback(vulkanDevice.window, onMouseClicked);
	SetMouseControl(true);
}

void VulkanApp::initVulkan() {
	vulkanDevice.initVulkanDevice(vulkanSwapChain.surface);

	vulkanSwapChain.initSwapChain(vulkanDevice.instance,vulkanDevice.physical_device, vulkanDevice.device, vulkanDevice.window);

	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	//createCommandPool();
	createDepthResources();
	createFramebuffers();

	createTextureImage();
	createTextureImageView();
	createTextureSampler();
		createMeshBuffers(myMesh); 
		createMeshBuffers(mySecondMesh);
	createUniformBuffer();
		//createUniformBufferCUBE();
	createDescriptorPool();
	createDescriptorSet(descriptorSetOne);
	//createDescriptorSet(descriptorSetTwo);
	createCommandBuffers();
	createSemaphores();
}

void VulkanApp::mainLoop() {
	while (!glfwWindowShouldClose(vulkanDevice.window)) {
		glfwPollEvents();
		handleInputs();

		updateUniformBuffer();
		drawFrame();
	}

	vkDeviceWaitIdle(vulkanDevice.device);
}

void VulkanApp::cleanup() {
	cleanupSwapChain();

	vkDestroySampler(vulkanDevice.device, textureSampler, nullptr);
	vkDestroyImageView(vulkanDevice.device, textureImageView, nullptr);

	vkDestroyImage(vulkanDevice.device, textureImage, nullptr);
	vkFreeMemory(vulkanDevice.device, textureImageMemory, nullptr);

	vkDestroyDescriptorPool(vulkanDevice.device, descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(vulkanDevice.device, descriptorSetLayout, nullptr);

	myMesh->cleanup(vulkanDevice.device);
	mySecondMesh->cleanup(vulkanDevice.device);

	vkDestroyBuffer(vulkanDevice.device, uniformBuffer, nullptr);
	vkFreeMemory(vulkanDevice.device, uniformBufferMemory, nullptr);
	
	//vkDestroyBuffer(vulkanDevice.device, uniformBufferCube, nullptr);
	//vkFreeMemory(vulkanDevice.device, uniformBufferMemoryCube, nullptr);
	
	vkDestroySemaphore(vulkanDevice.device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(vulkanDevice.device, imageAvailableSemaphore, nullptr);

	
	vulkanDevice.cleanup(vulkanSwapChain.surface);
}

void VulkanApp::cleanupSwapChain() {
	

	vkDestroyImageView(vulkanDevice.device, depthImageView, nullptr);
	vkDestroyImage(vulkanDevice.device, depthImage, nullptr);
	vkFreeMemory(vulkanDevice.device, depthImageMemory, nullptr);

	for (size_t i = 0; i < vulkanSwapChain.swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(vulkanDevice.device, vulkanSwapChain.swapChainFramebuffers[i], nullptr);
	}

	vkFreeCommandBuffers(vulkanDevice.device, vulkanDevice.commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(vulkanDevice.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vulkanDevice.device, pipelineLayout, nullptr);
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
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();
}

//7
void VulkanApp::createImageViews() {
	vulkanSwapChain.swapChainImageViews.resize(vulkanSwapChain.swapChainImages.size());

	for (uint32_t i = 0; i < vulkanSwapChain.swapChainImages.size(); i++) {
		vulkanSwapChain.swapChainImageViews[i] = createImageView(vulkanSwapChain.swapChainImages[i], vulkanSwapChain.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
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
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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

//9
void VulkanApp::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(vulkanDevice.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

//9
void VulkanApp::createGraphicsPipeline() {

	VkShaderModule vertShaderModule = loadShaderModule(vulkanDevice.device, "shaders/vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule(vulkanDevice.device, "shaders/frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0 ,VK_FALSE);

	VkViewport viewport = initializers::viewport((float)vulkanSwapChain.swapChainExtent.width, (float)vulkanSwapChain.swapChainExtent.height, 0.0f,1.0f);
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	VkRect2D scissor = initializers::rect2D(vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height,0,0);

	VkPipelineViewportStateCreateInfo viewportState = initializers::pipelineViewportStateCreateInfo(1,1);
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;
	
	VkPipelineRasterizationStateCreateInfo rasterizer = initializers::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	multisampling.sampleShadingEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE,VK_TRUE, VK_COMPARE_OP_LESS);
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(1);
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(vulkanDevice.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(vulkanDevice.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(vulkanDevice.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(vulkanDevice.device, vertShaderModule, nullptr);
}

/*
//10
void VulkanApp::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vulkanDevice.physical_device, vulkanSwapChain.surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	if (vkCreateCommandPool(vulkanDevice.device, &poolInfo, nullptr, &vulkanDevice.commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}
}
*/
//11
void VulkanApp::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	createImage(vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

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

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
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
void VulkanApp::createTextureImage() {
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

	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vulkanDevice.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanDevice.device, stagingBufferMemory, nullptr);
}

//14
void VulkanApp::createTextureImageView() {
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

//15
void VulkanApp::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
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

	if (vkCreateSampler(vulkanDevice.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo();
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(vulkanDevice.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
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
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

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
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void VulkanApp::createMeshBuffers(Mesh* mesh) {
	//mesh->createMeshBuffers(vulkanDevice.device, vulkanDevice.physical_device);
	createVertexBuffer(mesh);
	createIndexBuffer(mesh);
}

//15
void VulkanApp::createVertexBuffer(Mesh* mesh) {
	VkDeviceSize bufferSize = sizeof(mesh->vertices[0]) * mesh->vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkanDevice.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh->vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(vulkanDevice.device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh->vertexBuffer, mesh->vertexBufferMemory);

	copyBuffer(stagingBuffer, mesh->vertexBuffer, bufferSize);

	vkDestroyBuffer(vulkanDevice.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanDevice.device, stagingBufferMemory, nullptr);
}

//16
void VulkanApp::createIndexBuffer(Mesh* mesh) {
	VkDeviceSize bufferSize = sizeof(mesh->indices[0]) * mesh->indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vulkanDevice.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh->indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vulkanDevice.device, stagingBufferMemory);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh->indexBuffer, mesh->indexBufferMemory);

	copyBuffer(stagingBuffer, mesh->indexBuffer, bufferSize);

	vkDestroyBuffer(vulkanDevice.device, stagingBuffer, nullptr);
	vkFreeMemory(vulkanDevice.device, stagingBufferMemory, nullptr);
}

//17
void VulkanApp::createUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}
/*
//for the cube
void VulkanApp::createUniformBufferCUBE() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBufferCube, uniformBufferMemoryCube);
}
*/
//18
void VulkanApp::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1/*max sets*/);

	if (vkCreateDescriptorPool(vulkanDevice.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

//19
void VulkanApp::createDescriptorSet(VkDescriptorSet& descriptorSet) {
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

	if (vkAllocateDescriptorSets(vulkanDevice.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo imageInfo = initializers::descriptorImageInfo(textureSampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(vulkanDevice.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		//first mesh
		VkBuffer vertexBuffers[] = { myMesh->vertexBuffer};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], myMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetOne, 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(myMesh->indices.size()), 1, 0, 0, 0);

		//second mesh

		vertexBuffers[0] = mySecondMesh->vertexBuffer;
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], mySecondMesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetOne, 0, nullptr);
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mySecondMesh->indices.size()), 1, 0, 0, 0);

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

void VulkanApp::updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::mat4();
	ubo.view = camera->GetViewMatrix();
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChain.swapChainExtent.width / (float)vulkanSwapChain.swapChainExtent.height, 0.1f, 1000.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(vulkanDevice.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vulkanDevice.device, uniformBufferMemory);
}

void VulkanApp::updateUniformBufferCUBE() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f) / 4.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = camera->GetViewMatrix();
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChain.swapChainExtent.width / (float)vulkanSwapChain.swapChainExtent.height, 0.1f, 1000.0f);
	ubo.proj[1][1] *= -1;

	/*
	void* data;
	vkMapMemory(vulkanDevice.device, uniformBufferMemoryCube, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(vulkanDevice.device, uniformBufferMemoryCube);*/
}

void VulkanApp::handleInputs() {
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(vulkanDevice.window, true);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_ENTER) == GLFW_PRESS) {
		SetMouseControl(false);
	}
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera->ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(vulkanDevice.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera->ProcessKeyboard(DOWN, deltaTime);

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
	if(value)
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
