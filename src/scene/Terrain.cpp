#include "Terrain.h"

#include "core/Logger.h"

#include "rendering/RenderStructs.h"

#include "TerrainManager.h"
#include "rendering/Initializers.h"


TerrainQuad::TerrainQuad (cml::vec2f pos,
    cml::vec2f size,
    cml::vec2i logicalPos,
    cml::vec2f logicalSize,
    int level,
    cml::vec2i subDivPos,
    float centerHeightValue,
    Terrain* terrain)
:

  pos (pos),
  size (size),
  logicalPos (logicalPos),
  logicalSize (logicalSize),
  subDivPos (subDivPos),
  level (level),
  heightValAtCenter (0),
  isSubdivided (false),
  terrain (terrain)
{
	bound.h_w_pp = cml::vec4f (terrain->heightScale, terrain->coordinateData.size.x, -2, -3);

	cml::vec2f br = pos + size;
	bound.pos = cml::vec4f (pos.x, pos.y, br.x, br.y);

	if (level == 0)
	{
		bound.uv = cml::vec4f (0.f, 0.f, 1.f, 1.f);
	}
	else
	{
		float powLevel = (float)(1 << (level));
		cml::vec2f uv_tl = cml::vec2f (subDivPos.x / powLevel, subDivPos.y / powLevel);
		cml::vec2f uv_br = cml::vec2f (
		    1.0f / powLevel + subDivPos.x / powLevel, 1.0f / powLevel + subDivPos.y / powLevel);
		bound.uv = cml::vec4f (uv_tl.x, uv_tl.y, uv_br.x, uv_br.y);
	}
}

float TerrainQuad::GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos)
{
	return cml::clamp (
	    (float)(i) / ((1 << level) * (float)(numCells)) + (float)(subDivPos) / (float)(1 << level), 0.0f, 1.0f);
}

Terrain::Terrain (VulkanRenderer& renderer,
    InternalGraph::GraphPrototype& protoGraph,
    int numCells,
    int maxLevels,
    float heightScale,
    TerrainCoordinateData coords,
    VulkanModel* grid)
: maxLevels (maxLevels),
  renderer (renderer),
  coordinateData (coords),
  heightScale (heightScale),
  terrainGrid (grid),
  fastGraphUser (protoGraph,
      1337,
      coords.sourceImageResolution,
      cml::vec2<int32_t> (coords.noisePos.x, coords.noisePos.y),
      coords.noiseSize.x,
      heightScale)
{
	// simple calculation right now, does the absolute max number of quads possible with given max
	// level in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels <= 0)
	{
		maxNumQuads = 1;
	}
	else
	{
		maxNumQuads = 1024;
		// maxNumQuads = (int)((1.0 - cml::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads (like if everything was subdivided)
	}
}

int Terrain::FindEmptyIndex ()
{
	return curEmptyIndex++; // always gets an index one higher
}

void Terrain::InitTerrain (cml::vec3f cameraPos,
    VulkanTextureID texArrAlbedo,
    VulkanTextureID texArrRoughness,
    VulkanTextureID texArrMetallic,
    VulkanTextureID texArrNormal)
{
	SetupUniformBuffer ();
	SetupImage ();
	SetupDescriptorSets (texArrAlbedo, texArrRoughness, texArrMetallic, texArrNormal);
	SetupPipeline ();

	quadMap.emplace (std::make_pair (FindEmptyIndex (),
	    TerrainQuad{ coordinateData.pos,
	        coordinateData.size,
	        coordinateData.noisePos,
	        coordinateData.noiseSize,
	        0,
	        cml::vec2i (0, 0),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0)),
	        this }));
	InitTerrainQuad (rootQuad, cameraPos);
}

void Terrain::UpdateTerrain (cml::vec3f viewerPos)
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
	mbo.model = cml::mat4f ().translate (cml::vec3f (coordinateData.pos.x, 0, coordinateData.pos.y));
	mbo.normal = cml::mat4f (); // mbo.model.inverse ().transpose ();
	uniformBuffer->CopyToBuffer (&mbo, sizeof (ModelBufferObject));

	instanceBuffer = std::make_shared<VulkanBufferInstancePersistant> (renderer.device, 2048, 16);
}

void Terrain::SetupImage ()
{

	int length = fastGraphUser.image_length ();
	auto buffer_height = std::make_shared<VulkanBufferStagingResource> (renderer.device,
	    sizeof (float) * fastGraphUser.GetHeightMap ().size (),
	    fastGraphUser.GetHeightMap ().data ());

	TexCreateDetails details (
	    VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8, length, length);
	details.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	terrainHeightMap = renderer.textureManager.CreateTextureFromBuffer (buffer_height, details);

	auto buffer_splat = std::make_shared<VulkanBufferStagingResource> (renderer.device,
	    sizeof (cml::vec4<uint8_t>) * fastGraphUser.GetSplatMap ().size (),
	    fastGraphUser.GetSplatMap ().data ());

	TexCreateDetails splat_details (
	    VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8, length, length);
	splat_details.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	terrainSplatMap = renderer.textureManager.CreateTextureFromBuffer (buffer_splat, splat_details);

	auto buffer_normal = std::make_shared<VulkanBufferStagingResource> (renderer.device,
	    sizeof (cml::vec4<int16_t>) * fastGraphUser.GetNormalMap ().size (),
	    fastGraphUser.GetNormalMap ().data ());

	TexCreateDetails norm_details (
	    VK_FORMAT_R16G16B16A16_SNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8, length, length);
	norm_details.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	terrainNormalMap = renderer.textureManager.CreateTextureFromBuffer (buffer_normal, norm_details);
}

void Terrain::SetupDescriptorSets (
    VulkanTextureID texArrAlbedo, VulkanTextureID texArrRoughness, VulkanTextureID texArrMetallic, VulkanTextureID texArrNormal)
{
	descriptor = renderer.GetVulkanDescriptor ();

	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7, 1));
	descriptor->SetupLayout (m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7));
	descriptor->SetupPool (poolSizes, 1);

	// VkDescriptorSetAllocateInfo allocInfoTerrain = initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	descriptorSet = descriptor->CreateDescriptorSet ();

	std::vector<DescriptorUse> writes;
	writes.push_back (DescriptorUse (0, 1, uniformBuffer->resource));
	writes.push_back (DescriptorUse (1, 1, renderer.textureManager.get_texture (terrainHeightMap).resource));
	writes.push_back (DescriptorUse (2, 1, renderer.textureManager.get_texture (terrainNormalMap).resource));
	writes.push_back (DescriptorUse (3, 1, renderer.textureManager.get_texture (terrainSplatMap).resource));
	writes.push_back (DescriptorUse (4, 1, renderer.textureManager.get_texture (texArrAlbedo).resource));
	writes.push_back (DescriptorUse (5, 1, renderer.textureManager.get_texture (texArrRoughness).resource));
	writes.push_back (DescriptorUse (6, 1, renderer.textureManager.get_texture (texArrMetallic).resource));
	writes.push_back (DescriptorUse (7, 1, renderer.textureManager.get_texture (texArrNormal).resource));
	descriptor->UpdateDescriptorSet (descriptorSet, writes);
}

void Terrain::SetupPipeline ()
{
	PipelineOutline out;

	auto vert = renderer.shaderManager.get_module ("terrain", ShaderType::vertex);
	auto frag = renderer.shaderManager.get_module ("terrain", ShaderType::fragment);

	ShaderModuleSet shader_set;
	shader_set.Vertex (vert.value ()).Fragment (frag.value ());
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
		    INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof (float) * 12) // Location pad
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

	normal = std::make_unique<Pipeline> (renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));

	out.SetRasterizer (
	    VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE, 1.0f, VK_TRUE);

	wireframe = std::make_unique<Pipeline> (
	    renderer, out, renderer.GetRelevantRenderpass (RenderableType::opaque));
}

void Terrain::InitTerrainQuad (int quad, cml::vec3f viewerPos)
{

	UpdateTerrainQuad (quad, viewerPos);
}


void Terrain::UpdateTerrainQuad (int quad, cml::vec3f viewerPos)
{

	float SubdivideDistanceBias = 2.0f;

	cml::vec3f center = cml::vec3f (quadMap.at (quad).pos.x + quadMap.at (quad).size.x / 2.0f,
	    quadMap.at (quad).heightValAtCenter,
	    quadMap.at (quad).pos.y + quadMap.at (quad).size.y / 2.0f);
	float distanceToViewer = cml::distance (viewerPos, center);

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

void Terrain::SubdivideTerrain (int quad, cml::vec3f viewerPos)
{
	auto& q = quadMap.at (quad);
	q.isSubdivided = true;
	numQuads += 4;

	cml::vec2f new_pos = cml::vec2f (q.pos.x, q.pos.y);
	cml::vec2f new_size = cml::vec2f (q.size.x / 2.0, q.size.y / 2.0);

	cml::vec2i new_lpos = cml::vec2i (q.logicalPos.x, q.logicalPos.y);
	cml::vec2f new_lsize = cml::vec2f (q.logicalSize.x / 2.0, q.logicalSize.y / 2.0);

	q.subQuads.UpRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.UpRight,
	    TerrainQuad (cml::vec2f (new_pos.x, new_pos.y),
	        new_size,
	        cml::vec2i (new_lpos.x, new_lpos.y),
	        new_lsize,
	        q.level + 1,
	        cml::vec2i (q.subDivPos.x * 2, q.subDivPos.y * 2),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2)),
	        this)));

	q.subQuads.UpLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.UpLeft,
	    TerrainQuad (cml::vec2f (new_pos.x, new_pos.y + new_size.y),
	        new_size,
	        cml::vec2i (new_lpos.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        q.level + 1,
	        cml::vec2i (q.subDivPos.x * 2, q.subDivPos.y * 2 + 1),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2 + 1)),
	        this)));

	q.subQuads.DownRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.DownRight,
	    TerrainQuad (cml::vec2f (new_pos.x + new_size.x, new_pos.y),
	        new_size,
	        cml::vec2i (new_lpos.x + new_lsize.x, new_lpos.y),
	        new_lsize,
	        q.level + 1,
	        cml::vec2i (q.subDivPos.x * 2 + 1, q.subDivPos.y * 2),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (
	                                 NumCells / 2, NumCells, q.level + 1, q.subDivPos.x * 2 + 1),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, q.level + 1, q.subDivPos.y * 2)),
	        this)));

	q.subQuads.DownLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (q.subQuads.DownLeft,
	    TerrainQuad (cml::vec2f (new_pos.x + new_size.x, new_pos.y + new_size.y),
	        new_size,
	        cml::vec2i (new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        q.level + 1,
	        cml::vec2i (q.subDivPos.x * 2 + 1, q.subDivPos.y * 2 + 1),
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

	instanceBuffer->CopyToBuffer (instances.data (), sizeof (HeightMapBound) * instances.size ());

	terrainGrid->BindModel (cmdBuf);
	instanceBuffer->BindInstanceBuffer (cmdBuf);

	vkCmdDrawIndexed (cmdBuf, static_cast<uint32_t> (indCount), instances.size (), 0, 0, 0);
}

float Terrain::GetHeightAtLocation (float x, float z)
{
	return fastGraphUser.SampleHeightMap (x, z) * heightScale;
}
