#include "GameObject.h"

#include "core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

GameObject::GameObject (VulkanRenderer& renderer) : renderer (renderer) {}

void GameObject::InitGameObject ()
{
	SetupUniformBuffer ();
	SetupImage ();
	SetupModel ();
	SetupMaterial ();
	SetupPipeline ();
}

// void GameObject::LoadModel (std::string filename)
//{
//
//	//gameObjectMesh = std::make_shared<MeshData> ();
//	// this->gameObjectMesh->importFromFile(filename);
//}

void GameObject::LoadModel (std::shared_ptr<MeshData> mesh) { this->gameObjectMesh = mesh; }

void GameObject::SetupUniformBuffer ()
{
	uniformBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (ModelBufferObject));
	// uniformBuffer->CreateUniformBufferPersitantlyMapped(sizeof(ModelBufferObject));

	materialBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (Phong_Material));
	//	if (usePBR)
	//		materialBuffer->CreateUniformBufferPersitantlyMapped(sizeof(PBR_Material));
	//	else
	//		materialBuffer->CreateUniformBufferPersitantlyMapped(sizeof(Phong_Material));
	// PBR_Material pbr;
	// pbr.albedo = glm::vec3(0, 0, 1);
	// materialBuffer->CopyToBuffer(&pbr, sizeof(PBR_Material));
}

void GameObject::SetupImage ()
{
	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);
	gameObjectVulkanTexture = renderer.textureManager.CreateTexture2D (gameObjectTexture, details);
}

void GameObject::SetupModel ()
{
	gameObjectModel = std::make_shared<VulkanModel> (renderer, gameObjectMesh);
}

void GameObject::SetupMaterial ()
{
	mat = std::make_shared<VulkanMaterial> (renderer.device);

	mat->AddMaterialDataSlot ({ ResourceType::uniform, ResourceStages::fragment_only, materialBuffer->resource });

	mat->AddMaterialDataSlot (
	    { ResourceType::texture2D, ResourceStages::fragment_only, gameObjectVulkanTexture->resource });

	// mat->AddTexture(gameObjectVulkanTexture);
	mat->Setup ();
}

void GameObject::SetupDescriptor ()
{

	// descriptor = renderer.GetVulkanDescriptor();

	// std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	// m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	// VK_SHADER_STAGE_VERTEX_BIT, 0, 1)); m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	// VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1)); m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	// VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));

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

	// materialDescriptor = renderer.GetVulkanDescriptor();
	//
	// std::vector<VkDescriptorSetLayoutBinding> mat_bindings;
	// mat_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	// VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1)); mat_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	// VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	//
	// materialDescriptor->SetupLayout(mat_bindings);
	//
	// std::vector<DescriptorPoolSize> mat_poolSizes;
	// mat_poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	////mat_poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	// materialDescriptor->SetupPool(mat_poolSizes);
	//
	// material_descriptorSet = materialDescriptor->CreateDescriptorSet();
	//
	// std::vector<DescriptorUse> mat_writes;
	// mat_writes.push_back(DescriptorUse(0, 1, materialBuffer->resource));
	////mat_writes.push_back(DescriptorUse(1, 1, gameObjectVulkanTexture->resource));
	// materialDescriptor->UpdateDescriptorSet(material_descriptorSet, writes);
}

void GameObject::SetupPipeline ()
{
	PipelineOutline out;

	auto pbr_vert =
	    renderer.shaderManager.loadShaderModule ("assets/shaders/pbr.vert.spv", ShaderModuleType::vertex);
	auto pbr_frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/pbr.frag.spv", ShaderModuleType::fragment);

	ShaderModuleSet shader_set;
	shader_set.Vertex (pbr_vert).Fragment (pbr_frag);
	out.SetShaderModuleSet (shader_set);

	out.UseModelVertexLayout (*gameObjectModel.get ());

	out.SetInputAssembly (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

	out.AddViewport (1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	out.AddScissor (1, 1, 0, 0);

	out.SetRasterizer (
	    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	out.SetMultisampling (VK_SAMPLE_COUNT_1_BIT);
	out.SetDepthStencil (VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	out.AddColorBlendingAttachment (VK_FALSE,
	    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_ZERO);

	out.AddDescriptorLayouts (renderer.GetGlobalLayouts ());
	out.AddDescriptorLayout (mat->GetDescriptorSetLayout ());

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	normal = std::make_unique<Pipeline> (renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));

	out.SetRasterizer (
	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	wireframe = std::make_unique<Pipeline> (
	    renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));
}

void GameObject::UpdateUniformBuffer (float time)
{
	// modelPushConstant = {};
	// modelPushConstant.model = glm::mat4();
	//   // ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
	// modelPushConstant.model = glm::rotate(modelPushConstant.model, time / 2.0f, glm::vec3(0.5, 1, 0));
	// modelPushConstant.normal = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.model)));

	ModelBufferObject ubo = {};
	ubo.model = glm::mat4 (1.0f);
	ubo.model = glm::translate (ubo.model, position);
	// ubo.model = glm::rotate(ubo.model, time * 2.0f, glm::vec3(0.5, 1, 0));
	// ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
	ubo.normal = glm::mat4 (1.0f);
	uniformBuffer->CopyToBuffer (&ubo, sizeof (ModelBufferObject));
	// if (usePBR_Tex)
	//
	//	if (usePBR)
	//		materialBuffer->CopyToBuffer(&pbr_mat, sizeof(PBR_Material));
	//	else
	//		materialBuffer->CopyToBuffer(&phong_mat, sizeof(Phong_Material));

	// modelUniformBuffer.CopyToBuffer(renderer.device, &ubo, sizeof(ModelBufferObject));
	// modelUniformBuffer.Map(renderer.device, );
	// modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
	// modelUniformBuffer.Unmap();
}
void GameObject::DrawDepthPrePass (VkCommandBuffer commandBuffer)
{
	if (*gameObjectModel->readyToUse) // && *gameObjectVulkanTexture->readyToUse)
		isReadyToRender = true;

	if (!isReadyToRender)
	{
		return;
	}
	gameObjectModel->BindModel (commandBuffer);

	vkCmdDrawIndexed (commandBuffer, static_cast<uint32_t> (gameObjectModel->indexCount), 1, 0, 0, 0);
}


void GameObject::Draw (VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals)
{
	if (*gameObjectModel->readyToUse) // && *gameObjectVulkanTexture->readyToUse)
		isReadyToRender = true;

	if (!isReadyToRender)
	{
		return;
	}

	// VkDeviceSize offsets[] = { 0 };

	// vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vertices.buffer, offsets);
	// vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	// vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel.indexCount), 1, 0, 0, 0);

	// vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vmaBufferVertex, offsets);
	// vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.vmaIndicies.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	/*vkCmdPushConstants(
	    commandBuffer,
	    mvp->layout,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0,
	    sizeof(ModelPushConstant),
	    &modelPushConstant);*/


	// vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1,
	// &m_descriptorSet.set, 0, nullptr); vkCmdBindDescriptorSets(commandBuffer,
	// VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 3, 1, &material_descriptorSet.set, 0, nullptr);

	// mvp->BindPipelineOptionalWireframe (commandBuffer, wireframe);
	// vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));

	if (wireframe)
		this->wireframe->Bind (commandBuffer);
	else
		normal->Bind (commandBuffer);

	mat->Bind (commandBuffer, normal->GetLayout ());
	gameObjectModel->BindModel (commandBuffer);

	vkCmdDrawIndexed (commandBuffer, static_cast<uint32_t> (gameObjectModel->indexCount), 1, 0, 0, 0);


	// if (drawNormals)
	// {
	// 	vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at (2));
	// 	vkCmdDrawIndexed (commandBuffer, static_cast<uint32_t> (gameObjectModel->indexCount), 1, 0, 0, 0);
	// }
}