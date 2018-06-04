#pragma once

#include <string>
#include <vector>
#include <thread>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>


#include "../rendering/Renderer.h"
#include "../rendering/Model.h"
#include "../rendering/Texture.h"

#include "../resources/Texture.h"
#include "../core/CoreTools.h"
#include "../util/Gradient.h"
#include "../util/MemoryPool.h"

#include "../gui/InternalGraph.h"

const int NumCells = 64;
const int vertCount = (NumCells + 1) * (NumCells + 1);
const int indCount = NumCells * NumCells * 6;
const int vertElementCount = 8;

using TerrainMeshVertices = std::array<float, vertCount * vertElementCount>;
using TerrainMeshIndices = std::array<uint32_t, indCount>;

enum class Corner_Enum {
	uR = 0,
	uL = 1,
	dR = 2,
	dL = 3
};

struct TerrainPushConstant {
	glm::mat4 model;
};

struct TerrainCoordinateData {
	glm::vec2 pos;
	glm::vec2 size;
	glm::i32vec2 noisePos;
	glm::vec2 noiseSize;
	int sourceImageResolution;
	glm::ivec2 gridPos;
	TerrainCoordinateData(glm::vec2 pos, glm::vec2 size,
		glm::i32vec2 noisePos, glm::vec2 noiseSize, int imageRes, glm::ivec2 gridPos)
		: pos(pos), size(size), noisePos(noisePos), noiseSize(noiseSize),
		sourceImageResolution(imageRes), gridPos(gridPos) {

	}
};

struct TerrainQuad {
	TerrainQuad(glm::vec2 pos, glm::vec2 size,
		glm::i32vec2 logicalPos, glm::i32vec2 logicalSize,
		int level, glm::i32vec2 subDivPos, float centerHeightValue);
	~TerrainQuad();

	static float GetUVvalueFromLocalIndex(float i, int numCells, int level, int subDivPos);

	glm::vec2 pos; //position of corner
	glm::vec2 size; //width and length
	glm::i32vec2 logicalPos; //where in the proc-gen space it is (for noise images)
	glm::i32vec2 logicalSize; //how wide the area is.
	glm::i32vec2 subDivPos; //where in the subdivision grid it is (for splatmap)
	int level = 0; //how deep the quad is
	float heightValAtCenter = 0;
	bool isSubdivided = false;
	bool isUploaded = false;

	std::shared_ptr<TerrainMeshVertices> vertices;
	std::shared_ptr<TerrainMeshIndices> indices;

	struct SubQuads {
		std::shared_ptr<TerrainQuad> UpLeft;
		std::shared_ptr<TerrainQuad> DownLeft;
		std::shared_ptr<TerrainQuad> UpRight;
		std::shared_ptr<TerrainQuad> DownRight;
	} subQuads;

	Signal isReady;
};

class Terrain {
public:
	//std::shared_ptr<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>> terrainQuads;

	//Refence to all of the quads
	std::vector<std::shared_ptr<TerrainQuad>> quadHandles;
	std::vector<std::shared_ptr<TerrainQuad>> PrevQuadHandles;
	std::vector<TerrainMeshVertices> verts;
	std::vector<TerrainMeshIndices> inds;

	std::shared_ptr<TerrainQuad> rootQuad;
	int maxLevels;
	int maxNumQuads;
	int numQuads = 1;

	TerrainCoordinateData coordinateData;
	float heightScale = 100;

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;

	std::shared_ptr<VulkanBufferVertex> vertexBuffer;
	std::shared_ptr<VulkanBufferIndex> indexBuffer;

	DescriptorSet descriptorSet;

	std::shared_ptr<Texture> terrainSplatMap;
	std::shared_ptr<VulkanTexture2D> terrainVulkanSplatMap;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;
	//TerrainPushConstant modelMatrixData;

	//std::shared_ptr<Texture> maillerFace;

	InternalGraph::GraphUser fastGraphUser;

	Gradient splatmapTextureGradient;

	SimpleTimer drawTimer;

	std::vector<std::thread *> terrainGenerationWorkers;

	Terrain(std::shared_ptr<MemoryPool<TerrainQuad>> pool, InternalGraph::GraphPrototype& protoGraph,
		int numCells, int maxLevels, float heightScale, TerrainCoordinateData coordinateData);
	~Terrain();

	void InitTerrain(std::shared_ptr<VulkanRenderer> renderer, glm::vec3 cameraPos,
		std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray);

	void UpdateTerrain(glm::vec3 viewerPos);
	void DrawTerrain(VkCommandBuffer cmdBuff, bool wireframe);
	void BuildCommandBuffer(std::shared_ptr<Terrain> curTerrain, bool ifWireframe);
	void CleanUp();

	std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation(float x, float z);
private:

	void InitTerrainQuad(
		std::shared_ptr<TerrainQuad> quad, Corner_Enum corner, glm::vec3 viewerPos);

	bool UpdateTerrainQuad(std::shared_ptr<TerrainQuad> quad, glm::vec3 viewerPos);

	void SetupMeshbuffers();
	void SetupUniformBuffer();
	void SetupImage();
	void SetupPipeline();

	void SetupDescriptorSets(std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray);

	void UpdateMeshBuffer();

	void SubdivideTerrain(std::shared_ptr<TerrainQuad> quad, glm::vec3 viewerPos);
	void UnSubdivide(std::shared_ptr<TerrainQuad> quad);
};


//Create a mesh chunk for rendering using fastgraph as the input data
void GenerateTerrainChunk(InternalGraph::GraphUser& graphUser, std::shared_ptr<TerrainQuad> terrainQuad, Corner_Enum corner, float heightScale, float widthScale, int maxSubDivLevels);