#pragma once

#include "Skybox.h"
#include "..\core\Texture.h"
#include "..\vulkan\VulkanTexture.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanDevice.hpp"


Skybox::Skybox() {};
Skybox::~Skybox() {};

void Skybox::CleanUp() {
	model.destroy();
	vulkanCubeMap.destroy();

	skyboxUniformBuffer.cleanBuffer();

	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);

	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
}

void Skybox::InitSkybox(VulkanDevice* device, std::string filename, std::string fileExt, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight) {
	this->device = device;

	LoadSkyboxData(filename, fileExt);


	SetupUniformBuffer();
	SetupCubeMapImage();
	SetupDescriptor();
	SetupPipeline(pipelineManager, renderPass, viewPortWidth, viewPortHeight);

}

void Skybox::ReinitSkybox(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight){
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);

	SetupPipeline(pipelineManager, renderPass, viewPortWidth, viewPortHeight);
}


void Skybox::LoadSkyboxData(std::string skyboxImageFile, std::string fileExt) {
	model.loadFromFile("Resources/Models/cube.obj", device, device->graphics_queue);

	skyboxCubeMap.loadFromFile(skyboxImageFile, fileExt);
}

void Skybox::SetupUniformBuffer() {
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &skyboxUniformBuffer, sizeof(SkyboxUniformBuffer));
}

void Skybox::SetupCubeMapImage() {
	vulkanCubeMap.loadFromTexture(&skyboxCubeMap, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue);

}

void Skybox::SetupDescriptor() {
	VkDescriptorSetLayoutBinding skyboxUniformLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
	VkDescriptorSetLayoutBinding skyboxSamplerLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings = { skyboxUniformLayoutBinding, skyboxSamplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);

	if (vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

	if (vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	skyboxUniformBuffer.setupDescriptor();

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &skyboxUniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &vulkanCubeMap.descriptor, 1));

	vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Skybox::SetupPipeline(VulkanPipeline PipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight) {

	PipelineCreationObject* myPipe = PipelineManager.CreatePipelineOutline();

	PipelineManager.SetVertexShader(myPipe, loadShaderModule(device->device, "shaders/skybox.vert.spv"));
	PipelineManager.SetFragmentShader(myPipe, loadShaderModule(device->device, "shaders/skybox.frag.spv"));
	PipelineManager.SetVertexInput(myPipe, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	PipelineManager.SetInputAssembly(myPipe, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	PipelineManager.SetViewport(myPipe, viewPortWidth, viewPortHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	PipelineManager.SetScissor(myPipe, viewPortWidth, viewPortHeight, 0, 0);
	PipelineManager.SetViewportState(myPipe, 1, 1, 0);
	PipelineManager.SetRasterizer(myPipe, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	PipelineManager.SetMultisampling(myPipe, VK_SAMPLE_COUNT_1_BIT);
	PipelineManager.SetDepthStencil(myPipe, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE);
	PipelineManager.SetColorBlendingAttachment(myPipe, VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	PipelineManager.SetColorBlending(myPipe, 1, &myPipe->colorBlendAttachment);
	PipelineManager.SetDescriptorSetLayout(myPipe, { &descriptorSetLayout }, 1);

	pipelineLayout = PipelineManager.BuildPipelineLayout(myPipe);
	pipeline = PipelineManager.BuildPipeline(myPipe, renderPass, 0);


	PipelineManager.CleanShaderResources(myPipe);
	/*
	VkShaderModule vertShaderModule = loadShaderModule(device->device, "shaders/skybox.vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule(device->device, "shaders/skybox.frag.spv");

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

	VkViewport viewport = initializers::viewport((float)viewPortWidth, (float)viewPortHeight, 0, 1);
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
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	multisampling.sampleShadingEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
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

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

	if (vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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

	if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device->device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device->device, fragShaderModule, nullptr);
	*/
}

void Skybox::UpdateUniform(glm::mat4 proj, glm::mat4 view) {
	SkyboxUniformBuffer sbo = {};
	sbo.proj = proj;
	sbo.view = glm::mat4(glm::mat3(view));

	skyboxUniformBuffer.map(device->device);
	skyboxUniformBuffer.copyTo(&sbo, sizeof(sbo));
	skyboxUniformBuffer.unmap();
};

VkCommandBuffer Skybox::BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, VkCommandBufferInheritanceInfo inheritanceInfo) {

	VkCommandBufferBeginInfo commandBufferBeginInfo = initializers::commandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

	VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffer, &commandBufferBeginInfo));

	
	vkCmdBindDescriptorSets(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(secondaryCommandBuffer, 0, 1, &model.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(secondaryCommandBuffer, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(secondaryCommandBuffer, static_cast<uint32_t>(model.indexCount), 1, 0, 0, 0);

	VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffer));

	return secondaryCommandBuffer;
}


