#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Logger.h"

TerrainQuad::TerrainQuad() {
	pos = glm::vec2(0);
	size = glm::vec2(0);
	level = 0;
	isSubdivided = false;
}

void TerrainQuad::init(glm::vec2 pos, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos, float centerHeightValue) 
{
	this->pos = pos;
	this->size = size;
	this->logicalPos = logicalPos;
	this->logicalSize = logicalSize;
	this->subDivPos = subDivPos;
	this->level = level;
	isSubdivided = false;
	heightValAtCenter = centerHeightValue;
	modelUniformObject.model = glm::translate(glm::mat4(), glm::vec3(pos.x, 0, pos.y));
	modelUniformObject.normal = glm::mat4();// glm::transpose(glm::inverse(glm::mat3(modelUniformObject.model))));

}

TerrainQuad::~TerrainQuad()
{
	subQuads.DownLeft.reset();
	subQuads.DownRight.reset();
	subQuads.UpLeft.reset();
	subQuads.UpRight.reset();
}

Terrain::Terrain(
	std::shared_ptr<MemoryPool<TerrainQuad>> pool,
	InternalGraph::GraphPrototype& protoGraph,
	int numCells, int maxLevels, float heightScale, 
	TerrainCoordinateData coords)
	
	: maxLevels(maxLevels), heightScale(heightScale), coordinateData(coords),
	fastGraphUser(protoGraph, 1337, coords.sourceImageResolution, coords.noisePos, coords.noiseSize.x)
	
{
	
	//simple calculation right now, does the absolute max number of quads possible with given max level
	//in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		//with current quad density this is the average upper bound (kidna a guess but its probably more than enough for now (had to add 25 cause it wasn't enough lol!)
		maxNumQuads = 1 + 16 + 20 + 25 + 50 * maxLevels; 
		//maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads (like if everything was subdivided)
	}
	//terrainQuads = new MemoryPool<TerrainQuad, 2 * sizeof(TerrainQuad)>();
	//terrainQuads = pool;
	quadHandles.reserve(maxNumQuads);
	//terrainGenerationWorkers = std::vector < std::thread>(maxNumQuads);

	//fastTerrainUser = std::make_unique<NewNodeGraph::TerGenGraphUser>(sourceGraph, 1337, sourceImageResolution, noisePosition, noiseSize.x);

	//fastTerrainGraph = std::make_shared<NewNodeGraph::TerGenNodeGraph> (1337, sourceImageResolution, noisePosition, noiseSize.x);
	//fastTerrainGraph->BuildNoiseGraph();

	//splatmapTextureGradient.SetFrontColor(glm::vec4(1, 0, 0, 0));
	
	//splatmapTextureGradient.AddControlPoint(0.5f, glm::vec4(1, 0, 0, 0));
	//splatmapTextureGradient.AddControlPoint(0.6f, glm::vec4(0, 1, 0, 0));
	//splatmapTextureGradient.AddControlPoint(0.65f, glm::vec4(0, 1, 0, 0));
	//splatmapTextureGradient.AddControlPoint(0.7f, glm::vec4(0, 0, 1, 0));
	//splatmapTextureGradient.AddControlPoint(0.8f, glm::vec4(0, 0, 1, 0));

	//splatmapTextureGradient.SetBackColor(glm::vec4(0, 0, 0, 1));

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());
	
}


Terrain::~Terrain() {
	//Log::Debug << "terrain deleted\n";
	CleanUp();

}

void Terrain::CleanUp()
{
	//cleanup model buffers
	vertexBuffer.CleanBuffer(renderer->device);
	indexBuffer.CleanBuffer(renderer->device);

	terrainVulkanSplatMap.destroy(renderer->device);
	//terrainVulkanTextureArray.destroy(renderer->device);

	modelUniformBuffer.CleanBuffer(renderer->device);

	//vkDestroyDescriptorSetLayout(renderer->device.device, descriptorSetLayout, nullptr);
	//vkDestroyDescriptorPool(renderer->device.device, descriptorPool, nullptr);
}


void Terrain::InitTerrain(std::shared_ptr<VulkanRenderer> renderer, glm::vec3 cameraPos, VulkanTexture2DArray* terrainVulkanTextureArray)
{
	this->renderer = renderer;
	this->terrainVulkanTextureArray = terrainVulkanTextureArray;

	SetupMeshbuffers();
	SetupUniformBuffer();
	SetupImage();
	SetupModel();
	SetupDescriptorLayoutAndPool();
	SetupPipeline();

	std::shared_ptr<TerrainQuad> q = std::make_shared<TerrainQuad>();
	quadHandles.push_back(q);
	rootQuad = InitTerrainQuad(q, Corner_Enum::uR, coordinateData.pos, coordinateData.size, coordinateData.noisePos, coordinateData.noiseSize, 0, glm::i32vec2(0,0));

	UpdateTerrainQuad(rootQuad, cameraPos);

	UpdateModelBuffer();
	UpdateMeshBuffer();
}

void Terrain::UpdateTerrain(glm::vec3 viewerPos) {
	SimpleTimer updateTime;
	updateTime.StartTimer();
		
	bool shouldUpdateBuffers = UpdateTerrainQuad(rootQuad, viewerPos);
	
	
	if (shouldUpdateBuffers) {
		UpdateModelBuffer();
		UpdateMeshBuffer();
	}
	PrevQuadHandles = quadHandles;

	updateTime.EndTimer();
	
	//if (updateTime.GetElapsedTimeMicroSeconds() > 1500)
	//	Log::Debug << " Update time " << updateTime.GetElapsedTimeMicroSeconds() << "\n";
}

bool Terrain::UpdateTerrainQuad(std::shared_ptr<TerrainQuad> quad, glm::vec3 viewerPos) {
	
	glm::vec3 center = glm::vec3(quad->pos.x + quad->size.x / 2.0f, quad->heightValAtCenter, quad->pos.y + quad->size.y / 2.0f);
	float distanceToViewer = glm::distance(viewerPos, center);
	bool shouldUpdateBuffers = false;


	if (!quad->isSubdivided) { //can only subdivide if this quad isn't already subdivided
		if (distanceToViewer < quad->size.x * 2.0f && quad->level < maxLevels) { //must be 
			SubdivideTerrain(quad, viewerPos);
			shouldUpdateBuffers = true;
		} 
	}

	else {
		bool uL = false, uR = false, dL = false, dR = false;

		uL = UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos);
		uR = UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos);
		dL = UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos);
		dR = UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos);

		if (uL || uR || dL || dR)
			shouldUpdateBuffers = true;
	} 

	if (distanceToViewer > quad->size.x * 2.0f) {
		UnSubdivide(quad);
		shouldUpdateBuffers = true;
	}
	
	return shouldUpdateBuffers;
}

std::vector<RGBA_pixel>*  Terrain::LoadSplatMapFromGenerator() {
	return fastGraphUser.GetSplatMap().GetImageVectorData();

	//InternalGraph::NoiseImage2D<float> pixData = fastGraphUser.GetHeightMap();
	//std::vector<RGBA_pixel>* imageData = new std::vector<RGBA_pixel>(sourceImageResolution * sourceImageResolution);
	//
	//if (imageData == nullptr) {
	//	Log::Error << "failed to create splatmap image" << "\n";
	//	return nullptr;
	//}
	//for (int i = 0; i < pixData.GetImageWidth(); i++) {
	//	for (int j = 0; j < pixData.GetImageWidth(); j++) {
	//		glm::vec4 col = splatmapTextureGradient.SampleGradient(((pixData.GetImageData()[i * pixData.GetImageWidth() + (pixData.GetImageWidth() - j - 1)]) + 1.0f) / 2.0f);
	//		(*imageData)[(pixData.GetImageWidth() - j - 1) * pixData.GetImageWidth() + i] = RGBA_pixel((stbi_uc)(col.x * 255), (stbi_uc)(col.y * 255), (stbi_uc)(col.z * 255), (stbi_uc)(col.w * 255));
	//	}
	//}
	////for (int i = 0; i < sourceImageResolution * sourceImageResolution; i++ ) {
	////	imageData[i] = RGBA_pixel((stbi_uc)(col.x * 255), (stbi_uc)(col.y * 255), (stbi_uc)(col.z * 255), (stbi_uc)(col.w * 255));
	////}
	//
	//return imageData;
}

void Terrain::SetupMeshbuffers() {
	uint32_t vBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshIndices);

	// Create device local target buffers
	// Vertex buffer
	vertexBuffer.CreateVertexBuffer(renderer->device, maxNumQuads * vertCount);
	indexBuffer.CreateIndexBuffer(renderer->device, maxNumQuads * indCount);

	//VK_CHECK_RESULT(renderer->device.createBuffer(
	//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	&vertexBuffer,
	//	vBufferSize));
	//
	//// Index buffer
	//VK_CHECK_RESULT(renderer->device.createBuffer(
	//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	&indexBuffer,
	//	iBufferSize));
}

void Terrain::SetupUniformBuffer()
{
	modelUniformBuffer.CreateUniformBuffer(renderer->device, sizeof(ModelBufferObject) * maxNumQuads);
	//renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	//	&modelUniformBuffer, sizeof(ModelBufferObject) * maxNumQuads);
}

void Terrain::SetupImage() 
{
	if (terrainSplatMap != nullptr) {
		terrainVulkanSplatMap.loadFromTexture(renderer->device, terrainSplatMap, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	
	}
	else {
		throw std::runtime_error("failed to load terrain splat map!");

	}
}

void Terrain::SetupModel() 
{
	//terrainModel.loadFromMesh(terrainMesh, device, device->graphics_queue);


}

void Terrain::SetupDescriptorLayoutAndPool()
{
	descriptor = renderer->GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	descriptor->SetupPool(poolSizes, maxNumQuads);

}

void Terrain::SetupPipeline()
{
	VulkanPipeline &pipeMan = renderer->pipelineManager;
	mvp = pipeMan.CreateManagedPipeline();

	pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/terrain.vert.spv"));
	pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/terrain.frag.spv"));
	pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
	pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width, (float) renderer->vulkanSwapChain.swapChainExtent.height, 0.0f, 1.0f, 0.0f, 0.0f);
	pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width, renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
	pipeMan.SetViewportState(mvp, 1, 1, 0);
	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
	pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER, VK_FALSE, VK_FALSE);
	pipeMan.SetColorBlendingAttachment(mvp, VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
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

	pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

	pipeMan.CleanShaderResources(mvp);
	
	pipeMan.SetVertexShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.vert.spv"));
	pipeMan.SetFragmentShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.frag.spv"));
	pipeMan.SetGeometryShader(mvp, loadShaderModule(renderer->device.device, "assets/shaders/normalVecDebug.geom.spv"));
	
	pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);
	pipeMan.CleanShaderResources(mvp);

	
}

void Terrain::UpdateModelBuffer() {
	//VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {

		//vkFreeDescriptorSets(renderer->device.device, descriptorPool, 1, &(*it)->descriptorSet);
		descriptor->FreeDescriptorSet((*it)->descriptorSet);

		//if (vkAllocateDescriptorSets(renderer->device.device, &allocInfoTerrain, &(*it)->descriptorSet) != VK_SUCCESS) {
		//	throw std::runtime_error("failed to allocate descriptor set!");
		//}

		modelUniformBuffer.resource.FillResource(modelUniformBuffer.buffer.buffer, (it - quadHandles.begin()) * sizeof(ModelBufferObject), sizeof(ModelBufferObject));
		(*it)->descriptorSet = descriptor->CreateDescriptorSet();

		std::vector<DescriptorUse> writes;
		writes.push_back(DescriptorUse(2, 1, modelUniformBuffer.resource));
		writes.push_back(DescriptorUse(3, 1, terrainVulkanSplatMap.resource));
		writes.push_back(DescriptorUse(4, 1, terrainVulkanTextureArray->resource));
		descriptor->UpdateDescriptorSet((*it)->descriptorSet, writes);

	}

	std::vector<ModelBufferObject> modelObjects;
	modelObjects.reserve(quadHandles.size());

	for (auto it = quadHandles.begin(); it != quadHandles.end(); it++) {
		modelObjects.push_back((*it)->modelUniformObject);
	}

	modelUniformBuffer.CopyToBuffer(renderer->device, modelObjects.data(), modelObjects.size() * sizeof(ModelBufferObject));
}

void Terrain::UpdateMeshBuffer() {

	while (terrainGenerationWorkers.size() > 0) {
		terrainGenerationWorkers.front()->join();
		terrainGenerationWorkers.front() = std::move(terrainGenerationWorkers.back());
		terrainGenerationWorkers.pop_back();
	}

	SimpleTimer cpuDataTime;

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

				VkBufferCopy vBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices));
				vertexCopyRegions.push_back(vBufferRegion);

				VkBufferCopy iBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices));
				indexCopyRegions.push_back(iBufferRegion);
			}
		}
		for (uint32_t i = (uint32_t)PrevQuadHandles.size(); i < quadHandles.size(); i++) {
			verts[i] = quadHandles[i]->vertices;
			inds[i] = quadHandles[i]->indices;

			VkBufferCopy vBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices));
			vertexCopyRegions.push_back(vBufferRegion);

			VkBufferCopy iBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices));
			indexCopyRegions.push_back(iBufferRegion);
		}
	}
	else { //less meshes than before, can erase at end.
		for (int i = 0; i < quadHandles.size(); i++) {
			if (quadHandles[i] != PrevQuadHandles[i]) {
				verts[i] = quadHandles[i]->vertices;
				inds[i] = quadHandles[i]->indices;

				VkBufferCopy vBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices));
				vertexCopyRegions.push_back(vBufferRegion);

				VkBufferCopy iBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices));
				indexCopyRegions.push_back(iBufferRegion);
			}
		}
	}
	cpuDataTime.EndTimer();
	SimpleTimer gpuTransferTime;

	if (vertexCopyRegions.size() > 0 || indexCopyRegions.size() > 0)
	{
		VulkanBufferVertex vertexStaging;
		VulkanBufferIndex indexStaging;

		vertexStaging.CreateStagingVertexBuffer(renderer->device, verts.data(), verts.size() * vertCount);
		indexStaging.CreateStagingIndexBuffer(renderer->device, inds.data(), inds.size() * indCount);

		VkCommandBuffer copyCmd = renderer->device.GetTransferCommandBuffer();

		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer.buffer, vertexBuffer.buffer.buffer, (uint32_t)vertexCopyRegions.size(), vertexCopyRegions.data());

		//copyRegion.size = indexBuffer.size;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer.buffer, indexBuffer.buffer.buffer, (uint32_t)indexCopyRegions.size(), indexCopyRegions.data());

		renderer->device.SubmitTransferCommandBuffer();

		vertexStaging.CleanBuffer(renderer->device);
		indexStaging.CleanBuffer(renderer->device);

		//renderer->device.DestroyVmaAllocatedBuffer(&vmaStagingBufVertex, &vmaStagingVertices);
		//renderer->device.DestroyVmaAllocatedBuffer(&vmaStagingBufIndex, &vmaStagingIndecies);

	}
	gpuTransferTime.EndTimer();

	//Log::Debug << "Create copy command: " << cpuDataTime.GetElapsedTimeMicroSeconds() << "\n";
	//Log::Debug << "Execute buffer copies: " << gpuTransferTime.GetElapsedTimeMicroSeconds() << "\n";
}

std::shared_ptr<TerrainQuad> Terrain::InitTerrainQuad( std::shared_ptr<TerrainQuad> quad, 
	Corner_Enum corner, glm::vec2 position, glm::vec2 size,
	glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos) {
	
	quad->init(position, size, logicalPos, logicalSize, level, subDivPos, 0);

	//SimpleTimer terrainQuadCreateTime;

	std::thread* worker = new std::thread(GenerateTerrainChunk, std::ref(fastGraphUser), quad, corner, heightScale, maxLevels);
	terrainGenerationWorkers.push_back(worker);

	//GenerateNewTerrainSubdivision(fastGraphUser, quad->vertices, quad->indices, quad->terrainQuad, corner, heightScale, maxLevels);
	
	//terrainQuadCreateTime.EndTimer();
	//Log::Debug << "From Parent " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << "\n";
	//std::vector<VkDescriptorSetLayout> layouts;
	//layouts.resize(maxNumQuads);
	//std::fill(layouts.begin(), layouts.end(), descriptorSetLayout);

	modelUniformBuffer.resource.FillResource(modelUniformBuffer.buffer.buffer,
		(VkDeviceSize)((quadHandles.size() - 1) * sizeof(ModelBufferObject)), (VkDeviceSize)sizeof(ModelBufferObject));

	quad->descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(2, 1, modelUniformBuffer.resource));
	writes.push_back(DescriptorUse(3, 1, terrainVulkanSplatMap.resource));
	writes.push_back(DescriptorUse(4, 1, terrainVulkanTextureArray->resource));
	descriptor->UpdateDescriptorSet(quad->descriptorSet, writes);
	return quad;
}

void Terrain::SubdivideTerrain(std::shared_ptr<TerrainQuad> quad, glm::vec3 viewerPos) {
	quad->isSubdivided = true;
	numQuads += 4;

	glm::vec2 new_pos = glm::vec2(quad->pos.x, quad->pos.y);
	glm::vec2 new_size = glm::vec2(quad->size.x/2.0, quad->size.y/2.0);
	
	glm::i32vec2 new_lpos = glm::i32vec2(quad->logicalPos.x, quad->logicalPos.y);
	glm::i32vec2 new_lsize = glm::i32vec2(quad->logicalSize.x / 2.0, quad->logicalSize.y / 2.0);

	auto qUR = std::make_shared<TerrainQuad>();
	auto qUL = std::make_shared<TerrainQuad>();
	auto qDR = std::make_shared<TerrainQuad>();
	auto qDL = std::make_shared<TerrainQuad>();
	quadHandles.push_back(qUR);												 
	quadHandles.push_back(qUL);
	quadHandles.push_back(qDR);
	quadHandles.push_back(qDL);

	quad->subQuads.UpRight = InitTerrainQuad(qUR, Corner_Enum::uR, glm::vec2(new_pos.x, new_pos.y), new_size, 
		glm::i32vec2(new_lpos.x, new_lpos.y), new_lsize, quad->level + 1,  glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2));

	quad->subQuads.UpLeft = InitTerrainQuad(qUL, Corner_Enum::uL, glm::vec2(new_pos.x, new_pos.y + new_size.y), new_size, 
		glm::i32vec2(new_lpos.x, new_lpos.y + new_lsize.y), new_lsize, quad->level + 1, glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2 + 1));

	quad->subQuads.DownRight = InitTerrainQuad(qDR, Corner_Enum::dR, glm::vec2(new_pos.x + new_size.x, new_pos.y), new_size, 
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y), new_lsize, quad->level + 1, glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2));

	quad->subQuads.DownLeft = InitTerrainQuad(qDL, Corner_Enum::dL, glm::vec2(new_pos.x + new_size.x, new_pos.y + new_size.y), new_size, 
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y), new_lsize, quad->level + 1,  glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2 + 1));

	UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos);
	UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos);
	UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos);
	UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos);

	//Log::Debug << "Terrain subdivided: Level: " << quad->level << " Position: " << quad->pos.x << ", " <<quad->pos.z << " Size: " << quad->size.x << ", " << quad->size.z << "\n";

	
}

void Terrain::UnSubdivide(std::shared_ptr<TerrainQuad> quad) {
	if (quad->isSubdivided) {

		auto delUR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpRight);
		if (delUR != quadHandles.end()) {
			delUR->reset();
			quadHandles.erase(delUR);
			descriptor->FreeDescriptorSet(quad->subQuads.UpRight->descriptorSet);
			numQuads--;
		}

		auto delDR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownRight);
		if (delDR != quadHandles.end()) {
			delDR->reset();
			quadHandles.erase(delDR);
			descriptor->FreeDescriptorSet(quad->subQuads.DownRight->descriptorSet);
			numQuads--;
		}

		auto delUL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpLeft);
		if (delUL != quadHandles.end()) {
			delUL->reset();
			quadHandles.erase(delUL);
			descriptor->FreeDescriptorSet(quad->subQuads.UpLeft->descriptorSet);
			numQuads--;
		}

		auto delDL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownLeft);
		if (delDL != quadHandles.end()) {
			delDL->reset();
			quadHandles.erase(delDL);
			descriptor->FreeDescriptorSet(quad->subQuads.DownLeft->descriptorSet);
			numQuads--;
		}

		quad->isSubdivided = false;
	}
	//quad->isSubdivided = false;
	//Log::Debug << "Terrain un-subdivided: Level: " << quad->level << " Position: " << quad->pos.x << ", " << quad->pos.z << " Size: " << quad->size.x << ", " << quad->size.z << "\n";
}

void Terrain::RecursiveUnSubdivide(std::shared_ptr<TerrainQuad> quad) {
	if (quad->isSubdivided) {
		RecursiveUnSubdivide(quad->subQuads.DownLeft);
		RecursiveUnSubdivide(quad->subQuads.DownRight);
		RecursiveUnSubdivide(quad->subQuads.UpLeft);
		RecursiveUnSubdivide(quad->subQuads.UpRight);
	}
	UnSubdivide(quad);
}

void Terrain::UpdateUniformBuffer(float time)
{

}


void Terrain::DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], bool ifWireframe) {

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	
	std::vector<VkDeviceSize> vertexOffsettings(quadHandles.size());
	std::vector<VkDeviceSize> indexOffsettings(quadHandles.size());
	
	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings[i] = (i * sizeof(TerrainMeshVertices));
		indexOffsettings[i] = (i * sizeof(TerrainMeshIndices));
	}
	

	drawTimer.StartTimer();
	for (int i = 0; i < quadHandles.size(); ++i) {
		if (!quadHandles[i]->isSubdivided) {

			vkCmdPushConstants(
				cmdBuff,
				mvp->layout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(ModelPushConstant),
				&quadHandles[i]->modelUniformObject);

			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &quadHandles[i]->descriptorSet.set, 0, nullptr);
			vkCmdBindVertexBuffers(cmdBuff, 0, 1, &vertexBuffer.buffer.buffer, &vertexOffsettings[i]);
			vkCmdBindIndexBuffer(cmdBuff, indexBuffer.buffer.buffer, indexOffsettings[i], VK_INDEX_TYPE_UINT32);


			vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(indCount), 1, 0, 0, 0);

			//Vertex normals (yay geometry shaders!)
			//vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->pipelines->at(2));
			//vkCmdDrawIndexed(cmdBuff, static_cast<uint32_t>(indCount), 1, 0, 0, 0);
		}	
	}
	drawTimer.EndTimer();

	

}

float Terrain::GetHeightAtLocation(float x, float z) {

	return fastGraphUser.SampleHeightMap(x, z) * heightScale;
}

glm::vec3 CalcNormal(double L, double R, double U, double D, double UL, double DL, double UR, double DR, double vertexDistance, int numCells) {

	return glm::normalize(glm::vec3(L + UL + DL - (R + UR + DR), 2 * vertexDistance / numCells, U + UL + UR - (D + DL + DR)));
}


void RecalculateNormals(int numCells, TerrainMeshVertices& verts, TerrainMeshIndices& indices) {
	int index = 0;
	for (int i = 0; i < indCount / 3; i++) {
		glm::vec3 p1 = glm::vec3((verts)[(indices)[i * 3 + 0] * vertElementCount + 0], (verts)[(indices)[i * 3 + 0] * vertElementCount + 1], (verts)[(indices)[i * 3 + 0] * vertElementCount + 2]);
		glm::vec3 p2 = glm::vec3((verts)[(indices)[i * 3 + 1] * vertElementCount + 0], (verts)[(indices)[i * 3 + 1] * vertElementCount + 1], (verts)[(indices)[i * 3 + 1] * vertElementCount + 2]);
		glm::vec3 p3 = glm::vec3((verts)[(indices)[i * 3 + 2] * vertElementCount + 0], (verts)[(indices)[i * 3 + 2] * vertElementCount + 1], (verts)[(indices)[i * 3 + 2] * vertElementCount + 2]);

		glm::vec3 t1 = p2 - p1;
		glm::vec3 t2 = p3 - p1;

		glm::vec3 normal(glm::cross(t1, t2));

		(verts)[(indices)[i * 3 + 0] * vertElementCount + 3] += normal.x; (verts)[(indices)[i * 3 + 0] * vertElementCount + 4] += normal.y; (verts)[(indices)[i * 3 + 0] * vertElementCount + 5] += normal.z;
		(verts)[(indices)[i * 3 + 1] * vertElementCount + 3] += normal.x; (verts)[(indices)[i * 3 + 1] * vertElementCount + 4] += normal.y; (verts)[(indices)[i * 3 + 1] * vertElementCount + 5] += normal.z;
		(verts)[(indices)[i * 3 + 2] * vertElementCount + 3] += normal.x; (verts)[(indices)[i * 3 + 2] * vertElementCount + 4] += normal.y; (verts)[(indices)[i * 3 + 2] * vertElementCount + 5] += normal.z;
	}

	for (int i = 0; i < (numCells + 1) * (numCells + 1); i++) {
		glm::vec3 normal = glm::normalize(glm::vec3((verts)[i * vertElementCount + 3], (verts)[i * vertElementCount + 4], (verts)[i * vertElementCount + 5]));
		(verts)[i * vertElementCount + 3] = normal.x;
		(verts)[i * vertElementCount + 4] = normal.y;
		(verts)[i * vertElementCount + 5] = normal.z;
	}
}

void GenerateTerrainChunk(InternalGraph::GraphUser& graphUser, std::shared_ptr<TerrainQuad> quad,
	Corner_Enum corner, float heightScale, int maxSubDivLevels) {

	TerrainMeshVertices& verts = quad->vertices;
	TerrainMeshIndices& indices = quad->indices;

	int numCells = NumCells;
	float xLoc = quad->pos.x, zLoc = quad->pos.y, xSize = quad->size.x, zSize = quad->size.y;

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			float uvU = (float)i / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level);
			float uvV = (float)j / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level);

			float uvUminus = glm::clamp((float)(i - 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level), 0.0f , 1.0f);
			float uvUplus = glm::clamp((float)(i + 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level), 0.0f, 1.0f);
			float uvVminus = glm::clamp((float)(j - 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level), 0.0f, 1.0f);
			float uvVplus = glm::clamp((float)(j + 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level), 0.0f, 1.0f);

			float outHeight = graphUser.SampleHeightMap(uvU, uvV);
			float outHeightUM = graphUser.SampleHeightMap(uvUminus, uvV);
			float outHeightUP = graphUser.SampleHeightMap(uvUplus, uvV) ;
			float outHeightVM = graphUser.SampleHeightMap(uvU, uvVminus);
			float outHeightVP = graphUser.SampleHeightMap(uvU, uvVplus) ;

			glm::vec3 normal = glm::normalize(glm::vec3((outHeightUM - outHeightUP),
				((uvUplus - uvUminus) + (uvVplus - uvVminus))*2,
				(outHeightVM - outHeightVP)));

			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = (float)i *(xSize) / (float)numCells;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = outHeight * heightScale;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = (float)j * (zSize) / (float)numCells;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = uvU;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = uvV;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = 0;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = 0;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = 0;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = 0;
			//Log::Debug << value << "\n";
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			(indices)[counter++] = i * (numCells + 1) + j;
			(indices)[counter++] = i * (numCells + 1) + j + 1;
			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
			(indices)[counter++] = i * (numCells + 1) + j + 1;
			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
		}
	}

	//RecalculateNormals(numCells, verts, indices);
	quad->isFinishedGeneratingTerrain = true;
}