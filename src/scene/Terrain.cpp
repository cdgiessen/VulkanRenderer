#include "Terrain.h"
#include <glm/gtc/matrix_transform.hpp>

#include "core/Logger.h"

#include "rendering/RenderStructs.h"

#include "TerrainManager.h"

TerrainQuad::TerrainQuad (TerrainChunkBuffer& chunkBuffer,
    glm::vec2 pos,
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
  terrain (terrain),
  chunkBuffer (chunkBuffer)
{
}

TerrainQuad::~TerrainQuad ()
{
	if (index >= 0) // was setup
		chunkBuffer.Free (index);
}

void TerrainQuad::Setup ()
{
	index = chunkBuffer.Allocate ();

	vertices = (TerrainMeshVertices*)chunkBuffer.GetDeviceVertexBufferPtr (index);
	indices = (TerrainMeshIndices*)chunkBuffer.GetDeviceIndexBufferPtr (index);

	GenerateTerrainChunk (
	    std::ref (terrain->fastGraphUser), terrain->heightScale, terrain->coordinateData.size.x);
	chunkBuffer.SetChunkWritten (index);
	// quadSignal = chunkBuffer.GetChunkSignal(index);
}

float TerrainQuad::GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos)
{
	return glm::clamp (
	    (float)(i) / ((1 << level) * (float)(numCells)) + (float)(subDivPos) / (float)(1 << level), 0.0f, 1.0f);
}


glm::vec3 CalcNormal (
    double L, double R, double U, double D, double UL, double DL, double UR, double DR, double vertexDistance, int numCells)
{

	return glm::normalize (glm::vec3 (
	    L + UL + DL - (R + UR + DR), 2 * vertexDistance / numCells, U + UL + UR - (D + DL + DR)));
}


void RecalculateNormals (int numCells, TerrainMeshVertices* verts, TerrainMeshIndices* indices)
{
	int index = 0;
	for (int i = 0; i < indCount / 3; i++)
	{
		glm::vec3 p1 = glm::vec3 ((*verts)[(*indices)[i * 3 + 0] * vertElementCount + 0],
		    (*verts)[(*indices)[i * 3 + 0] * vertElementCount + 1],
		    (*verts)[(*indices)[i * 3 + 0] * vertElementCount + 2]);
		glm::vec3 p2 = glm::vec3 ((*verts)[(*indices)[i * 3 + 1] * vertElementCount + 0],
		    (*verts)[(*indices)[i * 3 + 1] * vertElementCount + 1],
		    (*verts)[(*indices)[i * 3 + 1] * vertElementCount + 2]);
		glm::vec3 p3 = glm::vec3 ((*verts)[(*indices)[i * 3 + 2] * vertElementCount + 0],
		    (*verts)[(*indices)[i * 3 + 2] * vertElementCount + 1],
		    (*verts)[(*indices)[i * 3 + 2] * vertElementCount + 2]);

		glm::vec3 t1 = p2 - p1;
		glm::vec3 t2 = p3 - p1;

		glm::vec3 normal (glm::cross (t1, t2));

		(*verts)[(*indices)[i * 3 + 0] * vertElementCount + 3] += normal.x;
		(*verts)[(*indices)[i * 3 + 0] * vertElementCount + 4] += normal.y;
		(*verts)[(*indices)[i * 3 + 0] * vertElementCount + 5] += normal.z;
		(*verts)[(*indices)[i * 3 + 1] * vertElementCount + 3] += normal.x;
		(*verts)[(*indices)[i * 3 + 1] * vertElementCount + 4] += normal.y;
		(*verts)[(*indices)[i * 3 + 1] * vertElementCount + 5] += normal.z;
		(*verts)[(*indices)[i * 3 + 2] * vertElementCount + 3] += normal.x;
		(*verts)[(*indices)[i * 3 + 2] * vertElementCount + 4] += normal.y;
		(*verts)[(*indices)[i * 3 + 2] * vertElementCount + 5] += normal.z;
	}

	for (int i = 0; i < (numCells + 1) * (numCells + 1); i++)
	{
		glm::vec3 normal = glm::normalize (glm::vec3 ((*verts)[i * vertElementCount + 3],
		    (*verts)[i * vertElementCount + 4],
		    (*verts)[i * vertElementCount + 5]));
		(*verts)[i * vertElementCount + 3] = normal.x;
		(*verts)[i * vertElementCount + 4] = normal.y;
		(*verts)[i * vertElementCount + 5] = normal.z;
	}
}

void TerrainQuad::GenerateTerrainChunk (InternalGraph::GraphUser& graphUser, float heightScale, float widthScale)
{

	const int numCells = NumCells;

	float uvUs[numCells + 3];
	float uvVs[numCells + 3];

	int powLevel = 1 << (level);
	for (int i = 0; i < numCells + 3; i++)
	{
		uvUs[i] = glm::clamp (
		    (float)(i - 1) / ((float)(powLevel) * (numCells)) + (float)subDivPos.x / (float)(powLevel), 0.0f, 1.0f);
		uvVs[i] = glm::clamp (
		    (float)(i - 1) / ((float)(powLevel) * (numCells)) + (float)subDivPos.y / (float)(powLevel), 0.0f, 1.0f);
	}

	float hDiff = uvUs[3] - uvUs[1];

	for (int i = 0; i < numCells + 1; i++)
	{
		for (int j = 0; j < numCells + 1; j++)
		{
			float uvU = uvUs[(i + 1)];
			float uvV = uvVs[(j + 1)];

			float uvUminus = uvUs[(i + 1) - 1];
			float uvUplus = uvUs[(i + 1) + 1];
			float uvVminus = uvVs[(j + 1) - 1];
			float uvVplus = uvVs[(j + 1) + 1];

			float outheight = graphUser.SampleHeightMap (uvU, uvV);
			float outheightum = graphUser.SampleHeightMap (uvUminus, uvV);
			float outheightup = graphUser.SampleHeightMap (uvUplus, uvV);
			float outheightvm = graphUser.SampleHeightMap (uvU, uvVminus);
			float outheightvp = graphUser.SampleHeightMap (uvU, uvVplus);

			glm::vec3 normal =
			    glm::normalize (glm::vec3 ((outheightvm - outheightvp) / hDiff,
			                        16.0f /*((uvUplus - uvUminus) + (uvVplus - uvVminus)) * 2*/,
			                        (outheightum - outheightup)) /
			                    hDiff);

			/*float y = graphUser.SampleHeightMap(uvU, uvV);
			float um = graphUser.SampleHeightMap(uvUminus, uvV);
			float up = graphUser.SampleHeightMap(uvUplus, uvV);
			float vm = graphUser.SampleHeightMap(uvU, uvVminus);
			float vp = graphUser.SampleHeightMap(uvU, uvVplus);

			float midU = (um + up) / 2.0;
			float midV = (vm + vp) / 2.0;*/


			// float outheightum = graphUser.SampleHeightMap(uvU - 0.01, uvV);
			// float outheightup = graphUser.SampleHeightMap(uvU + 0.01, uvV);
			// float outheightvm = graphUser.SampleHeightMap(uvU, uvV - 0.01);

			// deduce terrain normal
			/*glm::vec3 normal;
			normal.x = outheightum - outheightup;
			normal.z = outheightvm - outheightvp;
			normal.y = hDiff * 2;
			normal = glm::normalize(normal);
*/
			// float o0 = graphUser.SampleHeightMap(uvUminus,uvVminus);
			// float o1 = graphUser.SampleHeightMap(uvU, uvVminus);
			// float o2 = graphUser.SampleHeightMap(uvUplus, uvVminus);
			// float o3 = graphUser.SampleHeightMap(uvUminus, uvV);
			// float outheight = graphUser.SampleHeightMap(uvU, uvV);
			// float o5 = graphUser.SampleHeightMap(uvUplus, uvV);
			// float o6 = graphUser.SampleHeightMap(uvUminus, uvVplus);
			// float o7 = graphUser.SampleHeightMap(uvU, uvVplus);
			// float o8 = graphUser.SampleHeightMap(uvUplus, uvVplus);

			//////float s[9] contains above samples
			// float scaleX = uvUs[2] - uvUs[1];
			// float scaleZ = uvVs[2] - uvVs[1];
			// glm::vec3 normal = glm::vec3(0,1,0);
			// normal.x = 10 * -(o2-o0+2*(o5-o3)+o8-o6);
			// normal.z = 10 * -(o6-o0+2*(o7-o1)+o8-o2);
			// normal.y = 1.0;
			// normal = glm::normalize(normal);

			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 0] = uvU * (widthScale);
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 1] = outheight * heightScale;
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 2] = uvV * (widthScale);
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 3] = normal.x;
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 4] = normal.y;
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 5] = normal.z;
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 6] = uvU;
			(*vertices)[((i) * (numCells + 1) + j) * vertElementCount + 7] = uvV;
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



	// RecalculateNormals(numCells, vertices, indices);
}

Terrain::Terrain (VulkanRenderer& renderer,
    TerrainChunkBuffer& chunkBuffer,
    InternalGraph::GraphPrototype& protoGraph,
    int numCells,
    int maxLevels,
    float heightScale,
    TerrainCoordinateData coords)
: renderer (renderer),
  chunkBuffer (chunkBuffer),
  maxLevels (maxLevels),
  heightScale (heightScale),
  coordinateData (coords),
  fastGraphUser (protoGraph, 1337, coords.sourceImageResolution, coords.noisePos, coords.noiseSize.x)

{

	// simple calculation right now, does the absolute max number of quads possible with given max
	// level in future should calculate the actual number of max quads, based on distance calculation
	if (maxLevels <= 0)
	{
		maxNumQuads = 1;
	}
	else
	{
		// with current quad density this is the average upper bound (kidna a guess but its probably more than enough for now (had to add 25 cause it wasn't enough lol!)
		maxNumQuads = 1 + 16 + 20 + 25 + 50 * maxLevels;
		// maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads (like if everything was subdivided)
	}


	splatMapData = fastGraphUser.GetSplatMapPtr ();
	splatMapSize = (int)glm::pow (coords.sourceImageResolution, 2);
}

Terrain::~Terrain () {}

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
	    TerrainQuad{ chunkBuffer,
	        coordinateData.pos,
	        coordinateData.size,
	        coordinateData.noisePos,
	        coordinateData.noiseSize,
	        0,
	        glm::i32vec2 (0, 0),
	        GetHeightAtLocation (TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0),
	            TerrainQuad::GetUVvalueFromLocalIndex (NumCells / 2, NumCells, 0, 0)),
	        this }));
	quadMap.at (rootQuad).Setup ();
	InitTerrainQuad (rootQuad, cameraPos);
}

void Terrain::UpdateTerrain (glm::vec3 viewerPos)
{
	SimpleTimer updateTime;
	updateTime.StartTimer ();

	bool shouldUpdateBuffers = UpdateTerrainQuad (rootQuad, viewerPos);

	updateTime.EndTimer ();

	// if (updateTime.GetElapsedTimeMicroSeconds() > 1500)
	//	Log::Debug << " Update time " << updateTime.GetElapsedTimeMicroSeconds() << "\n";
}




void Terrain::SetupUniformBuffer ()
{

	uniformBuffer = std::make_shared<VulkanBufferUniform> (renderer.device, sizeof (ModelBufferObject));

	ModelBufferObject mbo;
	mbo.model = glm::mat4 (1.0f);
	mbo.model = glm::translate (mbo.model, glm::vec3 (coordinateData.pos.x, 0, coordinateData.pos.y));
	mbo.normal = glm::transpose (glm::inverse (mbo.model));
	uniformBuffer->CopyToBuffer (&mbo, sizeof (ModelBufferObject));
}

void Terrain::SetupImage ()
{
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
		throw std::runtime_error ("failed to get terrain splat map data!");
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
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1));
	m_bindings.push_back (VulkanDescriptor::CreateBinding (
	    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1));
	descriptor->SetupLayout (m_bindings);

	std::vector<DescriptorPoolSize> poolSizes;
	poolSizes.push_back (DescriptorPoolSize (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
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
	writes.push_back (DescriptorUse (1, 1, terrainVulkanSplatMap->resource));
	writes.push_back (DescriptorUse (2, 1, terrainVulkanTextureArrayAlbedo->resource));
	writes.push_back (DescriptorUse (3, 1, terrainVulkanTextureArrayRoughness->resource));
	writes.push_back (DescriptorUse (4, 1, terrainVulkanTextureArrayMetallic->resource));
	writes.push_back (DescriptorUse (5, 1, terrainVulkanTextureArrayNormal->resource));
	descriptor->UpdateDescriptorSet (descriptorSet, writes);
}

void Terrain::SetupPipeline ()
{
	PipelineOutline out;

	auto vert = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/terrain.vert.spv", ShaderModuleType::vertex);
	auto frag = renderer.shaderManager.loadShaderModule (
	    "assets/shaders/terrain.frag.spv", ShaderModuleType::fragment);
	out.SetShaderModuleSet (ShaderModuleSet (vert, frag));

	VertexLayout layout (VertexDescription ({ 3, 3, 2 }));
	out.AddVertexLayouts (layout.bindingDesc, layout.attribDesc);

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

void Terrain::InitTerrainQuad (int quad, glm::vec3 viewerPos)
{

	UpdateTerrainQuad (quad, viewerPos);
}


bool Terrain::UpdateTerrainQuad (int quad, glm::vec3 viewerPos)
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
			return true;
		}
	}

	else if (distanceToViewer > quadMap.at (quad).size.x * SubdivideDistanceBias)
	{
		UnSubdivide (quad);
		return true;
	}
	else
	{
		bool uR = UpdateTerrainQuad (quadMap.at (quad).subQuads.UpRight, viewerPos);
		bool uL = UpdateTerrainQuad (quadMap.at (quad).subQuads.UpLeft, viewerPos);
		bool dR = UpdateTerrainQuad (quadMap.at (quad).subQuads.DownRight, viewerPos);
		bool dL = UpdateTerrainQuad (quadMap.at (quad).subQuads.DownLeft, viewerPos);

		if (uR || uL || dR || dL) return true;
	}
	return false;
}

void Terrain::SubdivideTerrain (int quad, glm::vec3 viewerPos)
{
	quadMap.at (quad).isSubdivided = true;
	numQuads += 4;

	glm::vec2 new_pos = glm::vec2 (quadMap.at (quad).pos.x, quadMap.at (quad).pos.y);
	glm::vec2 new_size = glm::vec2 (quadMap.at (quad).size.x / 2.0, quadMap.at (quad).size.y / 2.0);

	glm::i32vec2 new_lpos =
	    glm::i32vec2 (quadMap.at (quad).logicalPos.x, quadMap.at (quad).logicalPos.y);
	glm::i32vec2 new_lsize =
	    glm::i32vec2 (quadMap.at (quad).logicalSize.x / 2.0, quadMap.at (quad).logicalSize.y / 2.0);

	quadMap.at (quad).subQuads.UpRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (quadMap.at (quad).subQuads.UpRight,
	    TerrainQuad (chunkBuffer,
	        glm::vec2 (new_pos.x, new_pos.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x, new_lpos.y),
	        new_lsize,
	        quadMap.at (quad).level + 1,
	        glm::i32vec2 (quadMap.at (quad).subDivPos.x * 2, quadMap.at (quad).subDivPos.y * 2),
	        GetHeightAtLocation (
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.y * 2)),
	        this)));
	quadMap.at (quadMap.at (quad).subQuads.UpRight).Setup ();

	quadMap.at (quad).subQuads.UpLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (quadMap.at (quad).subQuads.UpLeft,
	    TerrainQuad (chunkBuffer,
	        glm::vec2 (new_pos.x, new_pos.y + new_size.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        quadMap.at (quad).level + 1,
	        glm::i32vec2 (quadMap.at (quad).subDivPos.x * 2, quadMap.at (quad).subDivPos.y * 2 + 1),
	        GetHeightAtLocation (
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.x * 2),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.y * 2 + 1)),
	        this)));
	quadMap.at (quadMap.at (quad).subQuads.UpLeft).Setup ();

	quadMap.at (quad).subQuads.DownRight = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (quadMap.at (quad).subQuads.DownRight,
	    TerrainQuad (chunkBuffer,
	        glm::vec2 (new_pos.x + new_size.x, new_pos.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x + new_lsize.x, new_lpos.y),
	        new_lsize,
	        quadMap.at (quad).level + 1,
	        glm::i32vec2 (quadMap.at (quad).subDivPos.x * 2 + 1, quadMap.at (quad).subDivPos.y * 2),
	        GetHeightAtLocation (
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.x * 2 + 1),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.y * 2)),
	        this)));
	quadMap.at (quadMap.at (quad).subQuads.DownRight).Setup ();

	quadMap.at (quad).subQuads.DownLeft = FindEmptyIndex ();
	quadMap.emplace (std::make_pair (quadMap.at (quad).subQuads.DownLeft,
	    TerrainQuad (chunkBuffer,
	        glm::vec2 (new_pos.x + new_size.x, new_pos.y + new_size.y),
	        new_size,
	        glm::i32vec2 (new_lpos.x + new_lsize.x, new_lpos.y + new_lsize.y),
	        new_lsize,
	        quadMap.at (quad).level + 1,
	        glm::i32vec2 (quadMap.at (quad).subDivPos.x * 2 + 1, quadMap.at (quad).subDivPos.y * 2 + 1),
	        GetHeightAtLocation (
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.x * 2 + 1),
	            TerrainQuad::GetUVvalueFromLocalIndex (
	                NumCells / 2, NumCells, quadMap.at (quad).level + 1, quadMap.at (quad).subDivPos.y * 2 + 1)),
	        this)));
	quadMap.at (quadMap.at (quad).subQuads.DownLeft).Setup ();

	InitTerrainQuad (quadMap.at (quad).subQuads.UpRight, viewerPos);
	InitTerrainQuad (quadMap.at (quad).subQuads.UpLeft, viewerPos);
	InitTerrainQuad (quadMap.at (quad).subQuads.DownRight, viewerPos);
	InitTerrainQuad (quadMap.at (quad).subQuads.DownLeft, viewerPos);
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

void Terrain::PopulateQuadOffsets (int quad, std::vector<VkDeviceSize>& vert, std::vector<VkDeviceSize>& ind)
{
	if (quadMap.at (quad).isSubdivided)
	{
		PopulateQuadOffsets (quadMap.at (quad).subQuads.UpRight, vert, ind);
		PopulateQuadOffsets (quadMap.at (quad).subQuads.UpLeft, vert, ind);
		PopulateQuadOffsets (quadMap.at (quad).subQuads.DownRight, vert, ind);
		PopulateQuadOffsets (quadMap.at (quad).subQuads.DownLeft, vert, ind);
	}
	else
	{
		vert.push_back (quadMap.at (quad).index * sizeof (TerrainMeshVertices));
		ind.push_back (quadMap.at (quad).index * sizeof (TerrainMeshIndices));
	}
}

void Terrain::DrawDepthPrePass (VkCommandBuffer cmdBuff)
{
	VkDeviceSize offsets[] = { 0 };

	// if (!terrainVulkanSplatMap->readyToUse)
	//	return;

	std::vector<VkDeviceSize> vertexOffsettings;
	std::vector<VkDeviceSize> indexOffsettings;

	PopulateQuadOffsets (rootQuad, vertexOffsettings, indexOffsettings);

	for (int i = 0; i < vertexOffsettings.size (); i++)
	{
		vkCmdBindVertexBuffers (cmdBuff, 0, 1, &chunkBuffer.vert_buffer.buffer.buffer, &vertexOffsettings[i]);
		vkCmdBindIndexBuffer (cmdBuff, chunkBuffer.index_buffer.buffer.buffer, indexOffsettings[i], VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed (cmdBuff, static_cast<uint32_t> (indCount), 1, 0, 0, 0);
	}
}

void Terrain::DrawTerrain (VkCommandBuffer cmdBuff, bool ifWireframe)
{
	VkDeviceSize offsets[] = { 0 };
	// return;
	if (*terrainVulkanSplatMap->readyToUse == false) return;

	drawTimer.StartTimer ();

	std::vector<VkDeviceSize> vertexOffsettings;
	std::vector<VkDeviceSize> indexOffsettings;

	PopulateQuadOffsets (rootQuad, vertexOffsettings, indexOffsettings);

	/*vkCmdPushConstants(
	    cmdBuff,
	    mvp->layout,
	    VK_SHADER_STAGE_VERTEX_BIT,
	    0,
	    sizeof(TerrainPushConstant),
	    &modelMatrixData);*/

	if (ifWireframe)
		wireframe->Bind (cmdBuff);
	else
		normal->Bind (cmdBuff);

	// vkCmdBindPipeline (cmdBuff,
	//    VK_PIPELINE_BIND_POINT_GRAPHICS,
	//    ifWireframe ? mvp->pipelines->at (1) : mvp->pipelines->at (0));
	vkCmdBindDescriptorSets (
	    cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, normal->GetLayout (), 2, 1, &descriptorSet.set, 0, nullptr);

	for (int i = 0; i < vertexOffsettings.size (); i++)
	{
		vkCmdBindVertexBuffers (cmdBuff, 0, 1, &chunkBuffer.vert_buffer.buffer.buffer, &vertexOffsettings[i]);
		vkCmdBindIndexBuffer (cmdBuff, chunkBuffer.index_buffer.buffer.buffer, indexOffsettings[i], VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed (cmdBuff, static_cast<uint32_t> (indCount), 1, 0, 0, 0);
	}


	drawTimer.EndTimer ();
}

float Terrain::GetHeightAtLocation (float x, float z)
{

	return fastGraphUser.SampleHeightMap (x, z) * heightScale;
}