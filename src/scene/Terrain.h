#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>


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
	glm::vec2 pos;
	glm::vec2 size;
	glm::i32vec2 noisePos;
	glm::vec2 noiseSize;
	int sourceImageResolution;
	glm::ivec2 gridPos;
	TerrainCoordinateData (
	    glm::vec2 pos, glm::vec2 size, glm::i32vec2 noisePos, glm::vec2 noiseSize, int imageRes, glm::ivec2 gridPos)
	: pos (pos), size (size), noisePos (noisePos), noiseSize (noiseSize), sourceImageResolution (imageRes), gridPos (gridPos)
	{
	}
};

class Terrain;

static int heightmapboundsize = 16;
struct HeightMapBound
{
	glm::vec4 pos;
	glm::vec4 uv;
	glm::vec4 h_w_pp;
	glm::vec4 dumb;
};

struct TerrainQuad
{
	TerrainQuad (glm::vec2 pos,
	    glm::vec2 size,
	    glm::i32vec2 logicalPos,
	    glm::i32vec2 logicalSize,
	    int level,
	    glm::i32vec2 subDivPos,
	    float centerHeightValue,
	    Terrain* terrain);

	static float GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos);

	glm::vec2 pos;            // position of corner
	glm::vec2 size;           // width and length
	glm::i32vec2 logicalPos;  // where in the proc-gen space it is (for noise images)
	glm::i32vec2 logicalSize; // how wide the area is.
	glm::i32vec2 subDivPos;   // where in the subdivision grid it is (for splatmap)
	int level = 0;            // how deep the quad is
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

	TerrainCoordinateData coordinateData;
	float heightScale = 100;

	VulkanRenderer& renderer;

	std::unique_ptr<Pipeline> normal;
	std::unique_ptr<Pipeline> wireframe;

	std::shared_ptr<VulkanDescriptor> descriptor;

	DescriptorSet descriptorSet;

	std::shared_ptr<VulkanTexture> terrainHeightMap;
	std::shared_ptr<VulkanTexture> terrainSplatMap;
	std::shared_ptr<VulkanTexture> terrainNormalMap;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;

	std::shared_ptr<VulkanBufferInstancePersistant> instanceBuffer;

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

	void InitTerrain (glm::vec3 cameraPos,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal);

	void UpdateTerrain (glm::vec3 viewerPos);

	void DrawTerrainRecursive (
	    int quad, VkCommandBuffer cmdBuf, bool ifWireframe, std::vector<HeightMapBound>& instances);
	void DrawTerrainGrid (VkCommandBuffer cmdBuf, bool ifWireframe);
	// std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation (float x, float z);

	private:
	int curEmptyIndex = 0;
	int FindEmptyIndex ();

	void InitTerrainQuad (int quad, glm::vec3 viewerPos);

	void UpdateTerrainQuad (int quad, glm::vec3 viewerPos);

	void SetupUniformBuffer ();
	void SetupImage ();
	void SetupPipeline ();

	void SetupDescriptorSets (std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal);

	void SubdivideTerrain (int quad, glm::vec3 viewerPos);
	void UnSubdivide (int quad);

	// void PopulateQuadOffsets (int quad, std::vector<VkDeviceSize>& vert, std::vector<VkDeviceSize>& ind);
};
