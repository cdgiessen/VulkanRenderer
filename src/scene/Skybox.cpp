#include "Skybox.h"

Skybox::Skybox(VulkanRenderer& renderer) :renderer(renderer) {

};

Skybox::~Skybox() {
	renderer.pipelineManager.DeleteManagedPipeline(mvp);
};

void Skybox::InitSkybox() {

	SetupUniformBuffer();
	SetupCubeMapImage();
	SetupDescriptor();
	SetupPipeline();

}

void Skybox::SetupUniformBuffer() {
	skyboxUniformBuffer = std::make_shared<VulkanBufferUniform>(renderer.device, sizeof(SkyboxUniformBuffer));
	//skyboxUniformBuffer->CreateUniformBuffer(sizeof(SkyboxUniformBuffer));
}

void Skybox::SetupCubeMapImage() {
	TexCreateDetails details(VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		true, 4);
	vulkanCubeMap = renderer.textureManager.CreateCubeMap(skyboxCubeMap, details);
}

void Skybox::SetupDescriptor() {
	descriptor = renderer.GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool(poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(0, 1, skyboxUniformBuffer->resource));
	writes.push_back(DescriptorUse(1, 1, vulkanCubeMap->resource));
	descriptor->UpdateDescriptorSet(m_descriptorSet, writes);
}

void Skybox::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer.pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	auto vert = renderer.shaderManager.loadShaderModule("assets/shaders/skybox.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule("assets/shaders/skybox.frag.spv", ShaderModuleType::fragment);

	ShaderModuleSet set(vert, frag, {}, {}, {});
	pipeMan.SetShaderModuleSet(mvp, set);
	//pipeMan.SetVertexShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/skybox.vert.spv"));
	//pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer.device.device, "assets/shaders/skybox.frag.spv"));

	pipeMan.SetVertexInput(mvp, Vertex_PosNormTexColor::getBindingDescription(), Vertex_PosNormTexColor::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer.vulkanSwapChain.swapChainExtent.width, (float)renderer.vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer.vulkanSwapChain.swapChainExtent.width, renderer.vulkanSwapChain.swapChainExtent.height, 0, 0);
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

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState(mvp, dynamicStateEnables);

	std::vector<VkDescriptorSetLayout> layouts;
	renderer.AddGlobalLayouts(layouts);
	layouts.push_back(descriptor->GetLayout());
	pipeMan.SetDescriptorSetLayout(mvp, layouts);

	pipeMan.BuildPipelineLayout(mvp);
	pipeMan.BuildPipeline(mvp, renderer.renderPass->Get(), 0);

	//pipeMan.CleanShaderResources(mvp);

}

void Skybox::UpdateUniform(glm::mat4 proj, glm::mat4 view) {
	SkyboxUniformBuffer sbo = {};
	sbo.proj = proj;
	sbo.view = glm::mat4(glm::mat3(view));

	skyboxUniformBuffer->CopyToBuffer(&sbo, sizeof(SkyboxUniformBuffer));
	/*
	skyboxUniformBuffer.map(renderer.device.device);
	skyboxUniformBuffer.copyTo(&sbo, sizeof(sbo));
	skyboxUniformBuffer.unmap();*/
};


void Skybox::WriteToCommandBuffer(VkCommandBuffer commandBuffer) {
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &m_descriptorSet.set, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(0));

	model->BindModel(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->indexCount), 1, 0, 0, 0);
}

