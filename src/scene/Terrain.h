#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cml/cml.h>

#include <vulkan/vulkan.h>

#include "cml/cml.h"

#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"

#include "core/CoreTools.h"
#include "resources/Texture.h"

#include "gui/InternalGraph.h"


const int NumCells = 64;
const int indCount = NumCells * NumCells * 6;

struct TerrainCoordinateData
{
	cml::vec2f pos;
	cml::vec2f size;
	cml::vec2i noisePos;
	cml::vec2f noiseSize;
	int sourceImageResolution;
	cml::vec2i gridPos;
	TerrainCoordinateData (
	    cml::vec2f pos, cml::vec2f size, cml::vec2i noisePos, cml::vec2f noiseSize, int imageRes, cml::vec2i gridPos)
	: pos (pos), size (size), noisePos (noisePos), noiseSize (noiseSize), sourceImageResolution (imageRes), gridPos (gridPos)
	{
	}
};

class Terrain;

struct HeightMapBound
{
	cml::vec4f pos;
	cml::vec4f uv;
	cml::vec4f h_w_pp;
	cml::vec4f dumb;
};

struct TerrainQuad
{
	TerrainQuad (cml::vec2f pos,
	    cml::vec2f size,
	    cml::vec2i logicalPos,
	    cml::vec2f logicalSize,
	    int level,
	    cml::vec2i subDivPos,
	    float centerHeightValue,
	    Terrain* terrain);

	static float GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos);

	cml::vec2f pos;         // position of corner
	cml::vec2f size;        // width and length
	cml::vec2i logicalPos;  // where in the proc-gen space it is (for noise images)
	cml::vec2f logicalSize; // how wide the area is.
	cml::vec2i subDivPos;   // where in the subdivision grid it is (for splatmap)
	int level = 0;          // how deep the quad is
	float heightValAtCenter = 0;
	bool isSubdivided = false;

	Terrain* terrain; // who owns it
	int index = -1;   // index into chunkBuffer

	HeightMapBound bound;

	// index into terrain's quadMap
	struct SubQuads
	{
		int UpLeft = -1;
		int DownLeft = -1;
		int UpRight = -1;
		int DownRight = -1;
	} subQuads;
};

class Terrain
{
	public:
	std::unordered_map<int, TerrainQuad> quadMap;

	int rootQuad = 0;

	int maxLevels;
	int maxNumQuads;
	int numQuads = 1;

	VulkanRenderer& renderer;
	TerrainCoordinateData coordinateData;
	float heightScale = 100;


	std::unique_ptr<Pipeline> normal;
	std::unique_ptr<Pipeline> wireframe;

	std::shared_ptr<VulkanDescriptor> descriptor;

	DescriptorSet descriptorSet;

	VulkanTextureID terrainHeightMap;
	VulkanTextureID terrainSplatMap;
	VulkanTextureID terrainNormalMap;

	std::shared_ptr<VulkanBuffer> uniformBuffer;

	std::shared_ptr<VulkanBuffer> instanceBuffer;

	VulkanModel* terrainGrid;


	InternalGraph::GraphUser fastGraphUser;

	SimpleTimer drawTimer;

	Terrain (VulkanRenderer& renderer,
	    InternalGraph::GraphPrototype& protoGraph,
	    int numCells,
	    int maxLevels,
	    float heightScale,
	    TerrainCoordinateData coordinateData,
	    VulkanModel* terrainGrid);

	void InitTerrain (cml::vec3f cameraPos,
	    VulkanTextureID terrainVulkanTextureArrayAlbedo,
	    VulkanTextureID terrainVulkanTextureArrayRoughness,
	    VulkanTextureID terrainVulkanTextureArrayMetallic,
	    VulkanTextureID terrainVulkanTextureArrayNormal);

	void UpdateTerrain (cml::vec3f viewerPos);

	void DrawTerrainRecursive (
	    int quad, VkCommandBuffer cmdBuf, bool ifWireframe, std::vector<HeightMapBound>& instances);
	void DrawTerrainGrid (VkCommandBuffer cmdBuf, bool ifWireframe);
	// std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation (float x, float z);

	private:
	int curEmptyIndex = 0;
	int FindEmptyIndex ();

	void InitTerrainQuad (int quad, cml::vec3f viewerPos);

	void UpdateTerrainQuad (int quad, cml::vec3f viewerPos);

	void SetupUniformBuffer ();
	void SetupImage ();
	void SetupPipeline ();

	void SetupDescriptorSets (VulkanTextureID terrainVulkanTextureArrayAlbedo,
	    VulkanTextureID terrainVulkanTextureArrayRoughness,
	    VulkanTextureID terrainVulkanTextureArrayMetallic,
	    VulkanTextureID terrainVulkanTextureArrayNormal);

	void SubdivideTerrain (int quad, cml::vec3f viewerPos);
	void UnSubdivide (int quad);

	// void PopulateQuadOffsets (int quad, std::vector<VkDeviceSize>& vert, std::vector<VkDeviceSize>& ind);
};
