#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>



TerrainQuad::TerrainQuad() {
	pos = glm::vec3(0);
	size = glm::vec3(0);
	level = 0;
	isSubdivided = false;
}

TerrainQuad::~TerrainQuad() {}

void TerrainQuad::init(float posX, float posY, float sizeX, float sizeY, int level) {
	pos = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);
	this->level = level;
	isSubdivided = false;
	modelUniformObject.model = glm::translate(glm::mat4(), pos);
	modelUniformObject.normal = glm::mat4();// glm::transpose(glm::inverse(glm::mat3(modelUniformObject.model))));

}

void TerrainQuad::CleanUp() {
	
}

void TerrainQuad::CreateTerrainMesh(TerrainMeshVertices* verts, TerrainMeshIndices* indices) {

	uint32_t vertexCount = verts->size();
	uint32_t indexCount = indices->size();

	Mesh mesh = *genTerrain(NumCells, pos.x , pos.z, size.x, size.z);
	//memcpy(verts->data(), mesh.vertices.data(), vertexCount * sizeof(Vertex));
	///*
	for (int i = 0; i < (int)vertexCount/(sizeof(Vertex)/4); i++)
	{
		
		verts->at(i * vertElementCount + 0) =	mesh.vertices[i].pos[0];
		verts->at(i * vertElementCount + 1) =	mesh.vertices[i].pos[1];
		verts->at(i * vertElementCount + 2) =	mesh.vertices[i].pos[2];
		verts->at(i * vertElementCount + 3) =	mesh.vertices[i].normal[0];
		verts->at(i * vertElementCount + 4) =	mesh.vertices[i].normal[1];
		verts->at(i * vertElementCount + 5) =	mesh.vertices[i].normal[2];
		verts->at(i * vertElementCount + 6) =	mesh.vertices[i].texCoord[0];
		verts->at(i * vertElementCount + 7) =	mesh.vertices[i].texCoord[1];
		verts->at(i * vertElementCount + 8) =	mesh.vertices[i].color[0];
		verts->at(i * vertElementCount + 9) =	mesh.vertices[i].color[1];
		verts->at(i * vertElementCount + 10) =	mesh.vertices[i].color[2];
		verts->at(i * vertElementCount + 11) =	mesh.vertices[i].color[3];
	}
	//*/

	//memcpy(indices->data(), mesh.indices.data(), indexCount * sizeof(float));
	///*
	for (int i = 0; i < (int)indexCount; i++)
	{
		indices->at(i) = mesh.indices[i];
	}
	//*/



	
}










Terrain::Terrain(int numCells, int maxLevels, float posX, float posY, float sizeX, float sizeY) : maxLevels(maxLevels)
{
	//simple calculation right now, does the absolute max number of quads possible with given max level
	//in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0));
	}
	terrainQuads = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
	quadHandles.reserve(maxNumQuads);

	position = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());
}


Terrain::~Terrain() {
	terrainSplatMap->~Texture();
	terrainTextureArray->~TextureArray();

	terrainQuads->~MemoryPool();
	
}


void Terrain::InitTerrain(VulkanDevice* device, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->device = device;

	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptorLayoutAndPool();
	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);

	rootQuad = InitTerrainQuad(position, size, 0, global, lighting);

	UpdateModelBuffer(copyQueue, global, lighting);
	UpdateMeshBuffer(copyQueue);
}

void Terrain::ReinitTerrain(VulkanDevice* device, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting)
{
	this->device = device;

	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipeline(device->device, wireframe, nullptr);

	SetupPipeline(renderPass, viewPortWidth, viewPortHeight);
}

void Terrain::UpdateTerrain(glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {

	std::vector<std::vector<TerrainQuadData>::iterator> toDelete;

	bool shouldUpdateBuffers = UpdateTerrainQuad(rootQuad, viewerPos, copyQueue, gbo, lbo);
	
	if (shouldUpdateBuffers) {
		UpdateModelBuffer(copyQueue, gbo, lbo);
		UpdateMeshBuffer(copyQueue);
	}
}

bool Terrain::UpdateTerrainQuad(TerrainQuadData* quad, glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	
	glm::vec3 center = glm::vec3(quad->terrainQuad.pos.x + quad->terrainQuad.size.x / 2.0f, 0, quad->terrainQuad.pos.z + quad->terrainQuad.size.z / 2.0f);
	float distanceToViewer = glm::distance(viewerPos, center);
	bool shouldUpdateBuffers = false;;


	if (!quad->terrainQuad.isSubdivided) {
		if (distanceToViewer < quad->terrainQuad.size.x * 2.0f && quad->terrainQuad.level < maxLevels) {
			SubdivideTerrain(quad, gbo, lbo);
			shouldUpdateBuffers = true;
		} 
	}
	else if (distanceToViewer > quad->terrainQuad.size.x * 2.0f) {
		//toDelete.push_back(it);
		UnSubdivide(quad);
		shouldUpdateBuffers = true;
	}

	else if (quad->terrainQuad.isSubdivided) {
		bool uL = false, uR = false, dL = false, dR = false;
		
		uL = UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos, copyQueue, gbo, lbo);
		uR = UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos, copyQueue, gbo, lbo);
		dL = UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos, copyQueue, gbo, lbo);
		dR = UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos, copyQueue, gbo, lbo);
		
		if (uL || uR || dL || dR)
			shouldUpdateBuffers = true;
	}
	
	return shouldUpdateBuffers;
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
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &modelUniformBuffer, sizeof(ModelBufferObject) * maxNumQuads);
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

void Terrain::SetupDescriptorLayoutAndPool()
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

	poolInfoTerrain.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device->device, &poolInfoTerrain, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
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

void Terrain::UpdateModelBuffer(VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	
	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		
		vkFreeDescriptorSets(device->device, descriptorPool, 1, &(*it)->descriptorSet);
		

		if (vkAllocateDescriptorSets(device->device, &allocInfoTerrain, &(*it)->descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		modelUniformBuffer.setupDescriptor(sizeof(ModelBufferObject), (it - quadHandles.begin()) * sizeof(ModelBufferObject));

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.push_back(initializers::writeDescriptorSet((*it)->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &gbo.descriptor, 1));
		descriptorWrites.push_back(initializers::writeDescriptorSet((*it)->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &modelUniformBuffer.descriptor, 1));
		descriptorWrites.push_back(initializers::writeDescriptorSet((*it)->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lbo.descriptor, 1));
		descriptorWrites.push_back(initializers::writeDescriptorSet((*it)->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &terrainVulkanSplatMap.descriptor, 1));
		descriptorWrites.push_back(initializers::writeDescriptorSet((*it)->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &terrainVulkanTextureArray.descriptor, 1));

		vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	}

	std::vector<ModelBufferObject> modelObjects;
	modelObjects.reserve(quadHandles.size());

	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		modelObjects.push_back((*it)->terrainQuad.modelUniformObject);
	}

	modelUniformBuffer.map(device->device, modelObjects.size() * sizeof(ModelBufferObject));
	modelUniformBuffer.copyTo(modelObjects.data(), modelObjects.size() * sizeof(ModelBufferObject));
	modelUniformBuffer.unmap();
}

void Terrain::UploadMeshBuffer(VkQueue copyQueue) {

	//vertexCount = terrainQuads->size() * sizeof(TerrainMeshVertices) / 4;
	//indexCount = meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices) / 4;


	uint32_t vBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshIndices);
	
	std::vector<TerrainMeshVertices> verts;
	verts.reserve(quadHandles.size());
	
	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		verts.push_back((*it)->vertices);
		//memcpy(&verts.at(it - terrainQuads->begin()), &terrainQuads->at(it - terrainQuads->begin()).vertices, sizeof(TerrainMeshVertices));
	}

	std::vector<TerrainMeshIndices> inds;
	inds.reserve(quadHandles.size());
	
	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		inds.push_back((*it)->indices);
		//memcpy(&inds.at(it - terrainQuads->begin()), &terrainQuads->at(it - terrainQuads->begin()).indices, sizeof(TerrainMeshIndices));
	}

	// Use staging buffer to move vertex and index buffer to device local memory
	// Create staging buffers
	VulkanBuffer vertexStaging, indexStaging;
	
	// Vertex buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&vertexStaging,
		vBufferSize,
		verts.data()));
	
	// Index buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&indexStaging,
		iBufferSize,
		inds.data()));
	
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
//	vertexBuffer.map(device->device, meshVertexPool->MaxChunks() * sizeof(TerrainMeshVertices), 0);
//	vertexBuffer.copyTo(&meshVertexPool, meshVertexPool->ChunksUsed() * sizeof(TerrainMeshVertices));
//	vertexBuffer.unmap();
//
//	indexBuffer.map(device->device, meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices), 0);
//	indexBuffer.copyTo(&meshIndexPool, meshIndexPool->ChunksUsed() * sizeof(TerrainMeshIndices));
//	indexBuffer.unmap();
//

	vkDestroyBuffer(device->device, vertexBuffer.buffer, nullptr);
	vkFreeMemory(device->device, vertexBuffer.bufferMemory, nullptr);
	vkDestroyBuffer(device->device, indexBuffer.buffer, nullptr);
	vkFreeMemory(device->device, indexBuffer.bufferMemory, nullptr);

	uint32_t vBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshIndices);

	std::vector<TerrainMeshVertices> verts;
	verts.reserve(quadHandles.size());

	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		verts.push_back((*it)->vertices);
		//memcpy(&verts.at(it - quadHandles.begin()), &(*it)->vertices, sizeof(TerrainMeshVertices));
	}

	std::vector<TerrainMeshIndices> inds;
	inds.reserve(quadHandles.size());

	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		inds.push_back((*it)->indices);
		//memcpy(&inds.at(it - quadHandles.begin()), &(*it)->indices, sizeof(TerrainMeshIndices));
	}

	// Use staging buffer to move vertex and index buffer to device local memory
	// Create staging buffers
	VulkanBuffer vertexStaging, indexStaging;

	// Vertex buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&vertexStaging,
		vBufferSize,
		verts.data()));

	// Index buffer
	VK_CHECK_RESULT(device->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		&indexStaging,
		iBufferSize,
		inds.data()));

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

TerrainQuadData* Terrain::InitTerrainQuad(glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	TerrainQuadData* q = terrainQuads->allocate();
	quadHandles.push_back(q);

	q->terrainQuad.init(position.x, position.z, size.x, size.z, level);
	q->terrainQuad.CreateTerrainMesh(&q->vertices, &q->indices);

	//std::vector<VkDescriptorSetLayout> layouts;
	//layouts.resize(maxNumQuads);
	//std::fill(layouts.begin(), layouts.end(), descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	
	if (vkAllocateDescriptorSets(device->device, &allocInfoTerrain, &q->descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
	
	modelUniformBuffer.setupDescriptor(sizeof(ModelBufferObject), (quadHandles.size() - 1) * sizeof(ModelBufferObject));

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.push_back(initializers::writeDescriptorSet(q->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &gbo.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(q->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &modelUniformBuffer.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(q->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lbo.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(q->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &terrainVulkanSplatMap.descriptor, 1));
	descriptorWrites.push_back(initializers::writeDescriptorSet(q->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &terrainVulkanTextureArray.descriptor, 1));

	vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	return q;
}

void Terrain::SubdivideTerrain(TerrainQuadData* quad, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	quad->terrainQuad.isSubdivided = true;

	glm::vec3 new_pos = glm::vec3(quad->terrainQuad.pos.x, 0, quad->terrainQuad.pos.z);
	glm::vec3 new_size = glm::vec3(quad->terrainQuad.size.x/2.0, 0, quad->terrainQuad.size.z/2.0);

	quad->subQuads.UpRight = InitTerrainQuad(glm::vec3(new_pos.x, 0, new_pos.z), new_size, quad->terrainQuad.level + 1, gbo, lbo);

	quad->subQuads.UpLeft = InitTerrainQuad(glm::vec3(new_pos.x, 0, new_pos.z + new_size.z), new_size, quad->terrainQuad.level + 1, gbo, lbo);
	
	quad->subQuads.DownRight = InitTerrainQuad(glm::vec3(new_pos.x + new_size.x, 0, new_pos.z), new_size, quad->terrainQuad.level + 1, gbo, lbo);
	
	quad->subQuads.DownLeft = InitTerrainQuad(glm::vec3(new_pos.x + new_size.x, 0, new_pos.z + new_size.z), new_size, quad->terrainQuad.level + 1, gbo, lbo);

	//std::cout << "Terrain subdivided: Level: " << quad->terrainQuad.level << " Position: " << quad->terrainQuad.pos.x << ", " <<quad->terrainQuad.pos.z << " Size: " << quad->terrainQuad.size.x << ", " << quad->terrainQuad.size.z << std::endl;

	
}

void Terrain::UnSubdivide(TerrainQuadData* quad) {
	if (quad->terrainQuad.isSubdivided) {
		auto delUR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpRight);
		if(delUR != quadHandles.end())
			quadHandles.erase(delUR);
		vkFreeDescriptorSets(device->device, descriptorPool, 1, &quad->subQuads.UpRight->descriptorSet);
		terrainQuads->deallocate(quad->subQuads.UpRight);


		auto delDR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownRight);
		if (delDR != quadHandles.end())
			quadHandles.erase(delDR);
		vkFreeDescriptorSets(device->device, descriptorPool, 1, &quad->subQuads.DownRight->descriptorSet);
		terrainQuads->deallocate(quad->subQuads.DownRight);


		auto delUL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpLeft);
		if (delUL != quadHandles.end())
			quadHandles.erase(delUL);
		vkFreeDescriptorSets(device->device, descriptorPool, 1, &quad->subQuads.UpLeft->descriptorSet);
		terrainQuads->deallocate(quad->subQuads.UpLeft);


		auto delDL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownLeft);
		if (delDL != quadHandles.end())
			quadHandles.erase(delDL);
		vkFreeDescriptorSets(device->device, descriptorPool, 1, &quad->subQuads.DownLeft->descriptorSet);
		terrainQuads->deallocate(quad->subQuads.DownLeft);

		quad->terrainQuad.isSubdivided = false;
	}
	//quad->isSubdivided = false;
	//std::cout << "Terrain un-subdivided: Level: " << quad->terrainQuad.level << " Position: " << quad->terrainQuad.pos.x << ", " << quad->terrainQuad.pos.z << " Size: " << quad->terrainQuad.size.x << ", " << quad->terrainQuad.size.z << std::endl;
}

void Terrain::UpdateUniformBuffer(float time)
{

}

void Terrain::DrawTerrain(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe) {

	//vkCmdBindPipeline(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? wireframe : pipeline);
	//DrawTerrainQuad(curTerrain->rootQuad, cmdBuff, cmdBuffIndex, offsets);
	//return;

	

	vkCmdBindPipeline(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? wireframe : pipeline);
	
	std::vector<VkDeviceSize> vertexOffsettings;
	
	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings.push_back(i * sizeof(TerrainMeshVertices));
	}

	std::vector<VkDeviceSize> indexOffsettings;
	for (int i = 0; i < quadHandles.size(); i++) {
		indexOffsettings.push_back(i * sizeof(TerrainMeshIndices));
	}

	
	for (auto it = quadHandles.begin(); it < quadHandles.end(); it++) {
		if (!(*it)->terrainQuad.isSubdivided) {
			vkCmdBindVertexBuffers(cmdBuff[cmdBuffIndex], 0, 1, &vertexBuffer.buffer, &vertexOffsettings[it - quadHandles.begin()]);
			vkCmdBindIndexBuffer(cmdBuff[cmdBuffIndex], indexBuffer.buffer, indexOffsettings[it - quadHandles.begin()], VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(*it)->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(cmdBuff[cmdBuffIndex], static_cast<uint32_t>(indCount), 1, 0, 0, 0);
		}
	}
	
	

}

/*
void Terrain::BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe) {
	vkCmdBindPipeline(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? wireframe : pipeline);

	std::vector<VkDeviceSize> vertexOffsettings;

	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings.push_back(i * sizeof(TerrainMeshVertices));
	}

	std::vector<VkDeviceSize> indexOffsettings;
	for (int i = 0; i < quadHandles.size(); i++) {
		indexOffsettings.push_back(i * sizeof(TerrainMeshIndices));
	}


	for (auto it = quadHandles.begin(); it < quadHandles.end(); it++) {
		if (!(*it)->terrainQuad.isSubdivided) {
			vkCmdBindVertexBuffers(cmdBuff[cmdBuffIndex], 0, 1, &vertexBuffer.buffer, &vertexOffsettings[it - quadHandles.begin()]);
			vkCmdBindIndexBuffer(cmdBuff[cmdBuffIndex], indexBuffer.buffer, indexOffsettings[it - quadHandles.begin()], VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(cmdBuff[cmdBuffIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(*it)->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(cmdBuff[cmdBuffIndex], static_cast<uint32_t>(indCount), 1, 0, 0, 0);
		}
	}



}
*/