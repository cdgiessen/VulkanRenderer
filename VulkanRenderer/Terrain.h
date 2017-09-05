#pragma once

#include <string>
#include <vector>

#include <glm\common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan\vulkan.h>
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"
#include "VulkanBuffer.hpp"

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

	bool isSubdivided;

	ModelBufferObject modelUniformObject;

	TerrainQuad();
	~TerrainQuad();

	//puts the position, size, and level into the class
	void init(float posX, float posY, float sizeX, float sizeY, int level, int subDivPosX, int subDivPosZ);

};

struct TerrainQuadData {
	TerrainQuad terrainQuad;
	VkDescriptorSet descriptorSet;
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

	TerrainQuadData* rootQuad;
	int maxLevels;
	int maxNumQuads;
	int numQuads = 0;

	glm::vec3 position;
	glm::vec3 size;
	float heightScale = 100;

	VulkanDevice *device;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkPipeline wireframe;

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

	Terrain(int numCells, int maxLevels, float posX, float posY, float sizeX, float sizeY);
	~Terrain();

	void InitTerrain(VulkanDevice* device, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting, glm::vec3 cameraPos);
	void ReinitTerrain(VulkanDevice* device, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);
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

	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();
	void SetupPipeline(VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight);

	void SetupDescriptorLayoutAndPool();

	void UpdateModelBuffer(VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);

	void UploadMeshBuffer(VkQueue copyQueue);
	void UpdateMeshBuffer(VkQueue copyQueue);

	void UpdateUniformBuffer(float time);
	void SubdivideTerrain(TerrainQuadData* quad, VkQueue copyQueue, glm::vec3 viewerPos, VulkanBuffer &gbo, VulkanBuffer &lbo);
	void UnSubdivide(TerrainQuadData* quad);

	void GenerateNewTerrain(TerrainMeshVertices* verts, TerrainMeshIndices* indices, TerrainQuad terrainQuad);
	void GenerateTerrainFromExisting(TerrainMeshVertices* parentVerts, TerrainMeshIndices* parentIndices, TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner, TerrainQuad terrainQuad);

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
