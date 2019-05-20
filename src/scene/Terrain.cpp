#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>

#include "core/Logger.h"

#include "rendering/RenderStructs.h"

#include "TerrainManager.h"
#include "rendering/Initializers.h"

TerrainQuad::TerrainQuad (glm::vec2 pos,
    glm::vec2 size,
    glm::i32vec2 logicalPos,
    glm::i32vec2 logicalSize,
    int level,
    glm::i32vec2 subDivPos,
    float centerHeightValue,
    Terrain* terrain)
:

  pos (pos),
  size (size),
  logicalPos (logicalPos),
  logicalSize (logicalSize),
  subDivPos (subDivPos),
  isSubdivided (false),
  level (level),
  heightValAtCenter (0),
  terrain (terrain)
{
	bound.h_w_pp = glm::vec4 (terrain->heightScale, terrain->coordinateData.size.x, -2, -3);

	glm::vec2 br = pos + size;
	bound.pos = glm::vec4 (pos.y, pos.x, br.y, br.x);

	if (level == 0)
	{
		bound.uv = glm::vec4 (0, 0, 1, 1);
	}
	else
	{
		float powLevel = (float)(1 << (level));
		glm::vec2 uv_tl = glm::vec2 (subDivPos.y / powLevel, subDivPos.x / powLevel);
		glm::vec2 uv_br = glm::vec2 ((NumCells + 1) / (powLevel * NumCells) + subDivPos.y / powLevel,
		    (NumCells + 1) / (powLevel * NumCells) + subDivPos.x / powLevel);
		bound.uv = glm::vec4 (uv_tl.x, uv_tl.y, uv_br.x, uv_br.y);
	}
}

float TerrainQuad::GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos)
{
	return glm::clamp (
	    (float)(i) / ((1 << level) * (float)(numCells)) + (float)(subDivPos) / (float)(1 << level), 0.0f, 1.0f);
}

Terrain::Terrain (VulkanRenderer& renderer,
    InternalGraph::GraphPrototype& protoGraph,
    int numCells,
    int maxLevels,
    float heightScale,
    TerrainCoordinateData coords,
    VulkanModel* grid)
: renderer (renderer),
  maxLevels (maxLevels),
  heightScale (heightScale),
  coordinateData (coords),
  fastGraphUser (protoGraph, 1337, coords.sourceImageResolution, coords.noisePos, coords.noiseSize.x),
  terrainGrid (grid)
{

	// simple calculation right now, does the absolute max number of quads possible with given max
	// level in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels <= 0)
	{
		maxNumQuads = 1;
	}
	else
	{
		// with current quad density this is the average upper bound (kinda a guess but its probably more than enough for now (had to add 25 cause it wasn't enough lol!)
		maxNumQuads = 1 + 16 + 20 + 25 + 50 * maxLevels;
		// maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads (like if everything was subdivided)
	}
	heightMapData = fastGraphUser.GetHeightMap ().GetImageVectorData ();

	splatMapData = fastGraphUser.GetSplatMapPtr ();
	splatMapSize = (int)glm::pow (coords.sourceImageResolution, 2);
}

int Terrain::FindEmptyIndex ()
{
	return curEmptyIndex++; // always gets an index one higher
}

void Terrain::InitTerrain (glm::vec3 cameraPos,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal)
{
	SetupUniformBuffer ();
	SetupImage ();
	SetupDescriptorSets (terrainVulkanTextureArrayAlbedo,
	    terrainVulkanTextureArrayRoughness,
	    terrainVulkanTextureArrayMetallic,
	    terrainVulkanTextureArrayNormal);
	SetupPipeline ();

	quadMap.emplace (std::make_pair (FindEmptyIndex (),
	    TerrainQuad{ coordinateData.pos,
	        coordinateData.size,
	        coordinateData.noisePos,
	        coordinateData.noiseSize,
	        0,
	        glm::i32vec2 (0, 0),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0)),
	        this }));
	InitTerrainQuad (rootQuad, cameraPos);
}

void Terrain::UpdateTerrain (glm::vec3 viewerPos)
{
	SimpleTimer updateTime;
	updateTime.StartTimer ();

	UpdateTerrainQuad (rootQuad, viewerPos);

	updateTime.EndTimer ();

	// if (updateTime.GetElapsedTimeMicroSeconds() > 1500)
	//	Log::Debug << " Update time " << updateTime.GetElapsedTimeMicroSeconds() << "\n";
}

void Terrain::SetupUniformBuffer ()
{
	uniformBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (ModelBufferObject));

	ModelBufferObject mbo;
	mbo.model = glm::mat4 (1.0f);
	//mbo.model = glm::translate (mbo.model, glm::vec3 (coordinateData.pos.x, 0, coordinateData.pos.y));
	mbo.normal = glm::transpose (glm::inverse (mbo.model));
	uniformBuffer->CopyToBuffer (&mbo, sizeof (ModelBufferObject));

	instanceBuffer =
	    std::make_shared<VulkanBufferInstancePersistant> (renderer.device, 2048, heightmapboundsize);
}

void Terrain::SetupImage ()
{
	TexCreateDetails details (VK_FORMAT_R32_SFLOAT,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    true,
	    8,
	    coordinateData.sourceImageResolution,
	    coordinateData.sourceImageResolution);
	details.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	heightMapTexture = std::make_shared<VulkanTexture> (renderer, details, heightMapData);

	if (splatMapData != nullptr)
	{
		TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM,
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    true,
		    8,
		    coordinateData.sourceImageResolution,
		    coordinateData.sourceImageResolution);
		details.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		terrainVulkanSplatMap =
		    renderer.textureManager.CreateTextureFromData (details, splatMapData, splatMapSize);
	}
	else
	{
		throw std::runtime_error ("Failed to get terrain splat map data!");
	}
}

void Terrain::SetupDescriptorSets (std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal)
{
	descriptor = renderer.GetVulkanDescriptor ();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1));
	descriptor->SetupLayout (m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));
	descriptor->SetupPool (poolSizes, 1);

	// VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	descriptorSet = descriptor->CreateDescriptorSet ();

	std::vector<DescriptorUse> writes;
	writes.push_back (DescriptorUse (0, 1, uniformBuffer->resource));
	writes.push_back (DescriptorUse (1, 1, heightMapTexture->resource));
	writes.push_back (DescriptorUse (2, 1, terrainVulkanSplatMap->resource));
	writes.push_back (DescriptorUse (3, 1, terrainVulkanTextureArrayAlbedo->resource));
	writes.push_back (DescriptorUse (4, 1, terrainVulkanTextureArrayRoughness->resource));
	writes.push_back (DescriptorUse (5, 1, terrainVulkanTextureArrayMetallic->resource));
	writes.push_back (DescriptorUse (6, 1, terrainVulkanTextureArrayNormal->resource));
	descriptor->UpdateDescriptorSet (descriptorSet, writes);
}

void Terrain::SetupPipeline ()
{
	PipelineOutline out;

	auto vert = renderer.shaderManager.loadShaderModule ("assets/shaders/terrain.vert.spv", ShaderType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule ("assets/shaders/terrain.frag.spv", ShaderType::fragment);

	ShaderModuleSet shader_set;
	shader_set.Vertex (vert).Fragment (frag);
	out.SetShaderModuleSet (shader_set);

	VertexLayout layout (Vert_PosNormUv);
	out.AddVertexLayouts (layout.bindingDesc, layout.attribDesc);

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

	std::vector<VkVertexInputBindingDescription> instanceBufferBinding = { initializers::vertexInputBindingDescription (
		INSTANCE_BUFFER_BIND_ID, sizeof (HeightMapBound), VK_VERTEX_INPUT_RATE_INSTANCE) };

	std::vector<VkVertexInputAttributeDescription> instanceBufferAttribute = {
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32A32_SFLOAT, 0), // Location 4: pos
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof (float) * 4), // Location 5: yv
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof (float) * 8), // Location 6: h_w_pp
		initializers::vertexInputAttributeDescription (
		    INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof (float) * 12) // Location padd
	};

	out.AddVertexLayouts (instanceBufferBinding, instanceBufferAttribute);

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
	out.AddDescriptorLayout (descriptor->GetLayout ());

	out.AddDynamicStates ({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

	// out.AddPushConstantRange (
	//    initializers::pushConstantRange (VK_SHADER_STAGE_VERTEX_BIT, sizeof (HeightMapBound), 0));

	normal = std::make_unique<Pipeline> (renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));

	out.SetRasterizer (
	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	wireframe = std::make_unique<Pipeline> (
	    renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));
}

void Terrain::InitTerrainQuad (int quad, glm::vec3 viewerPos)
{

	UpdateTerrainQuad (quad, viewerPos);
}


void Terrain::UpdateTerrainQuad (int quad, glm::vec3 viewerPos)
{

	float SubdivideDistanceBias = 2.0f;

	glm::vec3 center = glm::vec3 (quadMap.at (quad).pos.x + quadMap.at (quad).size.x / 2.0f,
	    quadMap.at (quad).heightValAtCenter,
	    quadMap.at (quad).pos.y + quadMap.at (quad).size.y / 2.0f);
	float distanceToViewer = glm::distance (viewerPos, center);

	if (!quadMap.at (quad).isSubdivided)
	{ // can only subdivide if this quad isn't already subdivided
		if (distanceToViewer < quadMap.at (quad).size.x * SubdivideDistanceBias && quadMap.at (quad).level < maxLevels)
		{ // must be
			SubdivideTerrain (quad, viewerPos);
		}
	}

	else if (distanceToViewer > quadMap.at (quad).size.x * SubdivideDistanceBias)
	{
		UnSubdivide (quad);
	}
	else
	{
		UpdateTerrainQuad (quadMap.at (quad).subQuads.UpRight, viewerPos);
		UpdateTerrainQuad (quadMap.at (quad).subQuads.UpLeft, viewerPos);
		UpdateTerrainQuad (quadMap.at (quad).subQuads.DownRight, viewerPos);
		UpdateTerrainQuad (quadMap.at (quad).subQuads.DownLeft, viewerPos);
	}
}

void Terrain::SubdivideTerrain (int quad, glm::vec3 viewerPos)
{
	auto& q = quadMap.at (quad);
	q.isSubdivided = true;
	numQuads += 4;

	glm::vec2 new_pos = glm::vec2 (q.pos.x, q.pos.y);
	glm::vec2 new_size = glm::vec2 (q.size.x / 2.0, q.size.y / 2.0);

	glm::i32vec2 new_lpos = glm::i32vec2 (q.logicalPos.x, q.logicalPos.y);
	glm::i32vec2 new_lsize = glm::i32vec2 (q.logicalSize.x / 2.0, q.logicalSize.y / 2.0);

	q.subQuads.UpRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.UpRight,
	    TerrainQuad (glm::vec2 (new_pos.x, new_pos.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x, new_lpos.y),
	        new_lsize,
	        q.level + 1,
	        glm::i32vec2 (q.subDivPos.x * 2, q.subDivPos.y * 2),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2)),
	        this)));

	q.subQuads.UpLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.UpLeft,
	    TerrainQuad (glm::vec2 (new_pos.x, new_pos.y + new_size.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        q.level + 1,
	        glm::i32vec2 (q.subDivPos.x * 2, q.subDivPos.y * 2 + 1),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2 + 1)),
	        this)));

	q.subQuads.DownRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.DownRight,
	    TerrainQuad (glm::vec2 (new_pos.x + new_size.x, new_pos.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x + new_lsize.x, new_lpos.y),
	        new_lsize,
	        q.level + 1,
	        glm::i32vec2 (q.subDivPos.x * 2 + 1, q.subDivPos.y * 2),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2 + 1),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2)),
	        this)));

	q.subQuads.DownLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.DownLeft,
	    TerrainQuad (glm::vec2 (new_pos.x + new_size.x, new_pos.y + new_size.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        q.level + 1,
	        glm::i32vec2 (q.subDivPos.x * 2 + 1, q.subDivPos.y * 2 + 1),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2 + 1),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2 + 1)),
	        this)));

	InitTerrainQuad (q.subQuads.UpRight, viewerPos);
	InitTerrainQuad (q.subQuads.UpLeft, viewerPos);
	InitTerrainQuad (q.subQuads.DownRight, viewerPos);
	InitTerrainQuad (q.subQuads.DownLeft, viewerPos);
}

void Terrain::UnSubdivide (int quad)
{
	if (quadMap.at (quad).isSubdivided)
	{
		UnSubdivide (quadMap.at (quad).subQuads.UpRight);
		UnSubdivide (quadMap.at (quad).subQuads.UpLeft);
		UnSubdivide (quadMap.at (quad).subQuads.DownRight);
		UnSubdivide (quadMap.at (quad).subQuads.DownLeft);

		quadMap.erase (quadMap.at (quad).subQuads.UpRight);
		quadMap.erase (quadMap.at (quad).subQuads.UpLeft);
		quadMap.erase (quadMap.at (quad).subQuads.DownRight);
		quadMap.erase (quadMap.at (quad).subQuads.DownLeft);
		numQuads -= 4;

		quadMap.at (quad).isSubdivided = false;
	}
}

void Terrain::DrawTerrainRecursive (
    int quad, VkCommandBuffer cmdBuf, bool ifWireframe, std::vector<HeightMapBound>& instances)
{
	if (quadMap.at (quad).isSubdivided)
	{
		DrawTerrainRecursive (quadMap.at (quad).subQuads.UpRight, cmdBuf, ifWireframe, instances);
		DrawTerrainRecursive (quadMap.at (quad).subQuads.UpLeft, cmdBuf, ifWireframe, instances);
		DrawTerrainRecursive (quadMap.at (quad).subQuads.DownRight, cmdBuf, ifWireframe, instances);
		DrawTerrainRecursive (quadMap.at (quad).subQuads.DownLeft, cmdBuf, ifWireframe, instances);
	}
	else
	{
		instances.push_back (quadMap.at (quad).bound);
	}
}

void Terrain::DrawTerrainGrid (VkCommandBuffer cmdBuf, bool ifWireframe)
{
	if (ifWireframe)
		wireframe->Bind (cmdBuf);
	else
		normal->Bind (cmdBuf);
	vkCmdBindDescriptorSets (
	    cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, normal->GetLayout (), 2, 1, &descriptorSet.set, 0, nullptr);
	terrainGrid->BindModel (cmdBuf);

	std::vector<HeightMapBound> instances;
	instances.reserve (numQuads);

	DrawTerrainRecursive (0, cmdBuf, ifWireframe, instances);

	instanceBuffer->CopyToBuffer (instances.data (), sizeof(HeightMapBound) *instances.size());

	terrainGrid->BindModel (cmdBuf);
	instanceBuffer->BindInstanceBuffer (cmdBuf);

	vkCmdDrawIndexed (cmdBuf, static_cast<uint32_t> (indCount), instances.size (), 0, 0, 0);
}

float Terrain::GetHeightAtLocation (float x, float z)
{
	return fastGraphUser.SampleHeightMap (x, z) * heightScale;
}