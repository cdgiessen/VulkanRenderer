#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>



TerrainQuad::TerrainQuad() {
	pos = glm::vec3(0);
	size = glm::vec3(0);
	level = 0;
	isSubdivided = false;
}

TerrainQuad::~TerrainQuad() {}

void TerrainQuad::init(float posX, float posY, float sizeX, float sizeY, int level, int subDivPosX, int subDivPosZ, float centerHeightValue) {
	pos = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);
	this->level = level;
	isSubdivided = false;
	this->subDivPosX = subDivPosX;
	this->subDivPosZ = subDivPosZ;
	heightValAtCenter = centerHeightValue;
	modelUniformObject.model = glm::translate(glm::mat4(), pos);
	modelUniformObject.normal = glm::mat4();// glm::transpose(glm::inverse(glm::mat3(modelUniformObject.model))));

	bool isMeshDataCreated = false;

}

Terrain::Terrain(MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>* pool,int numCells, int maxLevels, float posX, float posY, float sizeX, float sizeY) : maxLevels(maxLevels)
{
	//simple calculation right now, does the absolute max number of quads possible with given max level
	//in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + + 25 + 50 * maxLevels; //with current quad density this is the average upper bound (kidna a guess but its probably more than enough for now (had to add 25 cause it wasn't enough lol!)
		//maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}
	//terrainQuads = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
	terrainQuads = pool;
	quadHandles.reserve(maxNumQuads);
	//terrainGenerationWorkers = std::vector < std::thread>(maxNumQuads);

	position = glm::vec3(posX, 0, posY);
	size = glm::vec3(sizeX, 0, sizeY);

	terrainGenerator = new TerrainGenerator(numCells, SplatMapSize, position, size);
	LoadSplatMapFromGenerator();
	LoadTextureArray();

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());
}


Terrain::~Terrain() {
	terrainSplatMap->~Texture();
	terrainTextureArray->~TextureArray();

	//terrainQuads->~MemoryPool();
	
}


void Terrain::InitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting, glm::vec3 cameraPos)
{
	this->device = device;

	SetupMeshbuffers();
	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptorLayoutAndPool();
	SetupPipeline(pipelineManager, renderPass, viewPortWidth, viewPortHeight);

	TerrainQuadData* q = terrainQuads->allocate();
	quadHandles.push_back(q);
	rootQuad = InitTerrainQuad(q, position, size, 0, global, lighting);

	UpdateTerrainQuad(rootQuad, cameraPos, copyQueue, global, lighting);

	UpdateModelBuffer(copyQueue, global, lighting);
	UpdateMeshBuffer(copyQueue);
}

void Terrain::ReinitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight)
{
	this->device = device;

	vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
	vkDestroyPipeline(device->device, pipeline, nullptr);
	vkDestroyPipeline(device->device, wireframe, nullptr);
	vkDestroyPipeline(device->device, debugNormals, nullptr);

	SetupPipeline(pipelineManager, renderPass, viewPortWidth, viewPortHeight);
}

void Terrain::UpdateTerrain(glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	SimpleTimer updateTerrainQuadTime, gpuTransfersTime;
	updateTerrainQuadTime.StartTimer();
	
	bool shouldUpdateBuffers = UpdateTerrainQuad(rootQuad, viewerPos, copyQueue, gbo, lbo);
	
	updateTerrainQuadTime.EndTimer();
	gpuTransfersTime.StartTimer();
	
	if (shouldUpdateBuffers) {
		UpdateModelBuffer(copyQueue, gbo, lbo);
		UpdateMeshBuffer(copyQueue);
	}
	PrevQuadHandles = quadHandles;

	gpuTransfersTime.EndTimer();
	
	if (updateTerrainQuadTime.GetElapsedTimeMicroSeconds() > 1000 || gpuTransfersTime.GetElapsedTimeMicroSeconds() > 1000)
		std::cout << "update time " << updateTerrainQuadTime.GetElapsedTimeMicroSeconds() << " transfer time " << gpuTransfersTime.GetElapsedTimeMicroSeconds() << std::endl;
}

bool Terrain::UpdateTerrainQuad(TerrainQuadData* quad, glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	
	glm::vec3 center = glm::vec3(quad->terrainQuad.pos.x + quad->terrainQuad.size.x / 2.0f, quad->terrainQuad.heightValAtCenter, quad->terrainQuad.pos.z + quad->terrainQuad.size.z / 2.0f);
	float distanceToViewer = glm::distance(viewerPos, center);
	bool shouldUpdateBuffers = false;

	if (!quad->terrainQuad.isSubdivided) { //can only subdivide if this quad isn't already subdivided
		if (distanceToViewer < quad->terrainQuad.size.x * 2.0f && quad->terrainQuad.level < maxLevels) { //must be 
			SubdivideTerrain(quad, copyQueue, viewerPos, gbo, lbo);
			shouldUpdateBuffers = true;
		} 
	}
	else if (distanceToViewer > quad->terrainQuad.size.x * 2.0f) {
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
	vkDestroyPipeline(device->device, debugNormals, nullptr);
}

void Terrain::LoadSplatMapFromGenerator() {
	terrainSplatMap = new Texture();
	terrainSplatMap->loadFromNoiseUtilImage(terrainGenerator->getImagePtr());
	
}

void Terrain::LoadTextureArray() {
	terrainTextureArray = new TextureArray();
	terrainTextureArray->loadFromFile("Resources/Textures/TerrainTextures/", texFileNames);
}

void Terrain::SetupMeshbuffers() {
	uint32_t vBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshIndices);

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
}

void Terrain::SetupUniformBuffer()
{
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &modelUniformBuffer, sizeof(ModelBufferObject) * maxNumQuads);
}

void Terrain::SetupImage() 
{
	if (terrainSplatMap != nullptr) {
		terrainVulkanSplatMap.loadFromTexture(terrainSplatMap, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	
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
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));

	VkDescriptorPoolCreateInfo poolInfoTerrain =
		initializers::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizesTerrain.size()),
			poolSizesTerrain.data(),
			1 * maxNumQuads);

	poolInfoTerrain.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device->device, &poolInfoTerrain, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Terrain::SetupPipeline(VulkanPipeline PipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight)
{
	PipelineCreationObject* myPipe = PipelineManager.CreatePipelineOutline();

	PipelineManager.SetVertexShader(myPipe, loadShaderModule(device->device, "shaders/terrain.vert.spv"));
	PipelineManager.SetFragmentShader(myPipe, loadShaderModule(device->device, "shaders/terrain.frag.spv"));
	PipelineManager.SetVertexInput(myPipe, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	PipelineManager.SetInputAssembly(myPipe, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	PipelineManager.SetViewport(myPipe, (float)viewPortWidth, (float)viewPortHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	PipelineManager.SetScissor(myPipe, viewPortWidth, viewPortHeight, 0, 0);
	PipelineManager.SetViewportState(myPipe, 1, 1, 0);
	PipelineManager.SetRasterizer(myPipe, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	PipelineManager.SetMultisampling(myPipe, VK_SAMPLE_COUNT_1_BIT);
	PipelineManager.SetDepthStencil(myPipe, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE);
	PipelineManager.SetColorBlendingAttachment(myPipe, VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
	PipelineManager.SetColorBlending(myPipe, 1, &myPipe->colorBlendAttachment);
	PipelineManager.SetDescriptorSetLayout(myPipe, { &descriptorSetLayout }, 1);

	pipelineLayout = PipelineManager.BuildPipelineLayout(myPipe);
	pipeline = PipelineManager.BuildPipeline(myPipe, renderPass, 0);

	PipelineManager.SetRasterizer(myPipe, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	wireframe = PipelineManager.BuildPipeline(myPipe, renderPass, 0);

	PipelineManager.CleanShaderResources(myPipe);
	PipelineManager.SetVertexShader(myPipe, loadShaderModule(device->device, "shaders/normalVecDebug.vert.spv"));
	PipelineManager.SetFragmentShader(myPipe, loadShaderModule(device->device, "shaders/normalVecDebug.frag.spv"));
	PipelineManager.SetGeometryShader(myPipe, loadShaderModule(device->device, "shaders/normalVecDebug.geom.spv"));

	debugNormals = PipelineManager.BuildPipeline(myPipe, renderPass, 0);
	PipelineManager.CleanShaderResources(myPipe);

	/*
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
	*/
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

void Terrain::UpdateMeshBuffer(VkQueue copyQueue) {
	//	vertexBuffer.map(device->device, meshVertexPool->MaxChunks() * sizeof(TerrainMeshVertices), 0);
	//	vertexBuffer.copyTo(&meshVertexPool, meshVertexPool->ChunksUsed() * sizeof(TerrainMeshVertices));
	//	vertexBuffer.unmap();
	//
	//	indexBuffer.map(device->device, meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices), 0);
	//	indexBuffer.copyTo(&meshIndexPool, meshIndexPool->ChunksUsed() * sizeof(TerrainMeshIndices));
	//	indexBuffer.unmap();
	//

	//vkDestroyBuffer(device->device, vertexBuffer.buffer, nullptr);
	//vkFreeMemory(device->device, vertexBuffer.bufferMemory, nullptr);
	//vkDestroyBuffer(device->device, indexBuffer.buffer, nullptr);
	//vkFreeMemory(device->device, indexBuffer.bufferMemory, nullptr);

	while(terrainGenerationWorkers.size() > 0){
		terrainGenerationWorkers.front()->join();
		terrainGenerationWorkers.front() = std::move(terrainGenerationWorkers.back());
		terrainGenerationWorkers.pop_back();
	}

	SimpleTimer cpuDataTime, gpuTransferTime;
	cpuDataTime.StartTimer();
	
	std::vector<VkBufferCopy> vertexCopyRegions;
	std::vector<VkBufferCopy> indexCopyRegions;

	uint32_t vBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(quadHandles.size()) * sizeof(TerrainMeshIndices);

	verts.resize(quadHandles.size());
	inds.resize(quadHandles.size());

	if (quadHandles.size() > PrevQuadHandles.size()) //more meshes than before
	{
		for (int i = 0; i < PrevQuadHandles.size(); i++) {
			if (quadHandles[i] != PrevQuadHandles[i]) {
				verts[i] = quadHandles[i]->vertices;
				inds[i] = quadHandles[i]->indices;
				
				VkBufferCopy vBufferRegion;
				vBufferRegion.size = sizeof(TerrainMeshVertices);
				vBufferRegion.srcOffset = i * sizeof(TerrainMeshVertices);
				vBufferRegion.dstOffset = i * sizeof(TerrainMeshVertices);
				vertexCopyRegions.push_back(vBufferRegion);

				VkBufferCopy iBufferRegion;
				iBufferRegion.size = sizeof(TerrainMeshIndices);
				iBufferRegion.srcOffset = i * sizeof(TerrainMeshIndices);
				iBufferRegion.dstOffset = i * sizeof(TerrainMeshIndices);
				indexCopyRegions.push_back(iBufferRegion);
			}
		}
		for (int i = PrevQuadHandles.size(); i < quadHandles.size(); i++) {
			verts[i] = quadHandles[i]->vertices;
			inds[i] = quadHandles[i]->indices;

			VkBufferCopy vBufferRegion;
			vBufferRegion.size = sizeof(TerrainMeshVertices);
			vBufferRegion.srcOffset = i * sizeof(TerrainMeshVertices);
			vBufferRegion.dstOffset = i * sizeof(TerrainMeshVertices);
			vertexCopyRegions.push_back(vBufferRegion);

			VkBufferCopy iBufferRegion;
			iBufferRegion.size = sizeof(TerrainMeshIndices);
			iBufferRegion.srcOffset = i * sizeof(TerrainMeshIndices);
			iBufferRegion.dstOffset = i * sizeof(TerrainMeshIndices);
			indexCopyRegions.push_back(iBufferRegion);
		}
	}
	else { //less meshes than before, can erase at end.
		for (int i = 0; i < quadHandles.size(); i++) {
			if (quadHandles[i] != PrevQuadHandles[i]) {
				verts[i] = quadHandles[i]->vertices;
				inds[i] = quadHandles[i]->indices;

				VkBufferCopy vBufferRegion;
				vBufferRegion.size = sizeof(TerrainMeshVertices);
				vBufferRegion.srcOffset = i * sizeof(TerrainMeshVertices);
				vBufferRegion.dstOffset = i * sizeof(TerrainMeshVertices);
				vertexCopyRegions.push_back(vBufferRegion);

				VkBufferCopy iBufferRegion;
				iBufferRegion.size = sizeof(TerrainMeshIndices);
				iBufferRegion.srcOffset = i * sizeof(TerrainMeshIndices);
				iBufferRegion.dstOffset = i * sizeof(TerrainMeshIndices);
				indexCopyRegions.push_back(iBufferRegion);
			}
		}
	}
	cpuDataTime.EndTimer();
	gpuTransferTime.StartTimer();
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
	//VK_CHECK_RESULT(device->createBuffer(
	//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//	&vertexBuffer,
	//	vBufferSize));

	//// Index buffer
	//VK_CHECK_RESULT(device->createBuffer(
	//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//	&indexBuffer,
	//	iBufferSize));

	// Copy from staging buffers
	VkCommandBuffer copyCmd = device->createCommandBuffer(device->transfer_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//VkBufferCopy copyRegion{};

	//copyRegion.size = vertexBuffer.size;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, vertexCopyRegions.size(), vertexCopyRegions.data());

	//copyRegion.size = indexBuffer.size;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, indexCopyRegions.size(), indexCopyRegions.data());

	device->flushCommandBuffer(device->transfer_queue_command_pool, copyCmd, copyQueue);

	// Destroy staging resources
	vkDestroyBuffer(device->device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device->device, vertexStaging.bufferMemory, nullptr);
	vkDestroyBuffer(device->device, indexStaging.buffer, nullptr);
	vkFreeMemory(device->device, indexStaging.bufferMemory, nullptr);

	gpuTransferTime.EndTimer();

	std::cout << "CPU " << cpuDataTime.GetElapsedTimeMicroSeconds() << std::endl;
	std::cout << "GPU " << gpuTransferTime.GetElapsedTimeMicroSeconds() << std::endl;
}



TerrainQuadData* Terrain::InitTerrainQuad(TerrainQuadData* q, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	numQuads++;
	
	q->terrainQuad.init(position.x, position.z, size.x, size.z, level, 0, 0, heightScale * terrainGenerator->SampleHeight(position.x + size.x/2, position.y, position.z + size.z/2));
	//q->terrainQuad.CreateTerrainMesh(&q->vertices, &q->indices);

	//SimpleTimer terrainQuadCreateTime;
	//terrainQuadCreateTime.StartTimer();
	
	std::thread* worker = new std::thread(GenerateNewTerrain, terrainGenerator, &q->vertices, &q->indices, q->terrainQuad, heightScale);
	terrainGenerationWorkers.push_back(worker);
	
	//terrainQuadCreateTime.EndTimer();
	//std::cout << "Original " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << std::endl;

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

TerrainQuadData* Terrain::InitTerrainQuadFromParent(TerrainQuadData* parent, TerrainQuadData* q, Corner_Enum corner, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo, int subDivPosX, int subDivPosZ) {
	
	q->terrainQuad.init(position.x, position.z, size.x, size.z, level, subDivPosX, subDivPosZ, heightScale * terrainGenerator->SampleHeight(position.x + size.x/2, position.y, position.z + size.z/2));
	//q->terrainQuad.CreateTerrainMeshFromParent(&parent->vertices, &parent->indices, &q->vertices, &q->indices, corner);

	//SimpleTimer terrainQuadCreateTime;
	//terrainQuadCreateTime.StartTimer();

	std::thread *worker = new std::thread(GenerateNewTerrainSubdivision, terrainGenerator, &q->vertices, &q->indices, q->terrainQuad, corner, heightScale);
	//std::thread worker = std::thread(GenerateTerrainFromExisting, &terrainGenerator, &parent->vertices, &parent->indices, &q->vertices, &q->indices, corner, q->terrainQuad, heightScale);
	terrainGenerationWorkers.push_back(worker);

	//terrainQuadCreateTime.EndTimer();
	//std::cout << "From Parent " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << std::endl;
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

void Terrain::SubdivideTerrain(TerrainQuadData* quad, VkQueue copyQueue, glm::vec3 viewerPos, VulkanBuffer &gbo, VulkanBuffer &lbo) {
	quad->terrainQuad.isSubdivided = true;
	numQuads += 4;

	glm::vec3 new_pos = glm::vec3(quad->terrainQuad.pos.x, 0, quad->terrainQuad.pos.z);
	glm::vec3 new_size = glm::vec3(quad->terrainQuad.size.x/2.0, 0, quad->terrainQuad.size.z/2.0);

	TerrainQuadData* qUR = terrainQuads->allocate();
	TerrainQuadData* qUL = terrainQuads->allocate();
	TerrainQuadData* qDR = terrainQuads->allocate();
	TerrainQuadData* qDL = terrainQuads->allocate();
	quadHandles.push_back(qUR);
	quadHandles.push_back(qUL);
	quadHandles.push_back(qDR);
	quadHandles.push_back(qDL);

	quad->subQuads.UpRight = InitTerrainQuadFromParent(quad, qUR, Corner_Enum::uR,glm::vec3(new_pos.x, 0, new_pos.z), new_size, quad->terrainQuad.level + 1, gbo, lbo, quad->terrainQuad.subDivPosX * 2, quad->terrainQuad.subDivPosZ * 2);
	quad->subQuads.UpLeft = InitTerrainQuadFromParent(quad, qUL, Corner_Enum::uL, glm::vec3(new_pos.x, 0, new_pos.z + new_size.z), new_size, quad->terrainQuad.level + 1, gbo, lbo, quad->terrainQuad.subDivPosX * 2, quad->terrainQuad.subDivPosZ * 2 + 1);
	quad->subQuads.DownRight = InitTerrainQuadFromParent(quad, qDR, Corner_Enum::dR, glm::vec3(new_pos.x + new_size.x, 0, new_pos.z), new_size, quad->terrainQuad.level + 1, gbo, lbo, quad->terrainQuad.subDivPosX * 2 + 1, quad->terrainQuad.subDivPosZ * 2);
	quad->subQuads.DownLeft = InitTerrainQuadFromParent(quad, qDL, Corner_Enum::dL, glm::vec3(new_pos.x + new_size.x, 0, new_pos.z + new_size.z), new_size, quad->terrainQuad.level + 1, gbo, lbo, quad->terrainQuad.subDivPosX * 2 + 1, quad->terrainQuad.subDivPosZ * 2 + 1);

	UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos, copyQueue, gbo, lbo);
	UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos, copyQueue, gbo, lbo);
	UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos, copyQueue, gbo, lbo);
	UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos, copyQueue, gbo, lbo);

	//std::cout << "Terrain subdivided: Level: " << quad->terrainQuad.level << " Position: " << quad->terrainQuad.pos.x << ", " <<quad->terrainQuad.pos.z << " Size: " << quad->terrainQuad.size.x << ", " << quad->terrainQuad.size.z << std::endl;

	
}

void Terrain::UnSubdivide(TerrainQuadData* quad) {
	if (quad->terrainQuad.isSubdivided) {
		numQuads -= 4;

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


void Terrain::DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe) {

	//vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? wireframe : pipeline);
	//DrawTerrainQuad(curTerrain->rootQuad, cmdBuff, cmdBuffIndex, offsets);
	//return;

	

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? wireframe : pipeline);
	
	std::vector<VkDeviceSize> vertexOffsettings(quadHandles.size());
	std::vector<VkDeviceSize> indexOffsettings(quadHandles.size());
	
	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings[i] = (i * sizeof(TerrainMeshVertices));
		indexOffsettings[i] = (i * sizeof(TerrainMeshIndices));
	}
	

	drawTimer.StartTimer();
	for (int i = 0; i < quadHandles.size(); i++) {
		if (!quadHandles[i]->terrainQuad.isSubdivided) {
			vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? wireframe : pipeline);
			vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertexBuffer.buffer, &vertexOffsettings[i]);
			vkCmdBindIndexBuffer(cmdBuff, indexBuffer.buffer, indexOffsettings[i], VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &quadHandles[i]->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(indCount), 1, 0, 0, 0);

			//Vertex normals (yay geometry shaders!)
			//vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, debugNormals);
			//vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(indCount), 1, 0, 0, 0);
		}
	}
	drawTimer.EndTimer();

	

}

/*
void Terrain::BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe) {
	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? wireframe : pipeline);

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
			vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertexBuffer.buffer, &vertexOffsettings[it - quadHandles.begin()]);
			vkCmdBindIndexBuffer(cmdBuff, indexBuffer.buffer, indexOffsettings[it - quadHandles.begin()], VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(*it)->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(indCount), 1, 0, 0, 0);
		}
	}



}
*/

glm::vec3 CalcNormal(double L, double R, double U, double D, double UL, double DL, double UR, double DR, double vertexDistance, int numCells) {

	return glm::normalize(glm::vec3(L + UL + DL - (R + UR + DR), 2 * vertexDistance / numCells, U + UL + UR - (D + DL + DR)));
}



void GenerateNewTerrain(TerrainGenerator *terrainGenerator, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, float heightScale) {
	int numCells = NumCells;
	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.z, xSize = terrainQuad.size.x, zSize = terrainQuad.size.z;

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			float value = (terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));
			
			float hL = terrainGenerator->SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			float hR = terrainGenerator->SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			float hD = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			float hU = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));


			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = (float)i *(xSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = (float)value * heightScale;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = (float)j * (zSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = i/(float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = j/(float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = terrainGenerator->SampleColor(0,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = terrainGenerator->SampleColor(1,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = terrainGenerator->SampleColor(2,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = 0;
			//std::cout << value << std::endl;
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
		}
	}
}

void GenerateNewTerrainSubdivision(TerrainGenerator *terrainGenerator, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, Corner_Enum corner, float heightScale) {
	int numCells = NumCells;
	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.z, xSize = terrainQuad.size.x, zSize = terrainQuad.size.z;

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			float value = (terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));

			float hL = terrainGenerator->SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			float hR = terrainGenerator->SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			float hD = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			float hU = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));


			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = (float)i *(xSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = (float)value * heightScale;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = (float)j * (zSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosX / (float)(1 << terrainQuad.level);
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosZ / (float)(1 << terrainQuad.level);		
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = terrainGenerator->SampleColor(0, (float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = terrainGenerator->SampleColor(1, (float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = terrainGenerator->SampleColor(2, (float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = 0;
			//std::cout << value << std::endl;
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
		}
	}
}

//Needs to account for which corner the new terrain is in
void GenerateTerrainFromExisting(TerrainGenerator &terrainGenerator, TerrainMeshVertices &parentVerts, TerrainMeshIndices &parentIndices, 
	TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner, TerrainQuad terrainQuad, float heightScale) {

	int numCells = NumCells;
	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.z, xSize = terrainQuad.size.x, zSize = terrainQuad.size.z;

	int parentIOffset = (corner == 2 || corner == 3) ? (numCells + 1) / 2 : 0;
	int parentJOffset = (corner == 1 || corner == 3) ? (numCells + 1) / 2 : 0;
	
	//uses the parent terrain for 1/4 of the grid
	for (int i = 0; i <= numCells; i += 2)
	{
		for (int j = 0; j <= numCells; j += 2)
		{
			int vLoc = (i*(numCells + 1) + j)* vertElementCount;

			//existing terrain is every other vertex in the grid
			int parentVLoc = ((i / 2 + parentIOffset)*(numCells + 1) + (j / 2 + parentJOffset))* vertElementCount;

			(*verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
			(*verts)[vLoc + 1] = (parentVerts)[parentVLoc + 1];
			(*verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
			(*verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosX / (float)(1 << terrainQuad.level);
			(*verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosZ / (float)(1 << terrainQuad.level);
			(*verts)[vLoc + 8] = (parentVerts)[parentVLoc + 8];
			(*verts)[vLoc + 9] = (parentVerts)[parentVLoc + 9];
			(*verts)[vLoc + 10] = (parentVerts)[parentVLoc + 10];
			(*verts)[vLoc + 11] = 1.0;

		}
	}
	/* //seams are hard to fix... (cause its a runtime dependent thing, not a create time
	//edges use data from parent to prevent seaming issues
	{
	int i = 0, j = 0;
	double hL, hR, hD, hU;
	glm::vec3 normal;
	int vLoc;

	// i = 0, j[1,numCells - 1]
	for (int j = 1; j < numCells; j += 2) {
	int vLoc = (i*(numCells + 1) + j)* vertElementCount;

	//Use the left/right or up/down to find the appropriate height
	int neighborVLocA = (i*(numCells + 1) + j + 1)* vertElementCount;
	int neighborVLocB = (i*(numCells + 1) + j - 1)* vertElementCount;

	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1])/2.0;
	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
	(*verts)[vLoc + 6] = i;
	(*verts)[vLoc + 7] = j;
	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
	}

	// i = numCells, j[1,numCells - 1]
	i = numCells;
	for (int j = 1; j < numCells; j += 2) {
	int vLoc = (i*(numCells + 1) + j)* vertElementCount;

	//Use the left/right or up/down to find the appropriate height
	int neighborVLocA = (i*(numCells + 1) + j + 1)* vertElementCount;
	int neighborVLocB = (i*(numCells + 1) + j - 1)* vertElementCount;

	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
	(*verts)[vLoc + 6] = i;
	(*verts)[vLoc + 7] = j;
	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
	}

	// j = 0, i[1, numCells - 1]
	j = 0;
	for (int i = 1; i < numCells; i += 2) {
	int vLoc = (i*(numCells + 1) + j)* vertElementCount;

	//Use the left/right or up/down to find the appropriate height
	int neighborVLocA = ((i + 1)*(numCells + 1) + j)* vertElementCount;
	int neighborVLocB = ((i - 1)*(numCells + 1) + j)* vertElementCount;

	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
	(*verts)[vLoc + 6] = i;
	(*verts)[vLoc + 7] = j;
	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
	}

	// j = numCells, i[1, numCells - 1]
	j = numCells;
	for (int i = 1; i < numCells; i +=2) {
	int vLoc = (i*(numCells + 1) + j)* vertElementCount;

	//Use the left/right or up/down to find the appropriate height
	int neighborVLocA = ((i + 1)*(numCells + 1) + j)* vertElementCount;
	int neighborVLocB = ((i - 1)*(numCells + 1) + j)* vertElementCount;

	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
	(*verts)[vLoc + 6] = i;
	(*verts)[vLoc + 7] = j;
	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
	}
	}
	*/

	//Fills in lines starting at i = 1 and skips a line, filling in all the j's (half of the terrain)
	for (int i = 1; i < numCells; i += 2)
	{
		for (int j = 0; j <= numCells; j++)
		{
			float value = (terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));

			int vLoc = (i*(numCells + 1) + j)* vertElementCount;

			(*verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
			(*verts)[vLoc + 1] = (float)value * heightScale;
			(*verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
			(*verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosX / (float)(1 << terrainQuad.level);
			(*verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosZ / (float)(1 << terrainQuad.level);
			(*verts)[vLoc + 8] = terrainGenerator.SampleColor(0,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells  + (zLoc));
			(*verts)[vLoc + 9] = terrainGenerator.SampleColor(1,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells  + (zLoc));
			(*verts)[vLoc + 10] = terrainGenerator.SampleColor(2,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[vLoc + 11] = 1.0;
		}
	}

	//fills the last 1/4 of cells, starting at i= 0 and jumping. Like the first double for loop but offset by one
	for (int i = 0; i <= numCells; i += 2)
	{
		for (int j = 1; j <= numCells; j += 2)
		{
			float value = (terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));

			int vLoc = (i*(numCells + 1) + j)* vertElementCount;

			(*verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
			(*verts)[vLoc + 1] = (float)value * heightScale;
			(*verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
			(*verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosX / (1 << terrainQuad.level);
			(*verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPosZ / (1 << terrainQuad.level);
			(*verts)[vLoc + 8] = terrainGenerator.SampleColor(0,(float)i *(xSize) / (float)numCells  + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[vLoc + 9] = terrainGenerator.SampleColor(1,(float)i *(xSize) / (float)numCells  + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[vLoc + 10] = terrainGenerator.SampleColor(2,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
			(*verts)[vLoc + 11] = 1.0;
		}
	}

	//normals -- Look I know its verbose and proabably prone to being a waste of time, but since its something as banal as normal calculation and the optomizer doesn't know about how most of the center doesn't need to recalculate the heights, it feels like a useful thing to do
	{

		//center normals
		{
			for (int i = 1; i < numCells; i++)
			{
				for (int j = 1; j < numCells; j++)
				{
					//gets height values of its neighbors, rather than recalculating them from scratch
					double hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
					double hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
					double hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
					double hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1

					//double hUL = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // i + 1, j + 1
					//double hDR = (*verts)[((i - 1)*(numCells + 1) + j - 1)* vertElementCount + 1]; // i - 1, j -1
					glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

					int vLoc = (i*(numCells + 1) + j)* vertElementCount;
					(*verts)[vLoc + 3] = normal.x;
					(*verts)[vLoc + 4] = normal.y;
					(*verts)[vLoc + 5] = normal.z;
				}
			}
		}

		//edge normals
		{
			int i = 0, j = 0;
			double hL, hR, hD, hU;
			glm::vec3 normal;
			int vLoc;

			// i = 0, j[1,numCells - 1]
			for (int j = 1; j < numCells; j++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale;
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// i = numCells, j[1,numCells - 1]
			i = numCells;
			for (int j = 1; j < numCells; j++) {
				hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale;
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// j = 0, i[1, numCells - 1]
			j = 0;
			for (int i = 1; i < numCells; i++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
				hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale;//
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// j = numCells, i[1, numCells - 1]
			j = numCells;
			for (int i = 1; i < numCells; i++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
				hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale;
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}
		}

		//corner normals
		{
			double hL, hR, hD, hU;
			glm::vec3 normal;
			int vLoc;

			int i = 0, j = 0;
			hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
			hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
			hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
			hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;


			i = 0, j = numCells;
			hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
			hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
			hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
			hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;

			i = numCells, j = 0;
			hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
			hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
			hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
			hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;

			i = numCells, j = numCells;
			hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
			hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
			hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
			hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
			
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;

			j++;

			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			
			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
		}

		i++;

		for (int j = 0; j < numCells; j++)
		{
			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;

			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;
		
			j++;

			(*indices)[counter++] = i * (numCells + 1) + j;
			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;

			(*indices)[counter++] = i * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(*indices)[counter++] = (i + 1) * (numCells + 1) + j;

		}
	}
}