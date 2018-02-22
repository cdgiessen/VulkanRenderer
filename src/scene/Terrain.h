#pragma once

#include <string>
#include <vector>
#include <thread>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>


#include "../rendering/VulkanRenderer.hpp"
#include "../rendering/VulkanModel.hpp"
#include "../rendering/VulkanTexture.hpp"

#include "../resources/Texture.h"
#include "../core/Gradient.h"
#include "../core/MemoryPool.h"
#include "../core/CoreTools.h"

//#include "TerrainGenerator.h"
//#include "../gui/TerGenNodeGraph.h"
#include "../gui/InternalGraph.h"

const int SplatMapSize = 1024;
const int NumCells = 32;
const int vertCount = (NumCells + 1) * (NumCells + 1);
const int indCount = NumCells * NumCells * 6;
const int vertElementCount = 12;

typedef std::array<float, vertCount * vertElementCount> TerrainMeshVertices;
typedef std::array<uint32_t, indCount> TerrainMeshIndices;

enum class Corner_Enum {
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
	DescriptorSet descriptorSet;
	VkDeviceMemory vertexOffset;
	VkDeviceMemory indexOffset;
	TerrainMeshVertices vertices;
	TerrainMeshIndices indices;

	struct SubQuads {
		std::shared_ptr<TerrainQuadData> UpLeft;
		std::shared_ptr<TerrainQuadData> DownLeft;
		std::shared_ptr<TerrainQuadData> UpRight;
		std::shared_ptr<TerrainQuadData> DownRight;
	} subQuads;

	~TerrainQuadData();
};

class Terrain {
public:
	//std::shared_ptr<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>> terrainQuads;

	//Refence to all of the quads
	std::vector<std::shared_ptr<TerrainQuadData>> quadHandles;
	std::vector<std::shared_ptr<TerrainQuadData>> PrevQuadHandles;
	std::vector<TerrainMeshVertices> verts;
	std::vector<TerrainMeshIndices> inds;

	std::shared_ptr<TerrainQuadData> rootQuad;
	int maxLevels;
	int maxNumQuads;
	int numQuads = 0;

	glm::vec2 position;
	glm::vec2 size;
	glm::i32vec2 noisePosition;
	glm::i32vec2 noiseSize;
	int sourceImageResolution;
	float heightScale = 100;

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;

	VulkanBufferVertex vertexBuffer;
	VulkanBufferIndex indexBuffer;

	std::shared_ptr<Texture> terrainSplatMap;
	VulkanTexture2D terrainVulkanSplatMap;

	//std::shared_ptr<TextureArray> terrainTextureArray;
	//VulkanTexture2DArray terrainVulkanTextureArray;
	VulkanTexture2DArray* terrainVulkanTextureArray;


	VulkanBufferUniform modelUniformBuffer;

	//std::shared_ptr<Texture> maillerFace;
	
	InternalGraph::GraphUser fastGraphUser;
	//std::unique_ptr<NewNodeGraph::TerGenGraphUser> fastTerrainUser;
	//std::shared_ptr<NewNodeGraph::TerGenNodeGraph> fastTerrainGraph;

	Gradient splatmapTextureGradient;

	SimpleTimer drawTimer;

	std::vector<std::thread *> terrainGenerationWorkers;

	Terrain(std::shared_ptr<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>> pool, InternalGraph::GraphPrototype& protoGraph,
		int numCells, int maxLevels, float heightScale, int sourceImageResolution,
		glm::vec2 pos, glm::vec2 size, glm::i32vec2 noisePosition, glm::i32vec2 noiseSize);
	~Terrain();

	void InitTerrain(std::shared_ptr<VulkanRenderer> renderer, glm::vec3 cameraPos, VulkanTexture2DArray* terrainVulkanTextureArray);

	void UpdateTerrain(glm::vec3 viewerPos);
	void DrawTerrain(VkCommandBuffer cmdBuff, VkDeviceSize offsets[1], std::shared_ptr<Terrain> curTerrain, bool wireframe);
	void BuildCommandBuffer(std::shared_ptr<Terrain> curTerrain, bool ifWireframe);
	void CleanUp();

	std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation(float x, float z);
private:

	std::shared_ptr<TerrainQuadData> InitTerrainQuad(std::shared_ptr<TerrainQuadData> q, glm::vec2 position, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize,
		int level);
	std::shared_ptr<TerrainQuadData> InitTerrainQuadFromParent(std::shared_ptr<TerrainQuadData> parent, std::shared_ptr<TerrainQuadData> q, Corner_Enum corner,
		glm::vec2 position, glm::vec2 size, glm::i32vec2 logicalPos, glm::i32vec2 logicalSize, int level, glm::i32vec2 subDivPos);

	bool UpdateTerrainQuad(std::shared_ptr<TerrainQuadData> quad, glm::vec3 viewerPos);

	void SetupMeshbuffers();
	void SetupUniformBuffer();
	void SetupImage();
	void SetupModel();
	void SetupPipeline();

	void SetupDescriptorLayoutAndPool();

	void UpdateModelBuffer();

	void UploadMeshBuffer();
	void UpdateMeshBuffer();

	void UpdateUniformBuffer(float time);
	void SubdivideTerrain(std::shared_ptr<TerrainQuadData> quad, glm::vec3 viewerPos);
	void UnSubdivide(std::shared_ptr<TerrainQuadData> quad);
	void RecursiveUnSubdivide(std::shared_ptr<TerrainQuadData> quad);

};


//Create a mesh chunk for rendering using fastgraph as the input data
void GenerateNewTerrainSubdivision(InternalGraph::GraphUser& graphUser, TerrainMeshVertices& verts, TerrainMeshIndices& indices,
	TerrainQuad terrainQuad, Corner_Enum corner, float heightScale, int maxSubDivLevels);

//not used as it depends on previous terrains, which is great for runtime but not for first generation (since it has dependence on its parents mesh being ready)
//void GenerateTerrainFromExisting(TerrainGenerator& terrainGenerator, NewNodeGraph::TerGenGraphUser& fastGraph, TerrainMeshVertices& parentVerts, TerrainMeshIndices& parentIndices,
//	TerrainMeshVertices& verts, TerrainMeshIndices& indices, Corner_Enum corner, TerrainQuad terrainQuad, float heightScale, int maxSubDivLevels);

//Uses input texture to generate terrain from
void GenerateTerrainFromTexture(Texture& tex, TerrainMeshVertices& verts, TerrainMeshIndices& indices, TerrainQuad terrainQuad, Corner_Enum corner, float heightScale, int maxSubDivLevels);