#include "InstancedSceneObject.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

InstancedSceneObject::InstancedSceneObject()
{
}


InstancedSceneObject::~InstancedSceneObject()
{
}


void InstancedSceneObject::InitInstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->renderer = renderer;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptor(global, lighting);
	SetupPipeline();
}

void InstancedSceneObject::ReinitInstancedSceneObject(std::shared_ptr<VulkanRenderer> renderer){
	this->renderer = renderer;

	vkDestroyPipelineLayout(renderer->device.device, pipelineLayout, nullptr);
	vkDestroyPipeline(renderer->device.device, pipeline, nullptr);
	vkDestroyPipeline(renderer->device.device, wireframe, nullptr);
	vkDestroyPipeline(renderer->device.device, debugNormals, nullptr);

	SetupPipeline();
}

void InstancedSceneObject::CleanUp()
{
	vulkanModel.destroy();
	vulkanTexture.destroy();

	uniformBuffer.cleanBuffer();
	vkDestroyBuffer(renderer->device.device, instanceBuffer.buffer, nullptr);
	vkFreeMemory(renderer->device.device, instanceBuffer.memory, nullptr);

	vkDestroyDescriptorSetLayout(renderer->device.device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(renderer->device.device, descriptorPool, nullptr);

	vkDestroyPipelineLayout(renderer->device.device, pipelineLayout, nullptr);
	vkDestroyPipeline(renderer->device.device, pipeline, nullptr);
	vkDestroyPipeline(renderer->device.device, wireframe, nullptr);
	vkDestroyPipeline(renderer->device.device, debugNormals, nullptr);
}

void InstancedSceneObject::LoadModel(std::string filename) {
	mesh = std::make_shared<Mesh>();
	//this->mesh->importFromFile(filename);
}

void InstancedSceneObject::LoadModel(std::shared_ptr<Mesh> mesh) {
	this->mesh = mesh;
}

void InstancedSceneObject::LoadTexture(std::string filename) {
	texture = std::make_shared<Texture>();
	texture->loadFromFileRGBA(filename);
}

void InstancedSceneObject::SetupUniformBuffer() {
	renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &uniformBuffer, sizeof(ModelBufferObject));

	ModelBufferObject ubo = {};
	ubo.model = glm::mat4();
	ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
	//ubo.model = glm::rotate(ubo.model, time / 2.0f, glm::vec3(0.5, 1, 0));
	ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
		
	VK_CHECK_RESULT(uniformBuffer.map(renderer->device.device));
	uniformBuffer.copyTo(&ubo, sizeof(ModelBufferObject));
	uniformBuffer.unmap();
}

void InstancedSceneObject::SetupImage() {
	vulkanTexture.loadFromTexture(texture, VK_FORMAT_R8G8B8A8_UNORM, std::shared_ptr<VulkanDevice>(&renderer->device), renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0);
}

void InstancedSceneObject::SetupModel() {
	vulkanModel.loadFromMesh(mesh, std::shared_ptr<VulkanDevice>(&renderer->device), renderer->device.graphics_queue);
}

void InstancedSceneObject::SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting) {
	//setup layout
	VkDescriptorSetLayoutBinding cboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
	VkDescriptorSetLayoutBinding uboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	VkDescriptorSetLayoutBinding lboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);
	VkDescriptorSetLayoutBinding samplerLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings = { cboLayoutBinding, uboLayoutBinding, lboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(renderer->device.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	//setup pool
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

	VkDescriptorPoolCreateInfo poolInfo = initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);

	if (vkCreateDescriptorPool(renderer->device.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	//setup descriptor set
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

	if (vkAllocateDescriptorSets(renderer->device.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	uniformBuffer.setupDescriptor();

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &global.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &uniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lighting.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &vulkanTexture.descriptor, 1));

	vkUpdateDescriptorSets(renderer->device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void InstancedSceneObject::SetupPipeline() {

	VulkanPipeline &pipeMan = renderer->pipelineManager;
	std::shared_ptr<PipelineCreationObject> myPipe = pipeMan.CreatePipelineOutline();

	pipeMan.SetVertexShader(myPipe, loadShaderModule(renderer->device.device, "shaders/instancedSceneObject.vert.spv"));
	pipeMan.SetFragmentShader(myPipe, loadShaderModule(renderer->device.device, "shaders/instancedSceneObject.frag.spv"));
	//pipeMan.SetVertexInput(myPipe, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(myPipe, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(myPipe, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(myPipe, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(myPipe, 1, 1, 0);
	pipeMan.SetRasterizer(myPipe, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(myPipe, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(myPipe, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(myPipe, VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	pipeMan.SetColorBlending(myPipe, 1, &myPipe->colorBlendAttachment);
	pipeMan.SetDescriptorSetLayout(myPipe, { &descriptorSetLayout }, 1);



	std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
		// Binding point 0: Mesh vertex layout description at per-vertex rate
		initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
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

	pipeMan.SetVertexInput(myPipe, bindingDescriptions, attributeDescriptions);

	pipelineLayout = pipeMan.BuildPipelineLayout(myPipe);
	pipeline = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);

	pipeMan.SetRasterizer(myPipe, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	wireframe = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(myPipe);
	pipeMan.SetVertexShader(myPipe, loadShaderModule(renderer->device.device, "shaders/normalVecDebug.vert.spv"));
	pipeMan.SetFragmentShader(myPipe, loadShaderModule(renderer->device.device, "shaders/normalVecDebug.frag.spv"));
	pipeMan.SetGeometryShader(myPipe, loadShaderModule(renderer->device.device, "shaders/normalVecDebug.geom.spv"));

	debugNormals = pipeMan.BuildPipeline(myPipe, renderer->renderPass, 0);
	pipeMan.CleanShaderResources(myPipe);
	/*

	VkShaderModule vertShaderModule = loadShaderModule(renderer->device.device, "shaders/gameObject_shader.vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule(renderer->device.device, "shaders/gameObject_shader.frag.spv");

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
	VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
	multisampling.sampleShadingEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
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
	*/
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

	instanceBuffer.size = instancesData.size() * sizeof(InstanceData);

	// Staging
	// Instanced data is static, copy to device local memory 
	// This results in better performance

	struct {
		VkDeviceMemory memory;
		VkBuffer buffer;
	} stagingBuffer;

	VK_CHECK_RESULT(renderer->device.createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		instanceBuffer.size,
		&stagingBuffer.buffer,
		&stagingBuffer.memory,
		instancesData.data()));

	VK_CHECK_RESULT(renderer->device.createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		instanceBuffer.size,
		&instanceBuffer.buffer,
		&instanceBuffer.memory));

	// Copy to staging buffer
	VkCommandBuffer copyCmd = renderer->device.createCommandBuffer(renderer->device.graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion = {};
	copyRegion.size = instanceBuffer.size;
	vkCmdCopyBuffer(
		copyCmd,
		stagingBuffer.buffer,
		instanceBuffer.buffer,
		1,
		&copyRegion);

	renderer->device.flushCommandBuffer(copyCmd, renderer->device.graphics_queue, true);

	instanceBuffer.descriptor.range = instanceBuffer.size;
	instanceBuffer.descriptor.buffer = instanceBuffer.buffer;
	instanceBuffer.descriptor.offset = 0;

	// Destroy staging resources
	vkDestroyBuffer(renderer->device.device, stagingBuffer.buffer, nullptr);
	vkFreeMemory(renderer->device.device, stagingBuffer.memory, nullptr);
}

void InstancedSceneObject::UpdateUniformBuffer()
{
	for (auto it = modelUniforms.begin(); it != modelUniforms.end(); it++) {
		//ModelBufferObject ubo = {};
		//ubo.model = glm::mat4();
		////ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
		//ubo.model = glm::rotate(ubo.model, time / 2.0f, glm::vec3(0.5, 1, 0));
		//ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));
	}

	//modelUniformsBuffer.map(renderer->device.device);
	//modelUniformsBuffer.copyTo(&modelUniforms, modelUniforms.size() * sizeof(ModelBufferObject));
	//modelUniformsBuffer.unmap();
}

void InstancedSceneObject::WriteToCommandBuffer(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	// Binding point 0 : Mesh vertex buffer
	vkCmdBindVertexBuffers(commandBuffer, VERTEX_BUFFER_BIND_ID, 1, &vulkanModel.vertices.buffer, offsets);
	// Binding point 1 : Instance data buffer
	vkCmdBindVertexBuffers(commandBuffer, INSTANCE_BUFFER_BIND_ID, 1, &instanceBuffer.buffer, offsets);

	vkCmdBindIndexBuffer(commandBuffer, vulkanModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vulkanModel.indexCount), 16, 0, 0, 0);

}