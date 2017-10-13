#pragma once

#include <string>
#include <vector>
#include <thread>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan\vulkan.h>

#include "..\vulkan\VulkanDevice.hpp"
#include "..\vulkan\VulkanModel.hpp"
#include "..\vulkan\VulkanPipeline.hpp"
#include "..\vulkan\VulkanTexture.hpp"
#include "..\vulkan\VulkanInitializers.hpp"

#include "..\core\Texture.h"

#include "..\core\MemoryPool.h"

#include "TerrainGenerator.h"
#include "..\gui\TerGenNodeGraph.h"

const int SplatMapSize = 1024;
const int NumCells = 64;
const int vertCount = (NumCells + 1) * (NumCells + 1);
const int indCount = NumCells * NumCells * 6;
const int vertElementCount = 12;

typedef std::array<float, vertCount * vertElementCount> TerrainMeshVertices;
typedef std::array<uint32_t, indCount> TerrainMeshIndices;

enum Corner_Enum {
	uR = 0,
	uL = 1,
	dR = 2,
	dL = 3
};

class TerrainQuad {
public:
	glm::vec2 pos; //position of corner
	glm::vec2 size; //width and length
	glm::i32vec2 logicalPos; //where in the proc-gen space it is (for noise images)
	glm::i32vec2 logicalSize; //how wide the area is.
	glm::i32vec2 subDivPos; //where in the subdivision grid it is (for splatmap)
	int level; //how deep the quad is
	float heightValAtCenter = 0;
	bool isSubdivided;
	
	ModelBufferObject modelUniformObject;

	TerrainQuad();
	~TerrainQuad();

	//puts the position, size, and level into the class
	void init(glm::vec2 pos, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos, float centerHeightValue);

};

struct TerrainQuadData {
	TerrainQuad terrainQuad;
	VkDescriptorSet descriptorSet;
	VkDeviceMemory vertexOffset;
	VkDeviceMemory indexOffset;
	TerrainMeshVertices vertices;
	TerrainMeshIndices indices;

	struct SubQuads {
		TerrainQuadData* UpLeft;
		TerrainQuadData* DownLeft;
		TerrainQuadData* UpRight;
		TerrainQuadData* DownRight;
	} subQuads;
};

class Terrain {
public:
	MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>* terrainQuads;
	std::vector<TerrainQuadData*> quadHandles;
	std::vector<TerrainQuadData*> PrevQuadHandles;
	std::vector<TerrainMeshVertices> verts;
	std::vector<TerrainMeshIndices> inds;

	TerrainQuadData* rootQuad;
	int maxLevels;
	int maxNumQuads;
	int numQuads = 0;

	glm::vec2 position;
	glm::vec2 size;
	glm::i32vec2 noisePosition;
	glm::i32vec2 noiseSize;
	float heightScale = 100;

	VulkanDevice *device;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipeline wireframe;
	VkPipeline debugNormals;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;

	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;

	Texture* terrainSplatMap;
	VulkanTexture2D terrainVulkanSplatMap;

	TextureArray* terrainTextureArray;
	VulkanTexture2DArray terrainVulkanTextureArray;

	VulkanBuffer modelUniformBuffer;

	TerrainGenerator* terrainGenerator;
	NewNodeGraph::TerGenNodeGraph* fastTerrainGraph;

	SimpleTimer drawTimer;

	std::vector<std::thread *> terrainGenerationWorkers;

	Terrain(MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>* pool, int numCells, int maxLevels, float heightScale, glm::vec2 pos, glm::vec2 size, glm::i32vec2 noisePosition, glm::i32vec2 noiseSize);
	~Terrain();

	void InitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, VkQueue copyQueue, 
		uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting, glm::vec3 cameraPos);
	void ReinitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
	void UpdateTerrain(glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);
	void DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], Terrain* curTerrain, bool wireframe);
	void BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe);
	void CleanUp();

	void LoadSplatMapFromGenerator();
	void LoadTextureArray();
private:

	TerrainQuadData* InitTerrainQuad(TerrainQuadData* q, glm::vec2 position, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, VulkanBuffer &gbo, VulkanBuffer &lbo);
	TerrainQuadData* InitTerrainQuadFromParent(TerrainQuadData* parent, TerrainQuadData* q, Corner_Enum corner, 
		glm::vec2 position, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, VulkanBuffer &gbo, VulkanBuffer &lbo, glm::i32vec2 subDivPos);

	bool UpdateTerrainQuad(TerrainQuadData* quad, glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);

	void SetupMeshbuffers();
	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();
	void SetupPipeline(VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);

	void SetupDescriptorLayoutAndPool();

	void UpdateModelBuffer(VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);

	void UploadMeshBuffer(VkQueue copyQueue);
	void UpdateMeshBuffer(VkQueue copyQueue);

	void UpdateUniformBuffer(float time);
	void SubdivideTerrain(TerrainQuadData* quad, VkQueue copyQueue, glm::vec3 viewerPos, VulkanBuffer &gbo, VulkanBuffer &lbo);
	void UnSubdivide(TerrainQuadData* quad);

	std::vector<std::string> texFileNames = {
		"dirt.jpg",
		"grass.jpg",
		"rockSmall.jpg",
		"Snow.jpg",
	};
	// Extra splatmap images, starting with just 4 for now
	//	"OakTreeLeaf.png",
	//	"Sand.png",
	//	"DeadOakTreeTrunk.png",
	//	"OakTreeTrunk.png",
	//	"SpruceTreeTrunk.png"};
	//
};

//mesh generation functions. Looks at all those parameters. 
void GenerateNewTerrain(TerrainGenerator *terrainGenerator, NewNodeGraph::TerGenNodeGraph* fastGraph, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, float heightScale, int maxSubDivLevels);

//Like GenerateNewTerrain but has corrected texture coordinates for subdivisions. Best to leave that function alone.
void GenerateNewTerrainSubdivision(TerrainGenerator *terrainGenerator, NewNodeGraph::TerGenNodeGraph* fastGraph, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, Corner_Enum corner, float heightScale, int maxSubDivLevels);

//not used as it depends on previous terrains, which is great for runtime but not for first generation (since it has dependence on its parents mesh being ready)
void GenerateTerrainFromExisting(TerrainGenerator *terrainGenerator, NewNodeGraph::TerGenNodeGraph* fastGraph, TerrainMeshVertices *parentVerts, TerrainMeshIndices *parentIndices,
	TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner, TerrainQuad terrainQuad, float heightScale, int maxSubDivLevels);