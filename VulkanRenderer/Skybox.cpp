#pragma once

#include "Skybox.h"
#include "Texture.h"
#include "VulkanTexture.hpp"
#include "VulkanModel.hpp"
#include "VulkanDevice.hpp"


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

void Skybox::InitSkybox(VulkanDevice* device, std::string filename, std::string fileExt, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight) {
	this->device = device;

	LoadSkyboxData(filename, fileExt);


	SetupUniformBuffer();
	SetupCubeMapImage();
	SetupDescriptor();
	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);

}

void Skybox::ReinitSkybox(VulkanDevice* device, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight){
	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
	
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);

	SetupDescriptor();
	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);
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

void Skybox::SetupPipeline(VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight) {

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
}

void Skybox::UpdateUniform(glm::mat4 proj, glm::mat4 view) {
	SkyboxUniformBuffer sbo = {};
	sbo.proj = proj;
	sbo.view = glm::mat4(glm::mat3(view));

	skyboxUniformBuffer.map(device->device);
	skyboxUniformBuffer.copyTo(&sbo, sizeof(sbo));
	skyboxUniformBuffer.unmap();
};


