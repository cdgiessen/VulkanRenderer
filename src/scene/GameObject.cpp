#include "GameObject.h"

#include "../core/Logger.h"

GameObject::GameObject() {}

GameObject::~GameObject() { Log::Debug << "game object deleted\n"; }

void GameObject::InitGameObject(std::shared_ptr<VulkanRenderer> renderer)
{
    this->renderer = renderer;

    //SetupUniformBuffer();
    SetupImage();
    SetupModel();
    SetupDescriptor();
    SetupPipeline();
}

void GameObject::CleanUp()
{
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

    gameObjectModel->destroy();
    gameObjectVulkanTexture.destroy(renderer->device);

    //modelUniformBuffer.CleanBuffer(renderer->device);
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

//void GameObject::SetupUniformBuffer()
//{
//	modelUniformBuffer.CreateUniformBuffer(renderer->device, sizeof(ModelBufferObject));
//    //renderer->device.createBuffer(
//    //    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//    //    (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
//    //    &modelUniformBuffer, sizeof(ModelBufferObject));
//}

void GameObject::SetupImage()
{

    gameObjectVulkanTexture.loadFromTexture(
        renderer->device, gameObjectTexture, VK_FORMAT_R8G8B8A8_UNORM,
        renderer->device.GetTransferCommandBuffer(),
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0);
}

void GameObject::SetupModel()
{
    gameObjectModel->loadFromMesh(gameObjectMesh, renderer->device.GetTransferCommandBuffer());

}

void GameObject::SetupDescriptor()
{
	descriptor = renderer->GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool(poolSizes);

	m_descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(3, 1, gameObjectVulkanTexture.resource));
	descriptor->UpdateDescriptorSet(m_descriptorSet, writes);
}

void GameObject::SetupPipeline()
{

    VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

    pipeMan.SetVertexShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/gameObject_shader.vert.spv"));
    pipeMan.SetFragmentShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/gameObject_shader.frag.spv"));
    pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(),
                           Vertex::getAttributeDescriptions());
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
	layouts.push_back(descriptor->GetLayout());
    pipeMan.SetDescriptorSetLayout(mvp, layouts);
	
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(ModelPushConstant);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	pipeMan.SetModelPushConstant(mvp, pushConstantRange);

    pipeMan.BuildPipelineLayout(mvp);
    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

    pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE,
                          VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE,
                          1.0f, VK_TRUE);
    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

    pipeMan.CleanShaderResources(mvp);
    pipeMan.SetVertexShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.vert.spv"));
    pipeMan.SetFragmentShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.frag.spv"));
    pipeMan.SetGeometryShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.geom.spv"));

    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);
	    pipeMan.CleanShaderResources(mvp);
}

void GameObject::UpdateUniformBuffer(float time)
{
	modelPushConstant = {};
	modelPushConstant.model = glm::mat4();
    // ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
	modelPushConstant.model = glm::rotate(modelPushConstant.model, time / 2.0f, glm::vec3(0.5, 1, 0));
	modelPushConstant.normal = glm::transpose(glm::inverse(glm::mat3(modelPushConstant.model)));



	//modelUniformBuffer.CopyToBuffer(renderer->device, &ubo, sizeof(ModelBufferObject));
    //modelUniformBuffer.Map(renderer->device, );
    //modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
    //modelUniformBuffer.Unmap();
}

void GameObject::Draw(VkCommandBuffer commandBuffer, bool wireframe, bool drawNormals) {

	VkDeviceSize offsets[] = { 0 };

	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vertices.buffer, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel.indexCount), 1, 0, 0, 0);

	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gameObjectModel.vmaBufferVertex, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, gameObjectModel.vmaIndicies.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	
	vkCmdPushConstants(
		commandBuffer,
		mvp->layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ModelPushConstant),
		&modelPushConstant);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &m_descriptorSet.set, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	gameObjectModel->BindModel(commandBuffer);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel->indexCount), 1, 0, 0, 0);


	if (drawNormals) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(2));
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gameObjectModel->indexCount), 1, 0, 0, 0);
	}


}