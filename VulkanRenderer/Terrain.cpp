#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>



TerrainQuad::TerrainQuad() {
	pos = glm::vec3(0);
	size = glm::vec3(0);
	level = 0;
	isSubdivided = false;
	vertsPtr = nullptr;
	indicesPtr = nullptr;

}

TerrainQuad::~TerrainQuad() {}

void TerrainQuad::init(float posX, float posY, int sizeX, int sizeY, int level, uint32_t offset, TerrainMeshVertices* vertsPtr, TerrainMeshIndices* indicesPtr, VkDescriptorSet set) {
	pos = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);
	this->level = level;
	isSubdivided = false;
	modelOffset = offset;
	uniformOffset = offset;
	modelUniformObject.model = glm::translate(glm::mat4(), pos);
	modelUniformObject.normal = glm::mat4();// glm::transpose(glm::inverse(glm::mat3(modelUniformObject.model))));
	this->descriptorSet = set;

	this->vertsPtr = vertsPtr;
	this->indicesPtr = indicesPtr;

	CreateTerrainMesh(vertsPtr, indicesPtr);
}

void TerrainQuad::CleanUp() {
	
}

void TerrainQuad::UpdateTerrainQuad() {

}

void TerrainQuad::UpdateModelUniformBuffer(VulkanDevice* device, VulkanBuffer buff) {

	buff.map(device->device, sizeof(modelUniformObject), uniformOffset * sizeof(ModelBufferObject));
	buff.copyTo(&modelUniformObject, sizeof(modelUniformObject));
	buff.unmap();
}

void TerrainQuad::CreateTerrainMesh(TerrainMeshVertices* verts, TerrainMeshIndices* indices) {

	//uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
	//uint32_t indexCount = static_cast<uint32_t>(indices.size());

	Mesh* mesh = generateTerrainMesh(NumCells, pos.x , pos.y, size.x, size.z);
	//memccpy(verts->data.data, mesh->vertices.data, vertCount, sizeof(Vertex));
	///*
	for (int i = 0; i < (int)vertCount; i++)
	{
		verts->data[i*vertElementCount] =	mesh->vertices[i].pos[0];
		verts->data[i*vertElementCount + 1] = mesh->vertices[i].pos[1];
		verts->data[i*vertElementCount + 2] = mesh->vertices[i].pos[2];
		verts->data[i*vertElementCount + 3] = mesh->vertices[i].normal[0];
		verts->data[i*vertElementCount + 4] = mesh->vertices[i].normal[1];
		verts->data[i*vertElementCount + 5] = mesh->vertices[i].normal[2];
		verts->data[i*vertElementCount + 6] = mesh->vertices[i].texCoord[0];
		verts->data[i*vertElementCount + 7] = mesh->vertices[i].texCoord[1];
		verts->data[i*vertElementCount + 8] = mesh->vertices[i].color[0];
		verts->data[i*vertElementCount + 9] = mesh->vertices[i].color[1];
		verts->data[i*vertElementCount + 10] = mesh->vertices[i].color[2];
		verts->data[i*vertElementCount + 11] = mesh->vertices[i].color[3];
	}
	//*/

	//memcpy(indices->data.data, mesh->indices.data, indexCount * sizeof(float));
	///*
	for (int i = 0; i < (int)indCount; i++)
	{
		indices->data[i] = mesh->indices[i];
	}
	//*/



	
}










Terrain::Terrain(int numCells, int maxLevels, float posX, float posY, int sizeX, int sizeY) : maxLevels(maxLevels)
{
	if (maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = (1 - glm::pow(4, maxLevels + 1)) / (-3);
	}
	terrainQuadPool = new MemoryPool<TerrainQuad>(maxNumQuads);
	meshVertexPool = new MemoryPool<TerrainMeshVertices>(maxNumQuads);
	meshIndexPool = new MemoryPool<TerrainMeshIndices>(maxNumQuads);

	descriptorSets.resize(maxNumQuads);

	position = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());
}


Terrain::~Terrain() {
	terrainSplatMap->~Texture();
	terrainTextureArray->~TextureArray();

	terrainQuadPool->~MemoryPool();
	meshVertexPool->~MemoryPool();
	meshIndexPool->~MemoryPool();
}


void Terrain::InitTerrain(VulkanDevice* device, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->device = device;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptorSets();
	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);
	UploadMeshBuffer(copyQueue);

	rootQuad = terrainQuadPool->allocate();
	rootQuad->init(position.x, position.z, size.x, size.z, 0, 0, meshVertexPool->allocate(), meshIndexPool->allocate(), descriptorSets[0]);
	rootQuad->UpdateModelUniformBuffer(device, modelUniformBuffer);
	SetupQuadDescriptor(rootQuad, global, lighting);


}

void Terrain::ReinitTerrain(VulkanDevice* device, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->device = device;

	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);

	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipeline(device->device, wireframe, nullptr);

	SetupDescriptorSets();
	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);
}

void Terrain::UpdateTerrain(TerrainQuad* quad, glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {

	glm::vec3 center = glm::vec3(quad->pos.x + quad->size.x / 2.0f, 0, quad->pos.z + quad->size.z / 2.0f);
	float distanceToViewer = glm::distance(viewerPos, center);

	if (!quad->isSubdivided) {
		if (distanceToViewer < quad->size.x * 2.0f && quad->level < maxLevels) {
			SubdivideTerrain(quad, gbo, lbo);
		}
	}
	else if (distanceToViewer > quad->size.x * 2.0f) {
		UnSubdivide(quad);

	}
	else if (quad->isSubdivided) {
		UpdateTerrain(quad->node.LowerRight, viewerPos, copyQueue, gbo, lbo);
		UpdateTerrain(quad->node.UpperLeft, viewerPos, copyQueue, gbo, lbo);
		UpdateTerrain(quad->node.LowerLeft, viewerPos, copyQueue, gbo, lbo);
		UpdateTerrain(quad->node.UpperRight, viewerPos, copyQueue, gbo, lbo);

	}

	UpdateMeshBuffer(copyQueue);
}

void Terrain::CleanUp() 
{
	//cleanup model buffers
	if (vertexBuffer.buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device->device, vertexBuffer.buffer, nullptr);
		vkFreeMemory(device->device, vertexBuffer.bufferMemory, nullptr);
	}
	if (indexBuffer.buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device->device, indexBuffer.buffer, nullptr);
		vkFreeMemory(device->device, indexBuffer.bufferMemory, nullptr);
	}

	terrainVulkanSplatMap.destroy();
	terrainVulkanTextureArray.destroy();

	modelUniformBuffer.cleanBuffer();

	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);

	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipeline(device->device, wireframe, nullptr);
}

void Terrain::LoadTexture(std::string filename) {
	terrainSplatMap = new Texture();
	terrainSplatMap->loadFromFile(filename);
}

void Terrain::LoadTextureArray() {
	terrainTextureArray = new TextureArray();
	terrainTextureArray->loadFromFile("Resources/Textures/TerrainTextures/", texFileNames);
}

void Terrain::SetupUniformBuffer()
{
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &modelUniformBuffer, sizeof(ModelBufferObject) * maxNumQuads );
}

void Terrain::SetupImage() 
{
	if (terrainSplatMap != nullptr) {
		terrainVulkanSplatMap.loadFromTexture(terrainSplatMap, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 1);
	}
	else {
		throw std::runtime_error("failed to load terrain splat map!");
	}if (terrainTextureArray != nullptr) {
		terrainVulkanTextureArray.loadTextureArray(terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);
	}
	else
		throw std::runtime_error("failed to load terrain texture array!");
}

void Terrain::SetupModel() 
{
	//terrainModel.loadFromMesh(terrainMesh, device, device->graphics_queue);


}

void Terrain::SetupDescriptorSets()
{
	//layout
	VkDescriptorSetLayoutBinding cboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
	VkDescriptorSetLayoutBinding uboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	VkDescriptorSetLayoutBinding lboLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);
	VkDescriptorSetLayoutBinding splatMapLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1);
	VkDescriptorSetLayoutBinding texArratLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1);

	std::vector<VkDescriptorSetLayoutBinding> bindings = { cboLayoutBinding, uboLayoutBinding, lboLayoutBinding, splatMapLayoutBinding, texArratLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	if (vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	//Pool
	std::vector<VkDescriptorPoolSize> poolSizesTerrain;
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxNumQuads));

	VkDescriptorPoolCreateInfo poolInfoTerrain =
		initializers::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizesTerrain.size()),
			poolSizesTerrain.data(),
			maxNumQuads);

	if (vkCreateDescriptorPool(device->device, &poolInfoTerrain, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	//Descriptor set
	std::vector<VkDescriptorSetLayout> layouts;
	layouts.resize(maxNumQuads);
	std::fill(layouts.begin(), layouts.end(), descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, layouts.data(), maxNumQuads);

	if (vkAllocateDescriptorSets(device->device, &allocInfoTerrain, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
	

	modelUniformBuffer.setupDescriptor();
}

void Terrain::SetupQuadDescriptor(TerrainQuad* quad, VulkanBuffer &gbo, VulkanBuffer &lbo){

	modelUniformBuffer.setupDescriptor(sizeof(ModelBufferObject), quad->uniformOffset * sizeof(ModelBufferObject));
	quad->UpdateModelUniformBuffer(device, modelUniformBuffer);

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(quad->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &gbo.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(quad->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &modelUniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(quad->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lbo.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(quad->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &terrainVulkanSplatMap.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(quad->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &terrainVulkanTextureArray.descriptor, 1));

	vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Terrain::SetupPipeline(VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight)
{
	VkShaderModule vertShaderModule = loadShaderModule(device->device, "shaders/terrain.vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule(device->device, "shaders/terrain.frag.spv");

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

	rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.lineWidth = 1.0f;

	if (device->physical_device_features.fillModeNonSolid == VK_TRUE) {
		if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframe) != VK_SUCCESS) {
			throw std::runtime_error("failed to create wireframe pipeline!");
		}
	}

	vkDestroyShaderModule(device->device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device->device, fragShaderModule, nullptr);
}

void Terrain::UploadMeshBuffer(VkQueue copyQueue) {

	vertexCount = meshVertexPool->MaxChunks() * sizeof(TerrainMeshVertices)/4;
	indexCount = meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices)/4;
	

	uint32_t vBufferSize = static_cast<uint32_t>(meshVertexPool->MaxChunks()) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(meshIndexPool->MaxChunks()) * sizeof(TerrainMeshIndices);


	// Use staging buffer to move vertex and index buffer to device local memory
	// Create staging buffers
	VulkanBuffer vertexStaging, indexStaging;

	// Vertex buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&vertexStaging,
		vBufferSize,
		meshVertexPool->GetDataPtr()->data.data()));

	// Index buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&indexStaging,
		iBufferSize,
		meshIndexPool->GetDataPtr()->data.data()));

	// Create device local target buffers
	// Vertex buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vertexBuffer,
		vBufferSize));

	// Index buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&indexBuffer,
		iBufferSize));

	// Copy from staging buffers
	VkCommandBuffer copyCmd = device->createCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};

	copyRegion.size = vertexBuffer.size;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

	copyRegion.size = indexBuffer.size;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

	device->flushCommandBuffer(copyCmd, copyQueue);

	// Destroy staging resources
	vkDestroyBuffer(device->device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device->device, vertexStaging.bufferMemory, nullptr);
	vkDestroyBuffer(device->device, indexStaging.buffer, nullptr);
	vkFreeMemory(device->device, indexStaging.bufferMemory, nullptr);

}

void Terrain::UpdateMeshBuffer(VkQueue copyQueue) {
//	vertexBuffer.map(device->device);
//	vertexBuffer.copyTo(&meshVertexPool, meshVertexPool->ChunksUsed() * sizeof(TerrainMeshVertices));
//	vertexBuffer.unmap();
//
//	indexBuffer.map(device->device);
//	indexBuffer.copyTo(&meshIndexPool, meshIndexPool->ChunksUsed() * sizeof(TerrainMeshIndices));
//	indexBuffer.unmap();

	vertexCount = meshVertexPool->MaxChunks() * sizeof(TerrainMeshVertices) / 4;
	indexCount = meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices) / 4;


	uint32_t vBufferSize = static_cast<uint32_t>(meshVertexPool->MaxChunks()) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(meshIndexPool->MaxChunks()) * sizeof(TerrainMeshIndices);


	// Use staging buffer to move vertex and index buffer to device local memory
	// Create staging buffers
	VulkanBuffer vertexStaging, indexStaging;

	// Vertex buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&vertexStaging,
		vBufferSize,
		meshVertexPool->GetDataPtr()->data.data()));

	// Index buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&indexStaging,
		iBufferSize,
		meshIndexPool->GetDataPtr()->data.data()));


	// Copy from staging buffers
	VkCommandBuffer copyCmd = device->createCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};

	copyRegion.size = vertexBuffer.size;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

	copyRegion.size = indexBuffer.size;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

	device->flushCommandBuffer(copyCmd, copyQueue);

	// Destroy staging resources
	vkDestroyBuffer(device->device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device->device, vertexStaging.bufferMemory, nullptr);
	vkDestroyBuffer(device->device, indexStaging.buffer, nullptr);
	vkFreeMemory(device->device, indexStaging.bufferMemory, nullptr);
}

void Terrain::SubdivideTerrain(TerrainQuad* quad, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	quad->isSubdivided = true;

	glm::vec3 new_pos = glm::vec3(quad->pos.x, 0, quad->pos.z);
	glm::vec3 new_size = glm::vec3(quad->size.x/2, 0, quad->size.z/2);

	quad->node.LowerLeft = terrainQuadPool->allocate();
	quad->node.LowerLeft->init(new_pos.x, new_pos.z, new_size.x, new_size.z, quad->level + 1, 1, meshVertexPool->allocate(), meshIndexPool->allocate(), descriptorSets[1]);
	SetupQuadDescriptor(quad->node.LowerLeft, gbo, lbo);

	quad->node.LowerRight = terrainQuadPool->allocate();
	quad->node.LowerRight->init(new_pos.x, new_pos.z + new_size.z, new_size.x , new_size.z, quad->level + 1, 2, meshVertexPool->allocate(), meshIndexPool->allocate(), descriptorSets[2]);
	SetupQuadDescriptor(quad->node.LowerRight, gbo, lbo);

	quad->node.UpperLeft = terrainQuadPool->allocate();
	quad->node.UpperLeft->init(new_pos.x + new_size.x, new_pos.z, new_size.x, new_size.z, quad->level + 1, 3, meshVertexPool->allocate(), meshIndexPool->allocate(), descriptorSets[3]);
	SetupQuadDescriptor(quad->node.UpperLeft, gbo, lbo);

	quad->node.UpperRight = terrainQuadPool->allocate();
	quad->node.UpperRight->init(new_pos.x + new_size.x, new_pos.z + new_size.z, new_size.x, new_size.z, quad->level + 1, 4, meshVertexPool->allocate(), meshIndexPool->allocate(), descriptorSets[4]);
	SetupQuadDescriptor(quad->node.UpperRight, gbo, lbo);

	//subdivisionTree.LowerRight->InitTerrain(device, renderPass, viewPortWidth, viewPortHeight, global, lighting);

	//remove current level resources

	std::cout << "Terrain subdivided: Level " << quad->level << std::endl;

	
}

void Terrain::UnSubdivide(TerrainQuad* quad) {
	if (quad->isSubdivided) {
		meshVertexPool->deallocate(quad->node.LowerRight->vertsPtr);
		meshVertexPool->deallocate(quad->node.UpperLeft->vertsPtr);
		meshVertexPool->deallocate(quad->node.LowerLeft->vertsPtr);
		meshVertexPool->deallocate(quad->node.UpperRight->vertsPtr);

		meshIndexPool->deallocate(quad->node.LowerRight->indicesPtr);
		meshIndexPool->deallocate(quad->node.UpperLeft->indicesPtr);
		meshIndexPool->deallocate(quad->node.LowerLeft->indicesPtr);
		meshIndexPool->deallocate(quad->node.UpperRight->indicesPtr);
		
		quad->node.LowerRight->CleanUp();
		quad->node.UpperLeft->CleanUp();
		quad->node.LowerLeft->CleanUp();
		quad->node.UpperRight->CleanUp();

		terrainQuadPool->deallocate(quad->node.LowerLeft);
		terrainQuadPool->deallocate(quad->node.LowerRight);
		terrainQuadPool->deallocate(quad->node.UpperLeft);
		terrainQuadPool->deallocate(quad->node.UpperRight);
	}
	quad->isSubdivided = false;
	std::cout << "Terrain un-subdivided: Level " << quad->level << std::endl;
}

void Terrain::UpdateUniformBuffer(float time)
{

}

void Terrain::DrawTerrain(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain) {

	//vkCmdBindPipeline(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? wireframe : pipeline);
	//DrawTerrainQuad(curTerrain->rootQuad, cmdBuff, cmdBuffIndex, offsets);
	//return;

	

	vkCmdBindPipeline(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? wireframe : pipeline);
	
	std::vector<VkDeviceSize> vertexOffsettings;
	
	for (int i = 0; i < meshVertexPool->ChunksUsed(); i++) {
		vertexOffsettings.push_back(i * sizeof(TerrainMeshVertices));
	}

	std::vector<VkDeviceSize> indexOffsettings;
	for (int i = 0; i < meshVertexPool->ChunksUsed(); i++) {
		indexOffsettings.push_back(i * sizeof(TerrainMeshIndices));
	}

	
	auto* quadPtr = terrainQuadPool->GetDataPtr();
	for (int i = 0; i < terrainQuadPool->ChunksUsed(); i++) {
		vkCmdBindVertexBuffers(cmdBuff[cmdBuffIndex], 0, 1, &vertexBuffer.buffer, vertexOffsettings.data());
		vkCmdBindIndexBuffer(cmdBuff[cmdBuffIndex], indexBuffer.buffer, indexOffsettings[i], VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &quadPtr->descriptorSet, 0, nullptr);
		
		
		vkCmdDrawIndexed(cmdBuff[cmdBuffIndex], static_cast<uint32_t>(indCount), 1, 0, 0, 0);

		quadPtr++;
	}
	
	

}

void Terrain::DrawTerrainQuad(TerrainQuad* quad, std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1]) {
	


	if (quad->isSubdivided) {
		DrawTerrainQuad(quad->node.LowerLeft, cmdBuff, cmdBuffIndex, offsets);
		DrawTerrainQuad(quad->node.LowerRight, cmdBuff, cmdBuffIndex, offsets);
		DrawTerrainQuad(quad->node.UpperLeft, cmdBuff, cmdBuffIndex, offsets);
		DrawTerrainQuad(quad->node.UpperRight, cmdBuff, cmdBuffIndex, offsets);
	}
	else {

	vkCmdBindDescriptorSets(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &quad->descriptorSet, 0, nullptr);
	VkDeviceSize mOffset = (quad->modelOffset);
	vkCmdBindVertexBuffers(cmdBuff[cmdBuffIndex], 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBuff[cmdBuffIndex], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmdBuff[cmdBuffIndex], static_cast<uint32_t>(indCount * meshIndexPool->ChunksUsed()), 1, 0, 0, 0);
	
	}
}