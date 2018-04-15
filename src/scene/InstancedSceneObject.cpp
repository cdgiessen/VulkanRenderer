#include "InstancedSceneObject.h"

#include "../core/Logger.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1


InstancedSceneObject::InstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer, int maxInstances) : maxInstanceCount(maxInstances)
{
	vulkanModel = std::make_shared<VulkanModel>(renderer->device);
	vulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);

	uniformBuffer = std::make_shared<VulkanBufferUniform>(renderer->device);
	instanceBuffer = std::make_shared<VulkanBufferVertex>(renderer->device);
}


InstancedSceneObject::~InstancedSceneObject()
{

}


void InstancedSceneObject::InitInstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer)
{
	this->renderer = renderer;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptor();
	SetupPipeline();
}

void InstancedSceneObject::CleanUp()
{
	renderer->pipelineManager.DeleteManagedPipeline(mvp);

	vulkanModel->destroy();
	vulkanTexture->destroy();

	uniformBuffer->CleanBuffer();
	instanceBuffer->CleanBuffer();
	//vkDestroyBuffer(renderer->device.device, instanceBuffer.buffer, nullptr);
	//vkFreeMemory(renderer->device.device, instanceBuffer.memory, nullptr);
}

void InstancedSceneObject::LoadModel(std::string filename) {
	mesh = std::make_shared<Mesh>();
	//this->mesh->importFromFile(filename);
}

void InstancedSceneObject::LoadModel(std::shared_ptr<Mesh> mesh) {
	this->mesh = mesh;
}

void InstancedSceneObject::LoadTexture(std::shared_ptr<Texture> tex) {
	this->texture = tex; 
}

void InstancedSceneObject::SetFragmentShaderToUse(VkShaderModule shaderModule) {
	fragShaderModule = shaderModule;
}

void InstancedSceneObject::SetCullMode(VkCullModeFlagBits cullMode){
	cullModeFlagBits = cullMode;
}

void InstancedSceneObject::SetBlendMode(VkBool32 blendEnable){
	enableBlending = blendEnable;
}

void InstancedSceneObject::SetupUniformBuffer() {
	uniformBuffer->CreateUniformBuffer(sizeof(ModelBufferObject));

	ModelBufferObject ubo = {};
	ubo.model = glm::mat4();
	ubo.model = glm::translate(ubo.model, glm::vec3(0, 0, 0));
	//ubo.model = glm::rotate(ubo.model, time / 2.0f, glm::vec3(0.5, 1, 0));
	ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
		
	uniformBuffer->CopyToBuffer(&ubo, sizeof(ModelBufferObject));

	//VK_CHECK_RESULT(uniformBuffer.map(renderer->device.device));
	//uniformBuffer.copyTo(&ubo, sizeof(ModelBufferObject));
	//uniformBuffer.unmap();

	instanceBuffer->CreateVertexBuffer(sizeof(InstanceData) * maxInstanceCount);

}

void InstancedSceneObject::SetupImage() {
	vulkanTexture->loadFromTexture(texture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.GetTransferCommandBuffer(), VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0);
}

void InstancedSceneObject::SetupModel() {
	vulkanModel->loadFromMesh(mesh, renderer->device.GetTransferCommandBuffer());
}

void InstancedSceneObject::SetupDescriptor() {
	descriptor = renderer->GetVulkanDescriptor();

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
	writes.push_back(DescriptorUse(0, 1, uniformBuffer->resource));
	writes.push_back(DescriptorUse(1, 1, vulkanTexture->resource));
	descriptor->UpdateDescriptorSet(m_descriptorSet, writes);

}

void InstancedSceneObject::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/instancedSceneObject.vert.spv"));
	pipeMan.SetFragmentShader(mvp, fragShaderModule);
	//pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width, (float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, cullModeFlagBits,
		VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(mvp, enableBlending, 
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
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



	std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
		// Binding point 0: Mesh vertex layout description at per-vertex rate
		initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex_PosNormTexColor), VK_VERTEX_INPUT_RATE_VERTEX),
		// Binding point 1: Instanced data at per-instance rate
		initializers::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
	};

	// Vertex attribute bindings
	// Note that the shader declaration for per-vertex and per-instance attributes is the same, the different input rates are only stored in the bindings:
	// instanced.vert:
	//	layout (location = 0) in vec3 inPos;			Per-Vertex
	//	...
	//	layout (location = 4) in vec3 instancePos;	Per-Instance
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
		// Per-vertex attributees
		// These are advanced for each vertex fetched by the vertex shader
		initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),					// Location 0: Position			
		initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),	// Location 1: Normal			
		initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6),		// Location 2: Texture coordinates			
		initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),	// Location 3: Color

		// Per-Instance attributes
		// These are fetched for each instance rendered
		initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),	// Location 4: Position
		initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, 0),					// Location 5: Rotation
		initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SFLOAT,sizeof(float) * 6),			// Location 6: Scale
		initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 7, VK_FORMAT_R32_SINT, sizeof(float) * 7),			// Location 7: Texture array layer index
	};

	pipeMan.SetVertexInput(mvp, bindingDescriptions, attributeDescriptions);

	pipeMan.BuildPipelineLayout(mvp);
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(mvp);
	//pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.vert.spv"));
	//pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.frag.spv"));
	//pipeMan.SetGeometryShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.geom.spv"));
	//
	//pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);
	//pipeMan.CleanShaderResources(mvp);
	
}

void InstancedSceneObject::AddInstance(InstanceData data) {

	instancesData.push_back(data);

}

void InstancedSceneObject::UploadInstances() {
	size_t instanceBufferSize = instancesData.size() * sizeof(InstanceData);

	VulkanBufferUniform stagingBuffer(renderer->device);
	stagingBuffer.CreateStagingUniformBuffer(instancesData.data(), instanceBufferSize);

	auto copyCmd = renderer->device.GetTransferCommandBuffer();

	VkBufferCopy copyRegion = {};
	copyRegion.size = instanceBufferSize;
	vkCmdCopyBuffer(
		copyCmd,
		stagingBuffer.buffer.buffer,
		instanceBuffer->buffer.buffer,
		1,
		&copyRegion);

	renderer->device.SubmitTransferCommandBufferAndWait(copyCmd);

	instanceBuffer->resource.FillResource(instanceBuffer->buffer.buffer, 0, instanceBufferSize);

	// Destroy staging resources
	stagingBuffer.CleanBuffer();
}

void InstancedSceneObject::AddInstances(std::vector<glm::vec3> positions) {
	//for (auto it = positions.begin(); it != positions.end(); it++) {
	//	ModelBufferObject ubo = {};
	//	ubo.model = glm::translate(ubo.model, *it);
	//	ubo.normal = glm::mat4();
	//	modelUniforms.push_back(ubo);
	//}

	//modelUniformsBuffer.map(renderer->device.device);
	//modelUniformsBuffer.copyTo(&modelUniforms, modelUniforms.size() * sizeof(ModelBufferObject));
	//modelUniformsBuffer.unmap();

	for (int i = 0; i < positions.size(); i++)
	{
		InstanceData id;
		id.pos = positions[i];
		id.rot = glm::vec3(0,0,0);
		id.scale = 1.0f;
		instancesData.push_back(id);
	}

	UploadInstances();
}

void InstancedSceneObject::UpdateUniformBuffer()
{
	//for (auto it = modelUniforms.begin(); it != modelUniforms.end(); it++) {
		//ModelBufferObject ubo = {};
		//ubo.model = glm::mat4();
		////ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
		//ubo.model = glm::rotate(ubo.model, time / 2.0f, glm::vec3(0.5, 1, 0));
		//ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
	//}

	//modelUniformsBuffer.map(renderer->device.device);
	//modelUniformsBuffer.copyTo(&modelUniforms, modelUniforms.size() * sizeof(ModelBufferObject));
	//modelUniformsBuffer.unmap();
}

void InstancedSceneObject::WriteToCommandBuffer(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &m_descriptorSet.set, 0, nullptr);

	
	//vulkanModel.BindModel(commandBuffer);
	// Binding point 0 : Mesh vertex buffer
	vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, &vulkanModel->vmaVertices.buffer.buffer, offsets);
	// Binding point 1 : Instance data buffer
	vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, &instanceBuffer->buffer.buffer, offsets);

	vkCmdBindIndexBuffer(commandBuffer, vulkanModel->vmaIndicies.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanModel->indexCount), 16, 0, 0, 0);

}