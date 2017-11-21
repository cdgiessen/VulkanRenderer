#include "Water.h"

#include <glm/gtc/matrix_transform.hpp>

Water::Water(int numCells, float posX, float posY, float sizeX, float sizeY) : numCells(numCells), pos(glm::vec3(posX, 0, posY)), size(glm::vec3(sizeX, 0, sizeY))
{
	WaterMesh = createFlatPlane(numCells, size);
}

Water::~Water() {
	CleanUp();
}


void Water::InitWater(std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->renderer = renderer;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptor(global, lighting);
	SetupPipeline();
}

void Water::ReinitWater(std::shared_ptr<VulkanRenderer> renderer)
{
	this->renderer = renderer;

	vkDestroyPipelineLayout(renderer->device.device, pipelineLayout, nullptr);
	vkDestroyPipeline(renderer->device.device, pipeline, nullptr);
	vkDestroyPipeline(renderer->device.device, wireframe, nullptr);

	SetupPipeline();
}

void Water::CleanUp()
{
	WaterModel.destroy(renderer->device);
	WaterVulkanTexture.destroy(renderer->device);

	modelUniformBuffer.cleanBuffer();

	vkDestroyDescriptorSetLayout(renderer->device.device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(renderer->device.device, descriptorPool, nullptr);

	vkDestroyPipelineLayout(renderer->device.device, pipelineLayout, nullptr);
	vkDestroyPipeline(renderer->device.device, pipeline, nullptr);
	vkDestroyPipeline(renderer->device.device, seascapePipeline, nullptr);
	vkDestroyPipeline(renderer->device.device, wireframe, nullptr);
}

void Water::LoadTexture(std::string filename) {
	WaterTexture = std::make_shared<Texture>();
	WaterTexture->loadFromFileRGBA(filename);
}

void Water::SetupUniformBuffer()
{
	renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &modelUniformBuffer, sizeof(ModelBufferObject));

	ModelBufferObject ubo = {};
	ubo.model = glm::translate(glm::mat4(), pos);
	ubo.normal = glm::transpose(glm::inverse(ubo.model));

	modelUniformBuffer.map(renderer->device.device);
	modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
	modelUniformBuffer.unmap();
}

void Water::SetupImage()
{
	WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);
}

void Water::SetupModel()
{
	WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);
}

void Water::SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting)
{
	//layout
	VkDescriptorSetLayoutBinding cboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
	VkDescriptorSetLayoutBinding uboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	VkDescriptorSetLayoutBinding lboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);
	VkDescriptorSetLayoutBinding samplerLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings = { cboLayoutBinding, uboLayoutBinding, lboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(renderer->device.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	//Pool
	std::vector<VkDescriptorPoolSize> poolSizesWater;
	poolSizesWater.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizesWater.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizesWater.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizesWater.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	VkDescriptorPoolCreateInfo poolInfoWater =
		initializers::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizesWater.size()),
			poolSizesWater.data(),
			1);

	if (vkCreateDescriptorPool(renderer->device.device, &poolInfoWater, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	//Descriptor set
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfoWater = initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

	if (vkAllocateDescriptorSets(renderer->device.device, &allocInfoWater, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
	modelUniformBuffer.setupDescriptor();

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &global.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &modelUniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lighting.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &WaterVulkanTexture.descriptor, 1));

	vkUpdateDescriptorSets(renderer->device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Water::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	std::shared_ptr<PipelineCreationObject> myPipe = pipeMan.CreatePipelineOutline();

	pipeMan.SetVertexShader(myPipe, loadShaderModule(renderer->device.device, "shaders/water.vert.spv"));
	pipeMan.SetFragmentShader(myPipe, loadShaderModule(renderer->device.device, "shaders/water.frag.spv"));
	pipeMan.SetVertexInput(myPipe, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(myPipe, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(myPipe, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(myPipe, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(myPipe, 1, 1, 0);
	pipeMan.SetRasterizer(myPipe, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(myPipe, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(myPipe, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(myPipe, VK_TRUE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, 
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending(myPipe, 1, &myPipe->colorBlendAttachment);
	pipeMan.SetDescriptorSetLayout(myPipe, { &descriptorSetLayout }, 1);

	pipelineLayout = pipeMan.BuildPipelineLayout(myPipe);
	pipeline = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);

	pipeMan.SetRasterizer(myPipe, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	wireframe = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(myPipe);
	pipeMan.SetVertexShader(myPipe, loadShaderModule(renderer->device.device, "shaders/Seascape.vert.spv"));
	pipeMan.SetFragmentShader(myPipe, loadShaderModule(renderer->device.device, "shaders/Seascape.frag.spv"));

	//pipelineLayout = pipeMan.BuildPipelineLayout(myPipe);
	seascapePipeline = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);

	
	pipeMan.CleanShaderResources(myPipe);
	/*
	VkShaderModule vertShaderModule = loadShaderModule(renderer->device.device, "shaders/water.vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule(renderer->device.device, "shaders/water.frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = initializers::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo();

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

	VkViewport viewport = initializers::viewport((float)viewPortWidth, (float)viewPortHeight, 0.0f, 1.0f);
	viewport.x = 0.0f;
	viewport.y = 0.0f;

	VkRect2D scissor = initializers::rect2D(viewPortWidth, viewPortHeight, 0, 0);

	VkPipelineViewportStateCreateInfo viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = initializers::pipelineRasterizationStateCreateInfo(
		VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.depthBiasEnable = VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multisampling = initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	multisampling.sampleShadingEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = initializers::pipelineColorBlendAttachmentState(
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);

	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

	VkPipelineColorBlendStateCreateInfo colorBlending = initializers::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

	if (vkCreatePipelineLayout(renderer->device.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(renderer->device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.lineWidth = 1.0f;

	if (device->physical_device_features.fillModeNonSolid == VK_TRUE) {
		if (vkCreateGraphicsPipelines(renderer->device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframe) != VK_SUCCESS) {
			throw std::runtime_error("failed to create wireframe pipeline!");
		}
	}

	vkDestroyShaderModule(renderer->device.device, vertShaderModule, nullptr);
	vkDestroyShaderModule(renderer->device.device, fragShaderModule, nullptr);

	//*/
}

void Water::BuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass)
{
	commandBuffers.resize(swapChain->swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = initializers::commandBufferAllocateInfo(renderer->device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size());

	if (vkAllocateCommandBuffers(renderer->device.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
		renderPassInfo.renderPass = *renderPass;
		renderPassInfo.framebuffer = swapChain->swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChain->swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.2f, 0.3f, 0.3f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VkDeviceSize offsets[] = { 0 };

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? wireframe : pipeline);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &WaterModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], WaterModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(WaterModel.indexCount), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void Water::RebuildCommandBuffer(std::shared_ptr<VulkanSwapChain> swapChain, std::shared_ptr<VkRenderPass> renderPass)
{
	vkFreeCommandBuffers(renderer->device.device, renderer->device.graphics_queue_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	BuildCommandBuffer(swapChain, renderPass);
}

void Water::UpdateUniformBuffer(float time, glm::mat4 view)
{
	ModelBufferObject ubo = {};
	ubo.model = glm::translate(glm::mat4(), pos);
	ubo.normal = glm::transpose(glm::inverse(glm::mat3(glm::mat4())));

	modelUniformBuffer.map(renderer->device.device);
	modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
	modelUniformBuffer.unmap();
}