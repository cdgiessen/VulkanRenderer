#pragma once

#include <string>
#include <vector>
#include <thread>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan\vulkan.h>
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanPipeline.h"
#include "VulkanInitializers.hpp"
#include "VulkanTools.h"

#include "Texture.h"

#include "MemoryPool.h"

#include "TerrainGenerator.h"

const int SplatMapSize = 1024;
const int NumCells = 50;
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
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	int level; //how deep the quad is
	int subDivPosX, subDivPosZ; //where in the subdivision grid it is (for splatmap)
	float heightValAtCenter = 0;
	bool isSubdivided;

	ModelBufferObject modelUniformObject;

	TerrainQuad();
	~TerrainQuad();

	//puts the position, size, and level into the class
	void init(float posX, float posY, float sizeX, float sizeY, int level, int subDivPosX, int subDivPosZ, float centerHeightValue);

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

	glm::vec3 position;
	glm::vec3 size;
	float heightScale = 1000;

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

	SimpleTimer drawTimer;

	std::vector<std::thread *> terrainGenerationWorkers;

	Terrain(MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>* pool, int numCells, int maxLevels, float posX, float posY, float sizeX, float sizeY);
	~Terrain();

	void InitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting, glm::vec3 cameraPos);
	void ReinitTerrain(VulkanDevice* device, VulkanPipeline pipelineManager, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
	void UpdateTerrain(glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);
	void DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], Terrain* curTerrain, bool wireframe);
	void BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe);
	void CleanUp();

	void LoadSplatMapFromGenerator();
	void LoadTextureArray();
private:

	TerrainQuadData* InitTerrainQuad(TerrainQuadData* q, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo);
	TerrainQuadData* InitTerrainQuadFromParent(TerrainQuadData* parent, TerrainQuadData* q, Corner_Enum corner, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo, int subDivPosX, int subDivPosZ);

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
void GenerateNewTerrain(TerrainGenerator *terrainGenerator, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, float heightScale);

//Like GenerateNewTerrain but has corrected texture coordinates for subdivisions. Best to leave that function alone.
void GenerateNewTerrainSubdivision(TerrainGenerator *terrainGenerator, TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad, Corner_Enum corner, float heightScale);

//not used as it depends on previous terrains, which is great for runtime but not for first generation (since it has dependence on its parents mesh being ready)
void GenerateTerrainFromExisting(TerrainGenerator &terrainGenerator, TerrainMeshVertices &parentVerts, TerrainMeshIndices &parentIndices,
	TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner, TerrainQuad terrainQuad, float heightScale);