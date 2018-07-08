#include "GameObject.h"

#include "../core/Logger.h"

GameObject::GameObject() {}

GameObject::~GameObject() {
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

	Log::Debug << "game object deleted\n";
}

void GameObject::InitGameObject(VulkanRenderer* renderer)
{
	this->renderer = renderer;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupMaterial();
	SetupPipeline();
}

void GameObject::LoadModel(std::string filename)
{
	gameObjectMesh = std::make_shared<Mesh>();
	// this->gameObjectMesh->importFromFile(filename);
}

void GameObject::LoadModel(std::shared_ptr<Mesh> mesh)
{
	this->gameObjectMesh = mesh;
}

void GameObject::SetupUniformBuffer()
{
	uniformBuffer = std::make_shared<VulkanBufferUniform>(renderer->device, sizeof(ModelBufferObject));
	//uniformBuffer->CreateUniformBufferPersitantlyMapped(sizeof(ModelBufferObject));

	materialBuffer = std::make_shared<VulkanBufferUniform>(renderer->device, sizeof(Phong_Material));
	//	if (usePBR)
	//		materialBuffer->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Material));
	//	else
	//		materialBuffer->CreateUniformBufferPersitantlyMapped(sizeof(Phong_Material));
	//PBR_Material pbr;
	//pbr.albedo = glm::vec3(0, 0, 1);
	//materialBuffer->CopyToBuffer(&pbr, sizeof(PBR_Material));
}

void GameObject::SetupImage()
{

	gameObjectVulkanTexture = std::make_unique<VulkanTexture2D>(renderer->device,
		gameObjectTexture, VK_FORMAT_R8G8B8A8_UNORM,
		*renderer,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0);
}

void GameObject::SetupModel()
{
	gameObjectModel->loadFromMesh(gameObjectMesh, *renderer);

}

void GameObject::SetupMaterial() {
	mat = std::make_shared<VulkanMaterial>(renderer->device);

	mat->AddMaterialDataSlot({ ResourceType::uniform,
		ResourceStages::fragment_only, materialBuffer->resource });

	mat->AddMaterialDataSlot({ ResourceType::texture2D,
		ResourceStages::fragment_only, gameObjectVulkanTexture->resource });

	//mat->AddTexture(gameObjectVulkanTexture);
	mat->Setup();
}

void GameObject::SetupDescriptor()
{

	// descriptor = renderer->GetVulkanDescriptor();

	// std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	// m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	//m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	// m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));

	// descriptor->SetupLayout(m_bindings);

	// std::vector<DescriptorPoolSize> poolSizes;
	// poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	// poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	// //poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	// descriptor->SetupPool(poolSizes);

	// m_descriptorSet = descriptor->CreateDescriptorSet();

	// std::vector<DescriptorUse> writes;
	// writes.push_back(DescriptorUse(0, 1, uniformBuffer->resource));
	// writes.push_back(DescriptorUse(1, 1, materialBuffer->resource));
	// //writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	// descriptor->UpdateDescriptorSet(m_descriptorSet, writes);

	//materialDescriptor = renderer->GetVulkanDescriptor();
	//
	//std::vector<VkDescriptorSetLayoutBinding> mat_bindings;
	//mat_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1));
	//mat_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	//
	//materialDescriptor->SetupLayout(mat_bindings);
	//
	//std::vector<DescriptorPoolSize> mat_poolSizes;
	//mat_poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	////mat_poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	//materialDescriptor->SetupPool(mat_poolSizes);
	//
	//material_descriptorSet = materialDescriptor->CreateDescriptorSet();
	//
	//std::vector<DescriptorUse> mat_writes;
	//mat_writes.push_back(DescriptorUse(0, 1, materialBuffer->resource));
	////mat_writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	//materialDescriptor->UpdateDescriptorSet(material_descriptorSet, writes);

}

void GameObject::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	auto pbr_vert = renderer->shaderManager.loadShaderModule("assets/shaders/pbr.vert.spv", ShaderModuleType::vertex);
	auto pbr_frag = renderer->shaderManager.loadShaderModule("assets/shaders/pbr.frag.spv", ShaderModuleType::fragment);

	auto go_vert = renderer->shaderManager.loadShaderModule("assets/shaders/gameObject_shader.vert.spv", ShaderModuleType::vertex);
	auto go_frag = renderer->shaderManager.loadShaderModule("assets/shaders/gameObject_shader.frag.spv", ShaderModuleType::fragment);

	auto geom = renderer->shaderManager.loadShaderModule("assets/shaders/normalVecDebug.geom.spv", ShaderModuleType::geometry);

	ShaderModuleSet pbr_set(pbr_vert, pbr_frag, {}, {}, {});
	ShaderModuleSet go_set(go_vert, go_frag, {}, {}, {});

	if (usePBR)
		pipeMan.SetShaderModuleSet(mvp, pbr_set);
	else
		pipeMan.SetShaderModuleSet(mvp, go_set);

	pipeMan.SetVertexInput(mvp, Vertex_PosNormTex::getBindingDescription(),
		Vertex_PosNormTex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
		VK_FALSE);

	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width,
		(float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f,
		1.0f, 0.0f, 0.0f);

	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width,
		renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);

	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE,
		1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER,
		VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(
		mvp, VK_FALSE,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending(mvp, 1, &mvp->pco.colorBlendAttachment);

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	pipeMan.SetDynamicState(mvp, dynamicStateEnables);


	std::vector<VkDescriptorSetLayout> layouts;
	renderer->AddGlobalLayouts(layouts);
	layouts.push_back(mat->GetDescriptorSetLayout());
	//layouts.push_back(materialDescriptor->GetLayout());
	pipeMan.SetDescriptorSetLayout(mvp, layouts);

	//VkPushConstantRange pushConstantRange = {};
	//pushConstantRange.offset = 0;
	//pushConstantRange.size = sizeof(ModelPushConstant);
	//pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//
	//pipeMan.SetModelPushConstant(mvp, pushConstantRange);

	pipeMan.BuildPipelineLayout(mvp);
	pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);

	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE,
		1.0f, VK_TRUE);
	pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);

	ShaderModuleSet pbr_geom_set(pbr_vert, pbr_frag, geom, {}, {});
	ShaderModuleSet go_geom_set(go_vert, go_frag, geom, {}, {});

	if (usePBR)
		pipeMan.SetShaderModuleSet(mvp, pbr_geom_set);
	else
		pipeMan.SetShaderModuleSet(mvp, go_geom_set);

	pipeMan.BuildPipeline(mvp, renderer->renderPass->Get(), 0);
	// pipeMan.CleanShaderResources(mvp);
}

void GameObject::UpdateUniformBuffer(float time)
{
	//modelPushConstant = {};
	//modelPushConstant.model = glm::mat4();
 //   // ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
	//modelPushConstant.model = glm::rotate(modelPushConstant.model, time / 2.0f, glm::vec3(0.5, 1, 0));
	//modelPushConstant.normal = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.model)));

	ModelBufferObject ubo = {};
	ubo.model = glm::mat4();
	ubo.model = glm::translate(ubo.model, position);
	//ubo.model = glm::rotate(ubo.model, time * 2.0f, glm::vec3(0.5, 1, 0));
	//ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
	ubo.normal = glm::mat4();
	uniformBuffer->CopyToBuffer(&ubo, sizeof(ModelBufferObject));
	//if (usePBR_Tex)
	//
	//	if (usePBR)
	//		materialBuffer->CopyToBuffer(&pbr_mat, sizeof(PBR_Material));
	//	else
	//		materialBuffer->CopyToBuffer(&phong_mat, sizeof(Phong_Material));

	//modelUniformBuffer.CopyToBuffer(renderer->device, &ubo, sizeof(ModelBufferObject));
	//modelUniformBuffer.Map(renderer->device, );
	//modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
	//modelUniformBuffer.Unmap();

}

void GameObject::Draw(VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals) {

	if (*gameObjectModel->readyToUse)// && *gameObjectVulkanTexture->readyToUse)
		isReadyToRender = true;

	if (!isReadyToRender) {
		return;
	}

	VkDeviceSize offsets[] = { 0 };

	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vertices.buffer, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel.indexCount), 1, 0, 0, 0);

	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vmaBufferVertex, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.vmaIndicies.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	/*vkCmdPushConstants(
		commandBuffer,
		mvp->layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ModelPushConstant),
		&modelPushConstant);*/

	mat->Bind(commandBuffer, mvp->layout);

	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &m_descriptorSet.set, 0, nullptr);
	//vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 3, 1, &material_descriptorSet.set, 0, nullptr);

	mvp->BindPipelineOptionalWireframe(commandBuffer, wireframe);
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	gameObjectModel->BindModel(commandBuffer);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel->indexCount), 1, 0, 0, 0);


	if (drawNormals) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(2));
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel->indexCount), 1, 0, 0, 0);
	}

}