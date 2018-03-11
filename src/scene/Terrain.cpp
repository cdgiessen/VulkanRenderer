#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>

#include "../core/Logger.h"

TerrainQuad::TerrainQuad() {
	pos = glm::vec2(0);
	size = glm::vec2(0);
	level = 0;
	isSubdivided = false;
}

TerrainQuad::~TerrainQuad() {

}

void TerrainQuad::init(glm::vec2 pos, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos, float centerHeightValue) {
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

TerrainQuadData::~TerrainQuadData()
{
	subQuads.DownLeft.reset();
	subQuads.DownRight.reset();
	subQuads.UpLeft.reset();
	subQuads.UpRight.reset();
}

Terrain::Terrain(
	std::shared_ptr<MemoryPool<TerrainQuadData>> pool,
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
	//terrainQuads = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
	//terrainQuads = pool;
	quadHandles.reserve(maxNumQuads);
	//terrainGenerationWorkers = std::vector < std::thread>(maxNumQuads);

	//fastTerrainUser = std::make_unique<NewNodeGraph::TerGenGraphUser>(sourceGraph, 1337, sourceImageResolution, noisePosition, noiseSize.x);

	//fastTerrainGraph = std::make_shared<NewNodeGraph::TerGenNodeGraph> (1337, sourceImageResolution, noisePosition, noiseSize.x);
	//fastTerrainGraph->BuildNoiseGraph();

	splatmapTextureGradient.SetFrontColor(glm::vec4(1, 0, 0, 0));

	splatmapTextureGradient.AddControlPoint(0.5f, glm::vec4(1, 0, 0, 0));
	splatmapTextureGradient.AddControlPoint(0.6f, glm::vec4(0, 1, 0, 0));
	splatmapTextureGradient.AddControlPoint(0.65f, glm::vec4(0, 1, 0, 0));
	splatmapTextureGradient.AddControlPoint(0.7f, glm::vec4(0, 0, 1, 0));
	splatmapTextureGradient.AddControlPoint(0.8f, glm::vec4(0, 0, 1, 0));

	splatmapTextureGradient.SetBackColor(glm::vec4(0, 0, 0, 1));

	//TerrainQuad* test = terrainQuadPool->allocate();
	//test->init(posX, posY, sizeX, sizeY, 0, meshVertexPool->allocate(), meshIndexPool->allocate());

	//maillerFace = std::make_shared< Texture>();
	//maillerFace->loadFromFileGreyOnly("Resources/Textures/maillerFace.png");
	
}


Terrain::~Terrain() {
	//Log::Debug << "terrain deleted\n";
	CleanUp();
		
	//terrainSplatMap.reset();
	//terrainTextureArray.reset();

	//maillerFace.reset();
}

void Terrain::CleanUp()
{

	//for (auto item : quadHandles) {
	//	terrainQuads->destroy(item.get());
	//}

	//RecursiveUnSubdivide(rootQuad);
	//rootQuad.reset();
	//for (auto item : quadHandles) {
	//	item.reset();
	//}
	//for (auto item : PrevQuadHandles) {
	//	item.reset();
	//}
	//quadHandles.clear();
	//PrevQuadHandles.clear();

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

	//std::shared_ptr<TerrainQuadData> q = std::make_shared<TerrainQuadData>(terrainQuads->allocate());
	std::shared_ptr<TerrainQuadData> q = std::make_shared<TerrainQuadData>();
	quadHandles.push_back(q);
	rootQuad = InitTerrainQuad(q, coordinateData.pos, coordinateData.size, coordinateData.noisePos, coordinateData.noiseSize, 0);

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

bool Terrain::UpdateTerrainQuad(std::shared_ptr<TerrainQuadData> quad, glm::vec3 viewerPos) {
	
	glm::vec3 center = glm::vec3(quad->terrainQuad.pos.x + quad->terrainQuad.size.x / 2.0f, quad->terrainQuad.heightValAtCenter, quad->terrainQuad.pos.y + quad->terrainQuad.size.y / 2.0f);
	float distanceToViewer = glm::distance(viewerPos, center);
	bool shouldUpdateBuffers = false;


	if (!quad->terrainQuad.isSubdivided) { //can only subdivide if this quad isn't already subdivided
		if (distanceToViewer < quad->terrainQuad.size.x * 2.0f && quad->terrainQuad.level < maxLevels) { //must be 
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

	if (distanceToViewer > quad->terrainQuad.size.x * 2.0f) {
		UnSubdivide(quad);
		shouldUpdateBuffers = true;
	}
	
	return shouldUpdateBuffers;
}

std::vector<RGBA_pixel>*  Terrain::LoadSplatMapFromGenerator() {
	//fastGraphUser.BuildOutputImage(noisePosition, (float)noiseSize.x);
	auto& thing = fastGraphUser.GetSplatMap();
	return thing.GetImageVectorData();


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

	//FastNoiseSIMD* testNoise = FastNoiseSIMD::NewFastNoiseSIMD();
	//float * testData1 = testNoise->GetPerlinFractalSet(0, 0, 0, 128, 1, 128, 1);
	//float * testData2 = testNoise->GetPerlinFractalSet(0, 0, 0, 128, 1, 128, 2);
	//float * testData3 = testNoise->GetPerlinFractalSet(0, 0, 0, 128, 1, 128, 0.5);
	//float * testData4 = testNoise->GetPerlinFractalSet(-10, 0, -10, 128, 1, 128, 1);
	//float * testData5 = testNoise->GetPerlinFractalSet(128, 0, 0, 128, 1, 128, 1);
	//float * testData6 = testNoise->GetPerlinFractalSet(64, 0, 64, 128, 1, 128, 1);
	//
	//Texture* testImage0 = new Texture(); testImage0->loadFromGreyscalePixelData(128, 128, testData1);
	//Texture* testImage1 = new Texture(); testImage1->loadFromGreyscalePixelData(128, 128, testData2);
	//Texture* testImage2 = new Texture(); testImage2->loadFromGreyscalePixelData(128, 128, testData3);
	//Texture* testImage3 = new Texture(); testImage3->loadFromGreyscalePixelData(128, 128, testData4);
	//Texture* testImage4 = new Texture(); testImage4->loadFromGreyscalePixelData(128, 128, testData5);
	//Texture* testImage5 = new Texture(); testImage5->loadFromGreyscalePixelData(128, 128, testData6);
	//
	//VulkanTexture2D testVulkanImage1; testVulkanImage1.loadFromTexture(testImage0, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	//VulkanTexture2D testVulkanImage2; testVulkanImage2.loadFromTexture(testImage1, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	//VulkanTexture2D testVulkanImage3; testVulkanImage3.loadFromTexture(testImage2, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	//VulkanTexture2D testVulkanImage4; testVulkanImage4.loadFromTexture(testImage3, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	//VulkanTexture2D testVulkanImage5; testVulkanImage5.loadFromTexture(testImage4, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);
	//VulkanTexture2D testVulkanImage6; testVulkanImage6.loadFromTexture(testImage5, VK_FORMAT_R8G8B8A8_UNORM, device, device->graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, false, 1, false);

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


	//layout
	//VkDescriptorSetLayoutBinding cboLayoutBinding	   = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
	//VkDescriptorSetLayoutBinding uboLayoutBinding	   = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	//VkDescriptorSetLayoutBinding lboLayoutBinding	   = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);
	//VkDescriptorSetLayoutBinding splatMapLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1);
	//VkDescriptorSetLayoutBinding texArratLayoutBinding = initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1);

	//std::vector<VkDescriptorSetLayoutBinding> bindings = { cboLayoutBinding, uboLayoutBinding, lboLayoutBinding, splatMapLayoutBinding, texArratLayoutBinding};

	//VkDescriptorSetLayoutCreateInfo layoutInfo = initializers::descriptorSetLayoutCreateInfo(bindings);

	//if (vkCreateDescriptorSetLayout(renderer->device.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to create descriptor set layout!");
	//}

	////Pool
	//std::vector<VkDescriptorPoolSize> poolSizesTerrain;
	//poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	//poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	//poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 * maxNumQuads));
	//poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));
	//poolSizesTerrain.push_back(initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 * maxNumQuads));

	//VkDescriptorPoolCreateInfo poolInfoTerrain =
	//	initializers::descriptorPoolCreateInfo(
	//		static_cast<uint32_t>(poolSizesTerrain.size()),
	//		poolSizesTerrain.data(),
	//		1 * maxNumQuads);

	//poolInfoTerrain.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	//if (vkCreateDescriptorPool(renderer->device.device, &poolInfoTerrain, nullptr, &descriptorPool) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to create descriptor pool!");
	//}
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
		modelObjects.push_back((*it)->terrainQuad.modelUniformObject);
	}

	modelUniformBuffer.CopyToBuffer(renderer->device, modelObjects.data(), modelObjects.size() * sizeof(ModelBufferObject));
	//modelUniformBuffer.map(renderer->device.device, modelObjects.size() * sizeof(ModelBufferObject));
	//modelUniformBuffer.copyTo(modelObjects.data(), modelObjects.size() * sizeof(ModelBufferObject));
	//modelUniformBuffer.unmap();
}

void Terrain::UpdateMeshBuffer() {
	//	vertexBuffer.map(renderer->device.device, meshVertexPool->MaxChunks() * sizeof(TerrainMeshVertices), 0);
	//	vertexBuffer.copyTo(&meshVertexPool, meshVertexPool->ChunksUsed() * sizeof(TerrainMeshVertices));
	//	vertexBuffer.unmap();
	//
	//	indexBuffer.map(renderer->device.device, meshIndexPool->MaxChunks() * sizeof(TerrainMeshIndices), 0);
	//	indexBuffer.copyTo(&meshIndexPool, meshIndexPool->ChunksUsed() * sizeof(TerrainMeshIndices));
	//	indexBuffer.unmap();
	//

	//vkDestroyBuffer(renderer->device.device, vertexBuffer.buffer, nullptr);
	//vkFreeMemory(renderer->device.device, vertexBuffer.bufferMemory, nullptr);
	//vkDestroyBuffer(renderer->device.device, indexBuffer.buffer, nullptr);
	//vkFreeMemory(renderer->device.device, indexBuffer.bufferMemory, nullptr);

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

		//VkBuffer vmaStagingBufVertex = VK_NULL_HANDLE;
		//VkBuffer vmaStagingBufIndex = VK_NULL_HANDLE;
		//
		//VmaAllocation vmaStagingVertices = VK_NULL_HANDLE;
		//VmaAllocation vmaStagingIndecies = VK_NULL_HANDLE;
		//
		//renderer->device.CreateMeshStagingBuffer(&vmaStagingBufVertex, &vmaStagingVertices, verts.data(), vBufferSize);
		//renderer->device.CreateMeshStagingBuffer(&vmaStagingBufIndex, &vmaStagingIndecies, inds.data(), iBufferSize);

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


//used for root quad
std::shared_ptr<TerrainQuadData> Terrain::InitTerrainQuad(std::shared_ptr<TerrainQuadData> q, glm::vec2 position, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, 
	int level) {
	numQuads++;
	
	q->terrainQuad.init(position, size, logicalPos, logicalSize, level, glm::i32vec2(0,0), 0);
	//q->terrainQuad.CreateTerrainMesh(&q->vertices, &q->indices);

	//SimpleTimer terrainQuadCreateTime;
	
	GenerateNewTerrainSubdivision(fastGraphUser, q->vertices, q->indices, q->terrainQuad, Corner_Enum::uR, heightScale, maxLevels);

	//GenerateTerrainFromTexture(*maillerFace, q->vertices, q->indices, q->terrainQuad, Corner_Enum::uR, heightScale, maxLevels);
	
	//std::thread* worker = new std::thread(GenerateNewTerrain, terrainGenerator, fastTerrainGraph, &q->vertices, &q->indices, q->terrainQuad, heightScale, maxLevels);
	//terrainGenerationWorkers.push_back(worker);
	
	//terrainQuadCreateTime.EndTimer();
	//Log::Debug << "Original " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << "\n";

	//std::vector<VkDescriptorSetLayout> layouts;
	//layouts.resize(maxNumQuads);
	//std::fill(layouts.begin(), layouts.end(), descriptorSetLayout);

	modelUniformBuffer.resource.FillResource(modelUniformBuffer.buffer.buffer, 
		(VkDeviceSize)((quadHandles.size() - 1) * sizeof(ModelBufferObject)), (VkDeviceSize)sizeof(ModelBufferObject));
	q->descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(2, 1, modelUniformBuffer.resource));
	writes.push_back(DescriptorUse(3, 1, terrainVulkanSplatMap.resource));
	writes.push_back(DescriptorUse(4, 1, terrainVulkanTextureArray->resource));
	descriptor->UpdateDescriptorSet(q->descriptorSet, writes);

	return q;
}

std::shared_ptr<TerrainQuadData> Terrain::InitTerrainQuadFromParent(std::shared_ptr<TerrainQuadData> parent, std::shared_ptr<TerrainQuadData> q, Corner_Enum corner, glm::vec2 position, glm::vec2 size,
	glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos) {
	
	q->terrainQuad.init(position, size, logicalPos, logicalSize, level, subDivPos, 0);
	//q->terrainQuad.CreateTerrainMeshFromParent(&parent->vertices, &parent->indices, &q->vertices, &q->indices, corner);

	//SimpleTimer terrainQuadCreateTime;

	//std::thread *worker = new std::thread(GenerateNewTerrainSubdivision, terrainGenerator, fastTerrainGraph, &q->vertices, &q->indices, q->terrainQuad, corner, heightScale, maxLevels);
	//terrainGenerationWorkers.push_back(worker);
	//GenerateTerrainFromExisting( fastTerrainGraph, &parent->vertices, &parent->indices, &q->vertices, &q->indices, corner, q->terrainQuad, heightScale, maxLevels);
	
	GenerateNewTerrainSubdivision(fastGraphUser, q->vertices, q->indices, q->terrainQuad, corner, heightScale, maxLevels);
	
	//GenerateTerrainFromTexture(*maillerFace, q->vertices, q->indices, q->terrainQuad, corner, heightScale, maxLevels);

	//terrainQuadCreateTime.EndTimer();
	//Log::Debug << "From Parent " << terrainQuadCreateTime.GetElapsedTimeMicroSeconds() << "\n";
	//std::vector<VkDescriptorSetLayout> layouts;
	//layouts.resize(maxNumQuads);
	//std::fill(layouts.begin(), layouts.end(), descriptorSetLayout);

	modelUniformBuffer.resource.FillResource(modelUniformBuffer.buffer.buffer,
		(VkDeviceSize)((quadHandles.size() - 1) * sizeof(ModelBufferObject)), (VkDeviceSize)sizeof(ModelBufferObject));

	q->descriptorSet = descriptor->CreateDescriptorSet();

	std::vector<DescriptorUse> writes;
	writes.push_back(DescriptorUse(2, 1, modelUniformBuffer.resource));
	writes.push_back(DescriptorUse(3, 1, terrainVulkanSplatMap.resource));
	writes.push_back(DescriptorUse(4, 1, terrainVulkanTextureArray->resource));
	descriptor->UpdateDescriptorSet(q->descriptorSet, writes);
	return q;
}

void Terrain::SubdivideTerrain(std::shared_ptr<TerrainQuadData> quad, glm::vec3 viewerPos) {
	quad->terrainQuad.isSubdivided = true;
	numQuads += 4;

	glm::vec2 new_pos = glm::vec2(quad->terrainQuad.pos.x, quad->terrainQuad.pos.y);
	glm::vec2 new_size = glm::vec2(quad->terrainQuad.size.x/2.0, quad->terrainQuad.size.y/2.0);
	
	glm::i32vec2 new_lpos = glm::i32vec2(quad->terrainQuad.logicalPos.x, quad->terrainQuad.logicalPos.y);
	glm::i32vec2 new_lsize = glm::i32vec2(quad->terrainQuad.logicalSize.x / 2.0, quad->terrainQuad.logicalSize.y / 2.0);

	std::shared_ptr<TerrainQuadData> qUR = std::make_shared<TerrainQuadData>();
	std::shared_ptr<TerrainQuadData> qUL = std::make_shared<TerrainQuadData>();
	std::shared_ptr<TerrainQuadData> qDR = std::make_shared<TerrainQuadData>();
	std::shared_ptr<TerrainQuadData> qDL = std::make_shared<TerrainQuadData>();
	quadHandles.push_back(qUR);												 
	quadHandles.push_back(qUL);
	quadHandles.push_back(qDR);
	quadHandles.push_back(qDL);

	quad->subQuads.UpRight = InitTerrainQuadFromParent(quad, qUR, Corner_Enum::uR, glm::vec2(new_pos.x, new_pos.y), new_size, 
		glm::i32vec2(new_lpos.x, new_lpos.y), new_lsize, quad->terrainQuad.level + 1,  glm::i32vec2(quad->terrainQuad.subDivPos.x * 2, quad->terrainQuad.subDivPos.y * 2));

	quad->subQuads.UpLeft = InitTerrainQuadFromParent(quad, qUL, Corner_Enum::uL, glm::vec2(new_pos.x, new_pos.y + new_size.y), new_size, 
		glm::i32vec2(new_lpos.x, new_lpos.y + new_lsize.y), new_lsize, quad->terrainQuad.level + 1, glm::i32vec2(quad->terrainQuad.subDivPos.x * 2, quad->terrainQuad.subDivPos.y * 2 + 1));

	quad->subQuads.DownRight = InitTerrainQuadFromParent(quad, qDR, Corner_Enum::dR, glm::vec2(new_pos.x + new_size.x, new_pos.y), new_size, 
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y), new_lsize, quad->terrainQuad.level + 1, glm::i32vec2(quad->terrainQuad.subDivPos.x * 2 + 1, quad->terrainQuad.subDivPos.y * 2));

	quad->subQuads.DownLeft = InitTerrainQuadFromParent(quad, qDL, Corner_Enum::dL, glm::vec2(new_pos.x + new_size.x, new_pos.y + new_size.y), new_size, 
		glm::i32vec2(new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y), new_lsize, quad->terrainQuad.level + 1,  glm::i32vec2(quad->terrainQuad.subDivPos.x * 2 + 1, quad->terrainQuad.subDivPos.y * 2 + 1));

	UpdateTerrainQuad(quad->subQuads.UpRight, viewerPos);
	UpdateTerrainQuad(quad->subQuads.UpLeft, viewerPos);
	UpdateTerrainQuad(quad->subQuads.DownRight, viewerPos);
	UpdateTerrainQuad(quad->subQuads.DownLeft, viewerPos);

	//Log::Debug << "Terrain subdivided: Level: " << quad->terrainQuad.level << " Position: " << quad->terrainQuad.pos.x << ", " <<quad->terrainQuad.pos.z << " Size: " << quad->terrainQuad.size.x << ", " << quad->terrainQuad.size.z << "\n";

	
}

void Terrain::UnSubdivide(std::shared_ptr<TerrainQuadData> quad) {
	if (quad->terrainQuad.isSubdivided) {

		auto delUR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpRight);
		if (delUR != quadHandles.end()) {
			delUR->reset();
			quadHandles.erase(delUR);
			descriptor->FreeDescriptorSet(quad->subQuads.UpRight->descriptorSet);
			//vkFreeDescriptorSets(renderer->device.device, descriptorPool, 1, &quad->subQuads.UpRight->descriptorSet);
			//terrainQuads->deallocate(quad->subQuads.UpRight);
			//terrainQuads->deleteElement(quad->subQuads.UpRight.get());
			numQuads--;
		}

		auto delDR = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownRight);
		if (delDR != quadHandles.end()) {
			delDR->reset();
			quadHandles.erase(delDR);
			descriptor->FreeDescriptorSet(quad->subQuads.DownRight->descriptorSet);
			//vkFreeDescriptorSets(renderer->device.device, descriptorPool, 1, &quad->subQuads.DownRight->descriptorSet);
			//terrainQuads->deallocate(quad->subQuads.DownRight);
			//terrainQuads->deleteElement(quad->subQuads.DownRight.get());
			numQuads--;
		}

		auto delUL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.UpLeft);
		if (delUL != quadHandles.end()) {
			delUL->reset();
			quadHandles.erase(delUL);
			descriptor->FreeDescriptorSet(quad->subQuads.UpLeft->descriptorSet);
			//vkFreeDescriptorSets(renderer->device.device, descriptorPool, 1, &quad->subQuads.UpLeft->descriptorSet);
			//terrainQuads->deallocate(quad->subQuads.UpLeft);
			//terrainQuads->deleteElement(quad->subQuads.UpLeft.get());
			numQuads--;
		}

		auto delDL = std::find(quadHandles.begin(), quadHandles.end(), quad->subQuads.DownLeft);
		if (delDL != quadHandles.end()) {
			delDL->reset();
			quadHandles.erase(delDL);
			descriptor->FreeDescriptorSet(quad->subQuads.DownLeft->descriptorSet);
			//vkFreeDescriptorSets(renderer->device.device, descriptorPool, 1, &quad->subQuads.DownLeft->descriptorSet);
			//terrainQuads->deallocate(quad->subQuads.DownLeft);
			//terrainQuads->deleteElement(quad->subQuads.DownLeft.get());
			numQuads--;
		}

		quad->terrainQuad.isSubdivided = false;
	}
	//quad->isSubdivided = false;
	//Log::Debug << "Terrain un-subdivided: Level: " << quad->terrainQuad.level << " Position: " << quad->terrainQuad.pos.x << ", " << quad->terrainQuad.pos.z << " Size: " << quad->terrainQuad.size.x << ", " << quad->terrainQuad.size.z << "\n";
}

void Terrain::RecursiveUnSubdivide(std::shared_ptr<TerrainQuadData> quad) {
	if (quad->terrainQuad.isSubdivided) {
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

	//vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, 0 ? wireframe : pipeline);
	//DrawTerrainQuad(curTerrain->rootQuad, cmdBuff, cmdBuffIndex, offsets);
	//return;

	

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
	
	std::vector<VkDeviceSize> vertexOffsettings(quadHandles.size());
	std::vector<VkDeviceSize> indexOffsettings(quadHandles.size());
	
	for (int i = 0; i < quadHandles.size(); i++) {
		vertexOffsettings[i] = (i * sizeof(TerrainMeshVertices));
		indexOffsettings[i] = (i * sizeof(TerrainMeshIndices));
	}
	

	drawTimer.StartTimer();
	for (int i = 0; i < quadHandles.size(); ++i) {
		if (!quadHandles[i]->terrainQuad.isSubdivided) {

			vkCmdPushConstants(
				cmdBuff,
				mvp->layout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(ModelPushConstant),
				&quadHandles[i]->terrainQuad.modelUniformObject);

			vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mvp->layout, 2, 1, &quadHandles[i]->descriptorSet.set, 0, nullptr);
			vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, ifWireframe ? mvp->pipelines->at(1) : mvp->pipelines->at(0));
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

/*
void Terrain::BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], std::shared_ptr<Terrain> curTerrain, bool ifWireframe) {
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

void GenerateNewTerrainSubdivision(InternalGraph::GraphUser& graphUser, TerrainMeshVertices& verts, TerrainMeshIndices& indices,
	TerrainQuad terrainQuad, Corner_Enum corner, float heightScale, int maxSubDivLevels) {

	int numCells = NumCells;
	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.y, xSize = terrainQuad.size.x, zSize = terrainQuad.size.y;

	//NewNodeGraph::TerGenNodeGraph myGraph = NewNodeGraph::TerGenNodeGraph(fastGraph);
	//myGraph.BuildOutputImage(terrainQuad.logicalPos, (float) (1.0/glm::pow(2.0,terrainQuad.level)));

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			
			//float value = (terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));
			//
			//float hL = terrainGenerator->SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			//float hR = terrainGenerator->SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc)*heightScale;
			//float hD = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			//float hU = terrainGenerator->SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			//glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
			//glm::vec3 normal = glm::vec3(0, 1, 0);

			float uvU = (float)i / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level);
			float uvV = (float)j / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level);

			float uvUminus = glm::clamp((float)(i - 1) / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level), 0.0f , 1.0f);
			float uvUplus = glm::clamp((float)(i + 1) / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level), 0.0f, 1.0f);
			float uvVminus = glm::clamp((float)(j - 1) / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level), 0.0f, 1.0f);
			float uvVplus = glm::clamp((float)(j + 1) / ((1 << terrainQuad.level) * (float)(numCells)) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level), 0.0f, 1.0f);

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

}

//void GenerateTerrainFromTexture(Texture& tex, TerrainMeshVertices& verts, TerrainMeshIndices& indices, TerrainQuad terrainQuad, Corner_Enum corner, float heightScale, int maxSubDivLevels) {
//	int numCells = NumCells;
//	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.y, xSize = terrainQuad.size.x, zSize = terrainQuad.size.y;
//
//	//NewNodeGraph::TerGenNodeGraph myGraph = NewNodeGraph::TerGenNodeGraph(*fastGraph);
//	//myGraph.BuildOutputImage(terrainQuad.logicalPos, (float) (1.0/glm::pow(2.0,terrainQuad.level)));
//
//	std::vector<float> px(tex.height * tex.width);
//	//for (int i = 0; i < px.size(); i++) {
//	//	px.at(i) = (*(tex.pixels.data() + i)) / 256.0f;
//	//}
//
//	NewNodeGraph::TEMPTerGenNodeGraph myGraph = NewNodeGraph::TEMPTerGenNodeGraph(&px, 284);
//
//	for (int i = 0; i <= numCells; i++)
//	{
//		for (int j = 0; j <= numCells; j++)
//		{
//			glm::vec3 normal = glm::vec3(0, 1, 0);
//
//			float uvU = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level);
//			float uvV = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level);
//
//			float sample = (float)myGraph.SampleHeight(uvU, 0, uvV);
//
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = (float)i *(xSize) / (float)numCells;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = sample * heightScale;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = (float)j * (zSize) / (float)numCells;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = uvU;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = uvV;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = sample * 2 - 1;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = sample * 2 - 1;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = sample * 2 - 1;
//			(verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = 1.0;
//			//Log::Debug << value << "\n";
//		}
//	}
//
//	int counter = 0;
//	for (int i = 0; i < numCells; i++)
//	{
//		for (int j = 0; j < numCells; j++)
//		{
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//		}
//	}
//
//	RecalculateNormals(numCells, verts, indices);
//}

//Needs to account for which corner the new terrain is in
//void GenerateTerrainFromExisting(TerrainGenerator& terrainGenerator, NewNodeGraph::TerGenGraphUser& fastGraph, TerrainMeshVertices& parentVerts, TerrainMeshIndices& parentIndices,
//	TerrainMeshVertices& verts, TerrainMeshIndices& indices, Corner_Enum corner, TerrainQuad terrainQuad, float heightScale, int maxSubDivLevels) {
//
//	int numCells = NumCells;
//	float xLoc = terrainQuad.pos.x, zLoc = terrainQuad.pos.y, xSize = terrainQuad.size.x, zSize = terrainQuad.size.y;
//
//	NewNodeGraph::TerGenGraphUser myGraph = NewNodeGraph::TerGenGraphUser(fastGraph);
//	myGraph.BuildOutputImage(terrainQuad.logicalPos, (float) (1.0 / glm::pow(2.0, terrainQuad.level)));
//
//	int parentIOffset = (corner == Corner_Enum::dR || corner == Corner_Enum::dL) ? (numCells + 1) / 2 : 0;
//	int parentJOffset = (corner == Corner_Enum::uL || corner == Corner_Enum::dL) ? (numCells + 1) / 2 : 0;
//	
//	//uses the parent terrain for 1/4 of the grid
//	for (int i = 0; i <= numCells; i += 2)
//	{
//		for (int j = 0; j <= numCells; j += 2)
//		{
//			int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//			//existing terrain is every other vertex in the grid
//			int parentVLoc = ((i / 2 + parentIOffset)*(numCells + 1) + (j / 2 + parentJOffset))* vertElementCount;
//
//			(verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
//			(verts)[vLoc + 1] = (parentVerts)[parentVLoc + 1];
//			(verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
//			(verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level);
//			(verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level);
//			(verts)[vLoc + 8] = (parentVerts)[parentVLoc + 8];
//			(verts)[vLoc + 9] = (parentVerts)[parentVLoc + 9];
//			(verts)[vLoc + 10] = (parentVerts)[parentVLoc + 10];
//			(verts)[vLoc + 11] = 1.0;
//
//		}
//	}
//	/* //seams are hard to fix... (cause its a runtime dependent thing, not a create time
//	//edges use data from parent to prevent seaming issues
//	{
//	int i = 0, j = 0;
//	double hL, hR, hD, hU;
//	glm::vec3 normal;
//	int vLoc;
//
//	// i = 0, j[1,numCells - 1]
//	for (int j = 1; j < numCells; j += 2) {
//	int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//	//Use the left/right or up/down to find the appropriate height
//	int neighborVLocA = (i*(numCells + 1) + j + 1)* vertElementCount;
//	int neighborVLocB = (i*(numCells + 1) + j - 1)* vertElementCount;
//
//	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
//	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1])/2.0;
//	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
//	(*verts)[vLoc + 6] = i;
//	(*verts)[vLoc + 7] = j;
//	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
//	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
//	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
//	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
//	}
//
//	// i = numCells, j[1,numCells - 1]
//	i = numCells;
//	for (int j = 1; j < numCells; j += 2) {
//	int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//	//Use the left/right or up/down to find the appropriate height
//	int neighborVLocA = (i*(numCells + 1) + j + 1)* vertElementCount;
//	int neighborVLocB = (i*(numCells + 1) + j - 1)* vertElementCount;
//
//	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
//	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
//	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
//	(*verts)[vLoc + 6] = i;
//	(*verts)[vLoc + 7] = j;
//	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
//	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
//	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
//	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
//	}
//
//	// j = 0, i[1, numCells - 1]
//	j = 0;
//	for (int i = 1; i < numCells; i += 2) {
//	int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//	//Use the left/right or up/down to find the appropriate height
//	int neighborVLocA = ((i + 1)*(numCells + 1) + j)* vertElementCount;
//	int neighborVLocB = ((i - 1)*(numCells + 1) + j)* vertElementCount;
//
//	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
//	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
//	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
//	(*verts)[vLoc + 6] = i;
//	(*verts)[vLoc + 7] = j;
//	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
//	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
//	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
//	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
//	}
//
//	// j = numCells, i[1, numCells - 1]
//	j = numCells;
//	for (int i = 1; i < numCells; i +=2) {
//	int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//	//Use the left/right or up/down to find the appropriate height
//	int neighborVLocA = ((i + 1)*(numCells + 1) + j)* vertElementCount;
//	int neighborVLocB = ((i - 1)*(numCells + 1) + j)* vertElementCount;
//
//	(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
//	(*verts)[vLoc + 1] = ((*verts)[neighborVLocA + 1] + (*verts)[neighborVLocB + 1]) / 2.0;
//	(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
//	(*verts)[vLoc + 6] = i;
//	(*verts)[vLoc + 7] = j;
//	(*verts)[vLoc + 8] = ((*verts)[neighborVLocA + 8] + (*verts)[neighborVLocB + 8]) / 2.0;
//	(*verts)[vLoc + 9] = ((*verts)[neighborVLocA + 9] + (*verts)[neighborVLocB + 9]) / 2.0;
//	(*verts)[vLoc + 10] = ((*verts)[neighborVLocA + 10] + (*verts)[neighborVLocB + 10]) / 2.0;
//	(*verts)[vLoc + 11] = ((*verts)[neighborVLocA + 11] + (*verts)[neighborVLocB + 11]) / 2.0;
//	}
//	}
//	*/
//
//	//Fills in lines starting at i = 1 and skips a line, filling in all the j's (half of the terrain)
//	for (int i = 1; i < numCells; i += 2)
//	{
//		for (int j = 0; j <= numCells; j++)
//		{
//			float value = (terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));
//
//			int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//			(verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
//			(verts)[vLoc + 1] = (float)value * heightScale;
//			(verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
//			(verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.x / (float)(1 << terrainQuad.level);
//			(verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.y / (float)(1 << terrainQuad.level);
//			(verts)[vLoc + 8] = terrainGenerator.SampleColor(0,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells  + (zLoc));
//			(verts)[vLoc + 9] = terrainGenerator.SampleColor(1,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells  + (zLoc));
//			(verts)[vLoc + 10] = terrainGenerator.SampleColor(2,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
//			(verts)[vLoc + 11] = 1.0;
//		}
//	}
//
//	//fills the last 1/4 of cells, starting at i= 0 and jumping. Like the first double for loop but offset by one
//	for (int i = 0; i <= numCells; i += 2)
//	{
//		for (int j = 1; j <= numCells; j += 2)
//		{
//			float value = (terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0.0, (float)j *(zSize) / (float)numCells + (zLoc)));
//
//			int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//
//			(verts)[vLoc + 0] = (float)i *(xSize) / (float)numCells;
//			(verts)[vLoc + 1] = (float)value * heightScale;
//			(verts)[vLoc + 2] = (float)j * (zSize) / (float)numCells;
//			(verts)[vLoc + 6] = (float)i / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.x / (1 << terrainQuad.level);
//			(verts)[vLoc + 7] = (float)j / ((1 << terrainQuad.level) * (float)numCells) + (float)terrainQuad.subDivPos.y / (1 << terrainQuad.level);
//			(verts)[vLoc + 8] = terrainGenerator.SampleColor(0,(float)i *(xSize) / (float)numCells  + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
//			(verts)[vLoc + 9] = terrainGenerator.SampleColor(1,(float)i *(xSize) / (float)numCells  + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
//			(verts)[vLoc + 10] = terrainGenerator.SampleColor(2,(float)i *(xSize) / (float)numCells + (xLoc), 0, j * (zSize) / (float)numCells + (zLoc));
//			(verts)[vLoc + 11] = 1.0;
//		}
//	}
//
//	//normals -- Look I know its verbose and proabably prone to being a waste of time, but since its something as banal as normal calculation and the optomizer doesn't know about how most of the center doesn't need to recalculate the heights, it feels like a useful thing to do
//	{
//
//		//center normals
//		{
//			for (int i = 1; i < numCells; i++)
//			{
//				for (int j = 1; j < numCells; j++)
//				{
//					//gets height values of its neighbors, rather than recalculating them from scratch
//					double hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//					double hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//					double hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//					double hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//
//					//double hUL = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // i + 1, j + 1
//					//double hDR = (*verts)[((i - 1)*(numCells + 1) + j - 1)* vertElementCount + 1]; // i - 1, j -1
//					glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//					int vLoc = (i*(numCells + 1) + j)* vertElementCount;
//					(verts)[vLoc + 3] = normal.x;
//					(verts)[vLoc + 4] = normal.y;
//					(verts)[vLoc + 5] = normal.z;
//				}
//			}
//		}
//
//		//edge normals
//		{
//			int i = 0, j = 0;
//			double hL, hR, hD, hU;
//			glm::vec3 normal;
//			int vLoc;
//
//			// i = 0, j[1,numCells - 1]
//			for (int j = 1; j < numCells; j++) {
//				hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//				hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale;
//				hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//				hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//				vLoc = (i*(numCells + 1) + j)* vertElementCount;
//				(verts)[vLoc + 3] = normal.x;
//				(verts)[vLoc + 4] = normal.y;
//				(verts)[vLoc + 5] = normal.z;
//			}
//
//			// i = numCells, j[1,numCells - 1]
//			i = numCells;
//			for (int j = 1; j < numCells; j++) {
//				hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale;
//				hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//				hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//				hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//				vLoc = (i*(numCells + 1) + j)* vertElementCount;
//				(verts)[vLoc + 3] = normal.x;
//				(verts)[vLoc + 4] = normal.y;
//				(verts)[vLoc + 5] = normal.z;
//			}
//
//			// j = 0, i[1, numCells - 1]
//			j = 0;
//			for (int i = 1; i < numCells; i++) {
//				hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//				hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//				hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//				hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale;//
//				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//				vLoc = (i*(numCells + 1) + j)* vertElementCount;
//				(verts)[vLoc + 3] = normal.x;
//				(verts)[vLoc + 4] = normal.y;
//				(verts)[vLoc + 5] = normal.z;
//			}
//
//			// j = numCells, i[1, numCells - 1]
//			j = numCells;
//			for (int i = 1; i < numCells; i++) {
//				hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//				hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//				hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale;
//				hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//				vLoc = (i*(numCells + 1) + j)* vertElementCount;
//				(verts)[vLoc + 3] = normal.x;
//				(verts)[vLoc + 4] = normal.y;
//				(verts)[vLoc + 5] = normal.z;
//			}
//		}
//
//		//corner normals
//		{
//			double hL, hR, hD, hU;
//			glm::vec3 normal;
//			int vLoc;
//
//			int i = 0, j = 0;
//			hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//			hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
//			hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//			hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
//			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//			vLoc = (i*(numCells + 1) + j)* vertElementCount;
//			(verts)[vLoc + 3] = normal.x;
//			(verts)[vLoc + 4] = normal.y;
//			(verts)[vLoc + 5] = normal.z;
//
//
//			i = 0, j = numCells;
//			hL = (verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
//			hR = terrainGenerator.SampleHeight((float)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
//			hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
//			hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//			vLoc = (i*(numCells + 1) + j)* vertElementCount;
//			(verts)[vLoc + 3] = normal.x;
//			(verts)[vLoc + 4] = normal.y;
//			(verts)[vLoc + 5] = normal.z;
//
//			i = numCells, j = 0;
//			hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
//			hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//			hD = (verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
//			hU = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
//			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//			vLoc = (i*(numCells + 1) + j)* vertElementCount;
//			(verts)[vLoc + 3] = normal.x;
//			(verts)[vLoc + 4] = normal.y;
//			(verts)[vLoc + 5] = normal.z;
//
//			i = numCells, j = numCells;
//			hL = terrainGenerator.SampleHeight((float)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (float)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
//			hR = (verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
//			hD = terrainGenerator.SampleHeight((float)i *(xSize) / (float)numCells + (xLoc), 0, (float)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
//			hU = (verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
//			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));
//
//			vLoc = (i*(numCells + 1) + j)* vertElementCount;
//			(verts)[vLoc + 3] = normal.x;
//			(verts)[vLoc + 4] = normal.y;
//			(verts)[vLoc + 5] = normal.z;
//		}
//	}
//
//	int counter = 0;
//	for (int i = 0; i < numCells; i++)
//	{
//		for (int j = 0; j < numCells; j++)
//		{
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//			
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//
//			j++;
//
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//		}
//
//		i++;
//
//		for (int j = 0; j < numCells; j++)
//		{
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//		
//			j++;
//
//			(indices)[counter++] = i * (numCells + 1) + j;
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//
//			(indices)[counter++] = i * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j + 1;
//			(indices)[counter++] = (i + 1) * (numCells + 1) + j;
//
//		}
//	}
//}

