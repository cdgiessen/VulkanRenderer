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

class TerrainChunkBuffer;
class Terrain;

struct TerrainQuad {
	TerrainQuad(TerrainChunkBuffer& chunkBuffer, int index);

	void Setup(glm::vec2 pos, glm::vec2 size,
		glm::i32vec2 logicalPos, glm::i32vec2 logicalSize,
		int level, glm::i32vec2 subDivPos, float centerHeightValue,
		Terrain* terrain);

	void CleanUp();
	void RecursiveCleanUp();

	static float GetUVvalueFromLocalIndex(float i, int numCells, int level, int subDivPos);

	//Create a mesh chunk for rendering using fastgraph as the input data
	void GenerateTerrainChunk(InternalGraph::GraphUser& graphUser, 
		float heightScale, float widthScale);

	void Draw();

	enum class ChunkState {
		free,//nobody owns me
		waiting_create, //writing to device buffer
		creating,
		waiting_upload, //finished device write, ready for upload,
		uploading, //is uploading to gpu
		ready, //ready to be rendered
	} state;

	glm::vec2 pos; //position of corner
	glm::vec2 size; //width and length
	glm::i32vec2 logicalPos; //where in the proc-gen space it is (for noise images)
	glm::i32vec2 logicalSize; //how wide the area is.
	glm::i32vec2 subDivPos; //where in the subdivision grid it is (for splatmap)
	int level = 0; //how deep the quad is
	float heightValAtCenter = 0;
	bool isSubdivided = false;

	Terrain* terrain; //who owns it
	TerrainChunkBuffer& chunkBuffer;
	int index = -1;
	
	TerrainMeshVertices* vertices;
	TerrainMeshIndices* indices;

	struct SubQuads {
		int UpLeft = -1;
		int DownLeft = -1;
		int UpRight = -1;
		int DownRight = -1;
	} subQuads;
};

class Terrain {
public:
	// MemoryPool<TerrainMeshVertices, 1024>& meshPool_vertices;
	// MemoryPool<TerrainMeshIndices, 1024>& meshPool_indices;
	TerrainChunkBuffer& chunkBuffer;

	//Refence to all of the quads
	//std::vector<int> quadHandles;
	//std::vector<TerrainQuad*> PrevQuadHandles;
	//std::vector<TerrainMeshVertices> verts;
	//std::vector<TerrainMeshIndices> inds;

	int rootQuadIndex = -1;
	TerrainQuad* rootQuad; 

	int maxLevels;
	int maxNumQuads;
	int numQuads = 1;

	TerrainCoordinateData coordinateData;
	float heightScale = 100;

	VulkanRenderer* renderer;

	std::shared_ptr<ManagedVulkanPipeline> mvp;

	std::shared_ptr<VulkanDescriptor> descriptor;

	//std::vector<VulkanBufferVertex> vertexBuffers;
	//std::vector<VulkanBufferIndex> indexBuffers;

	DescriptorSet descriptorSet;

	std::shared_ptr<Texture> terrainSplatMap;
	std::shared_ptr<VulkanTexture2D> terrainVulkanSplatMap;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;
	//TerrainPushConstant modelMatrixData;

	//std::shared_ptr<Texture> maillerFace;

	InternalGraph::GraphUser fastGraphUser;

	Gradient splatmapTextureGradient;

	SimpleTimer drawTimer;

	//std::vector<std::thread *> terrainGenerationWorkers;

	Terrain(VulkanRenderer* renderer,
		TerrainChunkBuffer& chunkBuffer,
		InternalGraph::GraphPrototype& protoGraph,
		int numCells, int maxLevels, float heightScale, TerrainCoordinateData coordinateData);
	~Terrain();

	void InitTerrain(VulkanRenderer* renderer, glm::vec3 cameraPos,
		std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray);

	void UpdateTerrain(glm::vec3 viewerPos);
	void DrawTerrain(VkCommandBuffer cmdBuff, bool wireframe);
	void CleanUp();

	std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation(float x, float z);
private:

	void InitTerrainQuad(
		TerrainQuad* quad, glm::vec3 viewerPos);

	bool UpdateTerrainQuad(TerrainQuad* quad, glm::vec3 viewerPos);

	void SetupMeshbuffers();
	void SetupUniformBuffer();
	void SetupImage();
	void SetupPipeline();

	void SetupDescriptorSets(std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray);

	void UpdateMeshBuffer();

	void SubdivideTerrain(TerrainQuad* quad, glm::vec3 viewerPos);
	void UnSubdivide(TerrainQuad* quad);
};
