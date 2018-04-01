#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Logger.h"

#include "../rendering/RendererStructs.h"

TerrainQuad::TerrainQuad(glm::vec2 pos, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, 
	int level, glm::i32vec2 subDivPos, float centerHeightValue):

	pos(pos), size(size), 
	logicalPos(logicalPos), logicalSize(logicalSize), subDivPos(subDivPos), isSubdivided(false),
	heightValAtCenter(0)
{
	isReady = std::make_shared<bool>();
	*isReady = false;
}

//void TerrainQuad::init(glm::vec2 pos, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos, float centerHeightValue) 
//{
//	this->pos = pos;
//	this->size = size;
//	this->logicalPos = logicalPos;
//	this->logicalSize = logicalSize;
//	this->subDivPos = subDivPos;
//	this->level = level;
//	isSubdivided = false;
//	heightValAtCenter = centerHeightValue;
//	modelUniformObject.model = glm::translate(glm::mat4(), glm::vec3(pos.x, 0, pos.y));
//	modelUniformObject.normal = glm::mat4();// glm::transpose(glm::inverse(glm::mat3(modelUniformObject.model))));
//
//}

TerrainQuad::~TerrainQuad()
{
	subQuads.DownLeft.reset();
	subQuads.DownRight.reset();
	subQuads.UpLeft.reset();
	subQuads.UpRight.reset();
}

float TerrainQuad::GetUVvalueFromLocalIndex(float i, int numCells, int level, int subDivPos) {
	return glm::clamp((float)(i) / ((1 << level) * (float)(numCells)) + (float)(subDivPos) / (float)(1 << level), 0.0f, 1.0f);
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

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());
	
	rootQuad = std::make_shared<TerrainQuad>(
		coordinateData.pos, coordinateData.size, coordinateData.noisePos, coordinateData.noiseSize,
		0, glm::i32vec2(0, 0),
		GetHeightAtLocation(TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, 0, 0),
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, 0, 0)));
	quadHandles.push_back(rootQuad);

	modelMatrixData.model = glm::translate(glm::mat4(), glm::vec3(coordinateData.pos.x, 0, coordinateData.pos.y));
}


Terrain::~Terrain() {
	CleanUp();
}

void Terrain::CleanUp()
{
	//cleanup model buffers
	vertexBuffer.CleanBuffer(renderer->device);
	indexBuffer.CleanBuffer(renderer->device);

	terrainVulkanSplatMap.destroy(renderer->device);
}


void Terrain::InitTerrain(std::shared_ptr<VulkanRenderer> renderer, glm::vec3 cameraPos, VulkanTexture2DArray* terrainVulkanTextureArray)
{
	this->renderer = renderer;

	SetupMeshbuffers();
	SetupUniformBuffer();
	SetupImage(renderer->device.GetTransferCommandBuffer());
	SetupDescriptorSets(terrainVulkanTextureArray);
	SetupPipeline();

	InitTerrainQuad(rootQuad, Corner_Enum::uR, cameraPos);

	UpdateMeshBuffer();
}

void Terrain::UpdateTerrain(glm::vec3 viewerPos) {
	SimpleTimer updateTime;
	updateTime.StartTimer();
		
	bool shouldUpdateBuffers = UpdateTerrainQuad(rootQuad, viewerPos);
	
	if (shouldUpdateBuffers) 
		UpdateMeshBuffer();
	
	PrevQuadHandles = quadHandles;

	updateTime.EndTimer();
	
	//if (updateTime.GetElapsedTimeMicroSeconds() > 1500)
	//	Log::Debug << " Update time " << updateTime.GetElapsedTimeMicroSeconds() << "\n";
}

std::vector<RGBA_pixel>*  Terrain::LoadSplatMapFromGenerator() {
	return fastGraphUser.GetSplatMap().GetImageVectorData();
}

void Terrain::SetupMeshbuffers() {
	uint32_t vBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshVertices);
	uint32_t iBufferSize = static_cast<uint32_t>(maxNumQuads) * sizeof(TerrainMeshIndices);

	// Create device local target buffers
	// Vertex buffer
	vertexBuffer.CreateVertexBuffer(renderer->device, maxNumQuads * vertCount);
	indexBuffer.CreateIndexBuffer(renderer->device, maxNumQuads * indCount);
}

void Terrain::SetupUniformBuffer()
{
	//modelUniformBuffer.CreateUniformBuffer(renderer->device, sizeof(ModelBufferObject));
	//renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	//	&modelUniformBuffer, sizeof(ModelBufferObject) * maxNumQuads);
}

void Terrain::SetupImage(VkCommandBuffer cmdBuf)
{
	if (terrainSplatMap != nullptr) {
		terrainVulkanSplatMap.loadFromTexture(renderer->device, terrainSplatMap, VK_FORMAT_R8G8B8A8_UNORM, cmdBuf,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	
	}
	else {
		throw std::runtime_error("failed to load terrain splat map!");

	}
}

void Terrain::SetupDescriptorSets(VulkanTexture2DArray* terrainVulkanTextureArray)
{
	descriptor = renderer->GetVulkanDescriptor();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	m_bindings.push_back(VulkanDescriptor::CreateBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1));
	descriptor->SetupLayout(m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	poolSizes.push_back(DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	descriptor->SetupPool(poolSizes, maxNumQuads);

	//VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	
	writes.push_back(DescriptorUse(3, 1, terrainVulkanSplatMap.resource));
	writes.push_back(DescriptorUse(4, 1, terrainVulkanTextureArray->resource));
	descriptor->UpdateDescriptorSet(descriptorSet, writes);

	
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
	pushConstantRange.size = sizeof(TerrainPushConstant);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipeMan.SetModelPushConstant(mvp, pushConstantRange);

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

	//std::vector<ReadyFlag> flags;
	//flags.reserve(quadHandles.size());

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
				//flags.push_back(quadHandles[i]->isReady);
			}
		}
		for (uint32_t i = (uint32_t)PrevQuadHandles.size(); i < quadHandles.size(); i++) {
			verts[i] = quadHandles[i]->vertices;
			inds[i] = quadHandles[i]->indices;

			VkBufferCopy vBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices), i * sizeof(TerrainMeshVertices));
			vertexCopyRegions.push_back(vBufferRegion);

			VkBufferCopy iBufferRegion = initializers::bufferCopyCreate(sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices), i * sizeof(TerrainMeshIndices));
			indexCopyRegions.push_back(iBufferRegion);
			//flags.push_back(quadHandles[i]->isReady);
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
				//flags.push_back(quadHandles[i]->isReady);
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

		renderer->device.SubmitTransferCommandBufferAndWait(copyCmd);

		vertexStaging.CleanBuffer(renderer->device);
		indexStaging.CleanBuffer(renderer->device);

		//renderer->device.DestroyVmaAllocatedBuffer(&vmaStagingBufVertex, &vmaStagingVertices);
		//renderer->device.DestroyVmaAllocatedBuffer(&vmaStagingBufIndex, &vmaStagingIndecies);

	}
	gpuTransferTime.EndTimer();

	//Log::Debug << "Create copy command: " << cpuDataTime.GetElapsedTimeMicroSeconds() << "\n";
	//Log::Debug << "Execute buffer copies: " << gpuTransferTime.GetElapsedTimeMicroSeconds() << "\n";
}

void Terrain::InitTerrainQuad( std::shared_ptr<TerrainQuad> quad, 
	Corner_Enum corner, glm::vec3 viewerPos) {
	
	//SimpleTimer terrainQuadCreateTime;

	std::thread* worker = new std::thread(GenerateTerrainChunk, std::ref(fastGraphUser), quad, corner, heightScale, coordinateData.size.x, maxLevels);
	terrainGenerationWorkers.push_back(worker);

	//terrainQuadCreateTime.EndTimer();
	//Log::Debug << "From Parent " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << "\n";

	UpdateTerrainQuad(quad, viewerPos);
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
		bool uR = UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos);
		bool uL = UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos);
		bool dR = UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos);
		bool dL = UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos);
		
		if(uR || uL || dR || dL)
			shouldUpdateBuffers = true;
	}

	if (distanceToViewer > quad->size.x * 2.0f) {
		UnSubdivide(quad);
		shouldUpdateBuffers = true;
	}

	return shouldUpdateBuffers;
}

void Terrain::SubdivideTerrain(std::shared_ptr<TerrainQuad> quad, glm::vec3 viewerPos) {
	quad->isSubdivided = true;
	numQuads += 4;

	glm::vec2 new_pos = glm::vec2(quad->pos.x, quad->pos.y);
	glm::vec2 new_size = glm::vec2(quad->size.x/2.0, quad->size.y/2.0);
	
	glm::i32vec2 new_lpos = glm::i32vec2(quad->logicalPos.x, quad->logicalPos.y);
	glm::i32vec2 new_lsize = glm::i32vec2(quad->logicalSize.x / 2.0, quad->logicalSize.y / 2.0);

	quad->subQuads.UpRight = std::make_shared<TerrainQuad>(
		glm::vec2(new_pos.x, new_pos.y), 
		new_size,
		glm::i32vec2(new_lpos.x, new_lpos.y), 
		new_lsize, 
		quad->level + 1, 
		glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2), 
		GetHeightAtLocation(
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.x * 2), 
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.y * 2)));

	quad->subQuads.UpLeft = std::make_shared<TerrainQuad>(
		glm::vec2(new_pos.x, new_pos.y + new_size.y), 
		new_size,
		glm::i32vec2(new_lpos.x, new_lpos.y + new_lsize.y), 
		new_lsize, 
		quad->level + 1, 
		glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2 + 1), 
		GetHeightAtLocation(
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.x * 2),
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.y * 2 + 1)));

	quad->subQuads.DownRight = std::make_shared<TerrainQuad>(
		glm::vec2(new_pos.x + new_size.x, new_pos.y), new_size,
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y), 
		new_lsize, 
		quad->level + 1, 
		glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2), 
		GetHeightAtLocation(
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.x * 2 + 1),
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.y * 2)));
	
	quad->subQuads.DownLeft = std::make_shared<TerrainQuad>(
		glm::vec2(new_pos.x + new_size.x, new_pos.y + new_size.y), 
		new_size,
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y),
		new_lsize,
		quad->level + 1,
		glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2 + 1),
		GetHeightAtLocation(
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.x * 2 + 1),
			TerrainQuad::GetUVvalueFromLocalIndex(NumCells / 2, NumCells, quad->level + 1, quad->subDivPos.y * 2 + 1)));

	quadHandles.push_back(quad->subQuads.UpRight);
	quadHandles.push_back(quad->subQuads.UpLeft);
	quadHandles.push_back(quad->subQuads.DownRight);
	quadHandles.push_back(quad->subQuads.DownLeft);

	InitTerrainQuad(quad->subQuads.UpRight, Corner_Enum::uR, viewerPos);
	InitTerrainQuad(quad->subQuads.UpLeft, Corner_Enum::uL, viewerPos);
	InitTerrainQuad(quad->subQuads.DownRight, Corner_Enum::dR, viewerPos);
	InitTerrainQuad(quad->subQuads.DownLeft, Corner_Enum::dL, viewerPos);

	//quad->subQuads.UpRight = InitTerrainQuad(qUR, Corner_Enum::uR, glm::vec2(new_pos.x, new_pos.y), new_size, 
	//	glm::i32vec2(new_lpos.x, new_lpos.y), new_lsize, quad->level + 1,  glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2));

	//quad->subQuads.UpLeft = InitTerrainQuad(qUL, Corner_Enum::uL, glm::vec2(new_pos.x, new_pos.y + new_size.y), new_size, 
	//	glm::i32vec2(new_lpos.x, new_lpos.y + new_lsize.y), new_lsize, quad->level + 1, glm::i32vec2(quad->subDivPos.x * 2, quad->subDivPos.y * 2 + 1));

	//quad->subQuads.DownRight = InitTerrainQuad(qDR, Corner_Enum::dR, glm::vec2(new_pos.x + new_size.x, new_pos.y), new_size, 
	//	glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y), new_lsize, quad->level + 1, glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2));

	//quad->subQuads.DownLeft = InitTerrainQuad(qDL, Corner_Enum::dL, glm::vec2(new_pos.x + new_size.x, new_pos.y + new_size.y), new_size, 
	//	glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y), new_lsize, quad->level + 1,  glm::i32vec2(quad->subDivPos.x * 2 + 1, quad->subDivPos.y * 2 + 1));

	//Log::Debug << "Terrain subdivided: Level: " << quad->level << " Position: " << quad->pos.x << ", " <<quad->pos.z << " Size: " << quad->size.x << ", " << quad->size.z << "\n";


}

void Terrain::UnSubdivide(std::shared_ptr<TerrainQuad> quad) {
	if (quad->isSubdivided) {

		auto delUR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpRight);
		if (delUR != quadHandles.end()) {
			delUR->reset();
			quadHandles.erase(delUR);
			//descriptor->FreeDescriptorSet(quad->subQuads.UpRight->descriptorSet);
			numQuads--;
		}

		auto delDR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownRight);
		if (delDR != quadHandles.end()) {
			delDR->reset();
			quadHandles.erase(delDR);
			//descriptor->FreeDescriptorSet(quad->subQuads.DownRight->descriptorSet);
			numQuads--;
		}

		auto delUL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpLeft);
		if (delUL != quadHandles.end()) {
			delUL->reset();
			quadHandles.erase(delUL);
			//descriptor->FreeDescriptorSet(quad->subQuads.UpLeft->descriptorSet);
			numQuads--;
		}

		auto delDL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownLeft);
		if (delDL != quadHandles.end()) {
			delDL->reset();
			quadHandles.erase(delDL);
			//descriptor->FreeDescriptorSet(quad->subQuads.DownLeft->descriptorSet);
			numQuads--;
		}

		quad->isSubdivided = false;
	}
	//quad->isSubdivided = false;
	//Log::Debug << "Terrain un-subdivided: Level: " << quad->level << " Position: " << quad->pos.x << ", " << quad->pos.z << " Size: " << quad->size.x << ", " << quad->size.z << "\n";
}

void Terrain::DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], bool ifWireframe) {

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &descriptorSet.set, 0, nullptr);

	std::vector<VkDeviceSize> vertexOffsettings(quadHandles.size());
	std::vector<VkDeviceSize> indexOffsettings(quadHandles.size());

	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings[i] = (i * sizeof(TerrainMeshVertices));
		indexOffsettings[i] = (i * sizeof(TerrainMeshIndices));
	}

	vkCmdPushConstants(
		cmdBuff,
		mvp->layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(TerrainPushConstant),
		&modelMatrixData);

	drawTimer.StartTimer();
	for (int i = 0; i < quadHandles.size(); ++i) {
		if (!quadHandles[i]->isSubdivided ){//&& (*(quadHandles[i]->isReady)) == true) {


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
	Corner_Enum corner, float heightScale, float widthScale, int maxSubDivLevels) {

	const int numCells = NumCells;

	TerrainMeshVertices& verts = quad->vertices;
	TerrainMeshIndices& indices = quad->indices;
	      
	float uvUs[numCells + 3];
	float uvVs[numCells + 3];
	
	float powLevel = 1 << quad->level;
	for (int i = 0; i < numCells + 3; i++)
	{
		uvUs[i] = glm::clamp((float)(i - 1) / (powLevel * (numCells)) + (float)quad->subDivPos.x / powLevel, 0.0f, 1.0f);
		uvVs[i] = glm::clamp((float)(i - 1) / (powLevel * (numCells)) + (float)quad->subDivPos.y / powLevel, 0.0f, 1.0f);
	}

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			float uvU = uvUs[(i + 1)];
			float uvV = uvVs[(j + 1)];

			float uvUminus = uvUs[(i + 1) - 1];
			float uvUplus = uvUs[(i + 1) + 1];
			float uvVminus = uvVs[(j + 1) - 1];
			float uvVplus = uvVs[(j + 1) + 1];

			float outheight = graphUser.SampleHeightMap(uvU, uvV);
			float outheightum = graphUser.SampleHeightMap(uvUminus, uvV);
			float outheightup = graphUser.SampleHeightMap(uvUplus, uvV) ;
			float outheightvm = graphUser.SampleHeightMap(uvU, uvVminus);
			float outheightvp = graphUser.SampleHeightMap(uvU, uvVplus) ;

			glm::vec3 normal = glm::normalize(glm::vec3((outheightum - outheightup),
				((uvUplus - uvUminus) + (uvVplus - uvVminus))*2,
				(outheightvm - outheightvp)));

			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = uvU * (widthScale);
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = outheight * heightScale;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = uvV * (widthScale);
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = uvU;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = uvV;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = 0.0f;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = 0.0f;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = 0.0f;
			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = 0.0f;
		}
	}

	//for (int i = 0; i <= numCells; i++)
	//{
	//	for (int j = 0; j <= numCells; j++)
	//	{
	//		float uvU = (float)i / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level);
	//		float uvV = (float)j / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level);

	//		float uvUminus = glm::clamp((float)(i - 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level), 0.0f, 1.0f);
	//		float uvUplus = glm::clamp((float)(i + 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.x / (float)(1 << quad->level), 0.0f, 1.0f);
	//		float uvVminus = glm::clamp((float)(j - 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level), 0.0f, 1.0f);
	//		float uvVplus = glm::clamp((float)(j + 1) / ((1 << quad->level) * (float)(numCells)) + (float)quad->subDivPos.y / (float)(1 << quad->level), 0.0f, 1.0f);

	//		float outHeight = graphUser.SampleHeightMap(uvU, uvV);
	//		float outHeightUM = graphUser.SampleHeightMap(uvUminus, uvV);
	//		float outHeightUP = graphUser.SampleHeightMap(uvUplus, uvV);
	//		float outHeightVM = graphUser.SampleHeightMap(uvU, uvVminus);
	//		float outHeightVP = graphUser.SampleHeightMap(uvU, uvVplus);

	//		glm::vec3 normal = glm::normalize(glm::vec3((outHeightUM - outHeightUP),
	//			((uvUplus - uvUminus) + (uvVplus - uvVminus)) * 2,
	//			(outHeightVM - outHeightVP)));

	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = uvU * (widthScale); //(float)i *(xSize) / (float)numCells;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = outHeight * heightScale;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = uvV * (widthScale); //(float)j * (zSize) / (float)numCells;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = uvU;
	//		(verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = uvV;
	//	}
	//}
	
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
}