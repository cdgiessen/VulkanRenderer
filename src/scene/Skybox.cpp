#include "Skybox.h"

Skybox::Skybox() {

};

Skybox::~Skybox() {

};

void Skybox::CleanUp() {
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

	model.destroy(renderer->device);
	vulkanCubeMap.destroy(renderer->device);

	skyboxUniformBuffer.cleanBuffer();

	vkDestroyDescriptorSetLayout(renderer->device.device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(renderer->device.device, descriptorPool, nullptr);

}

void Skybox::InitSkybox(std::shared_ptr<VulkanRenderer> renderer) {
	this->renderer = renderer;

	SetupUniformBuffer();
	SetupCubeMapImage();
	SetupDescriptor();

	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();
	mvp->ObjectCallBackFunction = std::make_unique<std::function<void(void)>>(std::bind(&Skybox::SetupPipeline, this));

	SetupPipeline();

}

void Skybox::SetupUniformBuffer() {
	renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		(VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
		&skyboxUniformBuffer, sizeof(SkyboxUniformBuffer));
}

void Skybox::SetupCubeMapImage() {
	vulkanCubeMap.loadFromTexture(renderer->device, skyboxCubeMap, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue);

}

void Skybox::SetupDescriptor() {
	VkDescriptorSetLayoutBinding skyboxUniformLayoutBinding 
		= initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
	VkDescriptorSetLayoutBinding skyboxSamplerLayoutBinding 
		= initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings = { skyboxUniformLayoutBinding, skyboxSamplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(renderer->device.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),
		poolSizes.data(), 1);

	if (vkCreateDescriptorPool(renderer->device.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

	if (vkAllocateDescriptorSets(renderer->device.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	skyboxUniformBuffer.setupDescriptor();

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(
		descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &skyboxUniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(
		descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &vulkanCubeMap.descriptor, 1));

	vkUpdateDescriptorSets(renderer->device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Skybox::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;

	pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/skybox.vert.spv"));
	pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/skybox.frag.spv"));
	pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width, (float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 
		VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(mvp, VK_FALSE,  
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending(mvp, 1, &mvp->pco.colorBlendAttachment);
	pipeMan.SetDescriptorSetLayout(mvp, { &descriptorSetLayout }, 1);

	pipeMan.BuildPipelineLayout(mvp);
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(mvp);
	
}

void Skybox::UpdateUniform(glm::mat4 proj, glm::mat4 view) {
	SkyboxUniformBuffer sbo = {};
	sbo.proj = proj;
	sbo.view = glm::mat4(glm::mat3(view));

	skyboxUniformBuffer.map(renderer->device.device);
	skyboxUniformBuffer.copyTo(&sbo, sizeof(sbo));
	skyboxUniformBuffer.unmap();
};

//VkCommandBuffer Skybox::BuildSecondaryCommandBuffer(VkCommandBuffer secondaryCommandBuffer, 
//		VkCommandBufferInheritanceInfo inheritanceInfo) {
//
//	VkCommandBufferBeginInfo commandBufferBeginInfo = initializers::commandBufferBeginInfo();
//	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
//	commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
//
//	VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffer, &commandBufferBeginInfo));
//
//	
//	vkCmdBindDescriptorSets(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 0, 1, &descriptorSet, 0, nullptr);
//	vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(0));
//
//	VkDeviceSize offsets[1] = { 0 };
//	vkCmdBindVertexBuffers(secondaryCommandBuffer, 0, 1, &model.vertices.buffer, offsets);
//	vkCmdBindIndexBuffer(secondaryCommandBuffer, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
//	vkCmdDrawIndexed(secondaryCommandBuffer, static_cast<uint32_t>(model.indexCount), 1, 0, 0, 0);
//
//	VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffer));
//
//	return secondaryCommandBuffer;
//}


