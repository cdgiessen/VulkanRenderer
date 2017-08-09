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

const int NumCells = 50;
const int vertCount = (NumCells + 1) * (NumCells + 1);
const int indCount = NumCells * NumCells * 6;
const int vertElementCount = 12;

typedef std::array<float, vertCount * vertElementCount> TerrainMeshVertices;
typedef std::array<uint32_t, indCount> TerrainMeshIndices;

class TerrainQuad {
public:
	glm::vec3 pos; //position of corner
	glm::vec3 size; //width and length
	int level; //how deep the quad is

	bool isSubdivided;

	ModelBufferObject modelUniformObject;

	TerrainQuad();
	~TerrainQuad();

	void init(float posX, float posY, float sizeX, float sizeY, int level);

	void CleanUp();

	void CreateTerrainMesh(TerrainMeshVertices* verts, TerrainMeshIndices* indices);

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

	glm::vec3 position;
	glm::vec3 size;

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

	Terrain(int numCells, int maxLevels, float posX, float posY, float sizeX, float sizeY);
	~Terrain();

	void InitTerrain(VulkanDevice* device, VkRenderPass renderPass, VkQueue copyQueue, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void ReinitTerrain(VulkanDevice* device, VkRenderPass renderPass, uint32_t viewPortWidth, uint32_t viewPortHeight, VulkanBuffer &global, VulkanBuffer &lighting);
	void UpdateTerrain(glm::vec3 viewerPos, VkQueue copyQueue, VulkanBuffer &gbo, VulkanBuffer &lbo);
	void DrawTerrain(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain);
	void CleanUp();

	void LoadTexture(std::string filename);
	void LoadTextureArray();
private:

	TerrainQuadData* InitTerrainQuad(glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo);

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
	void SubdivideTerrain(TerrainQuadData* quad, VulkanBuffer &gbo, VulkanBuffer &lbo);
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

static Mesh* genTerrain(int numCells, float xLoc, float zLoc, float xSize, float zSize) {
	std::vector<Vertex> verts;
	std::vector<uint16_t> indices;

	noise::module::Perlin myModule;
	myModule.SetOctaveCount(6);
	myModule.SetFrequency(0.05);
	myModule.SetPersistence(0.6);

	noise::utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	heightMapBuilder.SetSourceModule(myModule);
	heightMapBuilder.SetDestNoiseMap(heightMap);
	heightMapBuilder.SetDestSize(numCells + 1, numCells + 1);
	heightMapBuilder.SetBounds(xLoc, xLoc + xSize + xSize / (float)numCells, zLoc, zLoc + zSize + zSize / (float)numCells);
	heightMapBuilder.Build();

	utils::RendererImage renderer;
	utils::Image image;
	renderer.ClearGradient();
	//old gradient for vertex colors, new one for tex splat map
	////renderer.AddGradientPoint(-1.0000, utils::Color(0, 0, 128, 255)); // deeps
	////renderer.AddGradientPoint(0.000, utils::Color(0, 0, 255, 255)); // shallow
	////renderer.AddGradientPoint(0.1000, utils::Color(0, 128, 255, 255)); // shore
	//renderer.AddGradientPoint(-1.0000, utils::Color(76, 36, 16, 255)); // Ocean bottom
	//renderer.AddGradientPoint(-0.1000, utils::Color(107, 71, 53, 255)); // shallow ocean
	//renderer.AddGradientPoint(0.1000, utils::Color(228, 163, 55, 255)); // shore
	//renderer.AddGradientPoint(0.1625, utils::Color(240, 240, 64, 255)); // sand
	//renderer.AddGradientPoint(0.2250, utils::Color(32, 160, 0, 255)); // grass
	//renderer.AddGradientPoint(0.4750, utils::Color(127, 91, 48, 255)); // dirt
	//renderer.AddGradientPoint(0.7500, utils::Color(128, 128, 128, 255)); // rock
	//renderer.AddGradientPoint(1.0000, utils::Color(255, 255, 255, 255)); // snow

	/*
	"DeadSpruceTreeTrunk.png",
	"Sand.png",
	"SpruceTreeLeaf.png",
	"Rock.png",
	"Snow.png",
	*/

	renderer.AddGradientPoint(-1.000, utils::Color(255, 0, 0, 32));
	renderer.AddGradientPoint(0.0000, utils::Color(255, 0, 0, 255));
	renderer.AddGradientPoint(0.1000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.4000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.5000, utils::Color(255, 0, 0, 255));
	renderer.AddGradientPoint(0.6000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(0.8000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(1.0000, utils::Color(0, 0, 0, 255));

	renderer.SetSourceNoiseMap(heightMap);
	renderer.SetDestImage(image);
	renderer.Render();


	verts.resize((numCells + 1) * (numCells + 1));
	indices.resize((numCells) * (numCells) * 6);

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			double hL = myModule.GetValue((double)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc);
			double hR = myModule.GetValue((double)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc);
			double hD = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j + 1)*(zSize) / (float)numCells + zLoc);
			double hU = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j - 1)*(zSize) / (float)numCells + zLoc);
			glm::vec3 normal(hR - hL, 1, hU - hD);

			double value = (myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0.0, (double)j *(zSize) / (float)numCells + (zLoc)));

			verts[(i)*(numCells + 1) + j] = Vertex(glm::vec3((double)i *(xSize) / (float)numCells, value * 5, (double)j * (zSize) / (float)numCells), normal,
				glm::vec2(i, j),
				glm::vec4((float)image.GetValue(i, j).red / 256, (float)image.GetValue(i, j).green / 256, (float)image.GetValue(i, j).blue / 256, (float)image.GetValue(i, j).alpha / 256));
			//std::cout << value << std::endl;
		}
	}

	int counter = 0;
	for (int i = 0; i < numCells; i++)
	{
		for (int j = 0; j < numCells; j++)
		{
			indices[counter++] = i * (numCells + 1) + j;
			indices[counter++] = i * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j;
			indices[counter++] = i * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j + 1;
			indices[counter++] = (i + 1) * (numCells + 1) + j;
		}
	}

	return new Mesh(verts, indices);
}