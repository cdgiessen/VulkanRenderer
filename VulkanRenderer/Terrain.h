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

	bool isSubdivided;

	ModelBufferObject modelUniformObject;

	TerrainQuad();
	~TerrainQuad();

	//puts the position, size, and level into the class
	void init(float posX, float posY, float sizeX, float sizeY, int level);

	//takes pointers to the vertex and index arrays, then fills them
	void CreateTerrainMesh(TerrainMeshVertices* verts, TerrainMeshIndices* indices);

	//takes in pointers to new mesh and parent mesh to generate mesh with previous points
	void CreateTerrainMeshFromParent(TerrainMeshVertices* parentVerts, TerrainMeshIndices* parentIndices, TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner);

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
	void DrawTerrain(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool wireframe);
	void BuildCommandBuffer(std::vector<VkCommandBuffer> cmdBuff, int cmdBuffIndex, VkDeviceSize offsets[1], Terrain* curTerrain, bool ifWireframe);
	void CleanUp();

	void LoadTexture(std::string filename);
	void LoadTextureArray();
private:

	TerrainQuadData* InitTerrainQuad(TerrainQuadData* q, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo);
	TerrainQuadData* InitTerrainQuadFromParent(TerrainQuadData* parent, TerrainQuadData* q, Corner_Enum corner, glm::vec3 position, glm::vec3 size, int level, VulkanBuffer &gbo, VulkanBuffer &lbo);

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

static void genTerrain(TerrainMeshVertices* verts, TerrainMeshIndices* indices, int numCells, float xLoc, float zLoc, float xSize, float zSize) {
	
	noise::module::Perlin myModule;
	myModule.SetOctaveCount(6);
	myModule.SetFrequency(0.02);
	myModule.SetPersistence(0.4);

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

	float heightScale = 25;

	for (int i = 0; i <= numCells; i++)
	{
		for (int j = 0; j <= numCells; j++)
		{
			double value = (myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0.0, (double)j *(zSize) / (float)numCells + (zLoc)));
			
			double hL = myModule.GetValue((double)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc)*heightScale;
			double hR = myModule.GetValue((double)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc)*heightScale;
			double hD = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j + 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			double hU = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j - 1)*(zSize) / (float)numCells + zLoc)*heightScale;
			glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 0] = (double)i *(xSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 1] = value * heightScale;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 2] = (double)j * (zSize) / (float)numCells;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 3] = normal.x;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 4] = normal.y;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 5] = normal.z;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 6] = i;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 7] = j;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 8] = (float)image.GetValue(i, j).red / 256;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 9] = (float)image.GetValue(i, j).green / 256;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 10] = (float)image.GetValue(i, j).blue / 256;
			(*verts)[((i)*(numCells + 1) + j)* vertElementCount + 11] = (float)image.GetValue(i, j).alpha / 256;
			//std::cout << value << std::endl;
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


}


//Needs to account for which corner the new terrain is in
static void genTerrainFromExisting(TerrainMeshVertices* parentVerts, TerrainMeshIndices* parentIndices, TerrainMeshVertices* verts, TerrainMeshIndices* indices, Corner_Enum corner, int numCells, float xLoc, float zLoc, float xSize, float zSize) {

	noise::module::Perlin myModule;
	myModule.SetOctaveCount(6);
	myModule.SetFrequency(0.02);
	myModule.SetPersistence(0.4);

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

	int parentIOffset = (corner == 2 || corner == 3) ? (numCells + 1) / 2 : 0;
	int parentJOffset = (corner == 1 || corner == 3) ? (numCells + 1) / 2 : 0;

	float heightScale = 25;

	//uses the parent terrain for 1/4 of the grid
	for (int i = 0; i <= numCells; i += 2)
	{
		for (int j = 0; j <= numCells; j += 2)
		{
			int vLoc = (i*(numCells + 1) + j)* vertElementCount;

			//existing terrain is every other vertex in the grid
			int parentVLoc = ((i / 2 + parentIOffset)*(numCells + 1) + (j / 2 + parentJOffset))* vertElementCount;

			(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
			(*verts)[vLoc + 1] = (*parentVerts)[parentVLoc + 1];
			(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
			(*verts)[vLoc + 6] = i;
			(*verts)[vLoc + 7] = j;
			(*verts)[vLoc + 8] = (*parentVerts)[parentVLoc + 8];
			(*verts)[vLoc + 9] = (*parentVerts)[parentVLoc + 9];
			(*verts)[vLoc + 10] = (*parentVerts)[parentVLoc + 10];
			(*verts)[vLoc + 11] = (*parentVerts)[parentVLoc + 11];
			
		}
	}

	//Fills in lines starting at i = 1 and skips a line, filling in all the j's (half of the terrain)
	for (int i = 1; i <= numCells; i += 2)
	{
		for (int j = 0; j <= numCells; j++)
		{
				double value = (myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0.0, (double)j *(zSize) / (float)numCells + (zLoc)));

				int vLoc = (i*(numCells + 1) + j)* vertElementCount;

				(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
				(*verts)[vLoc + 1] = value * heightScale;
				(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
				(*verts)[vLoc + 6] = i;
				(*verts)[vLoc + 7] = j;
				(*verts)[vLoc + 8] = (float)image.GetValue(i, j).red / 256;
				(*verts)[vLoc + 9] = (float)image.GetValue(i, j).green / 256;
				(*verts)[vLoc + 10] = (float)image.GetValue(i, j).blue / 256;
				(*verts)[vLoc + 11] = (float)image.GetValue(i, j).alpha / 256;			
		}
	}

	//fills the last 1/4 of cells, starting at i= 0 and jumping. Like the first double for loop but offset by one
	for (int i = 0; i <= numCells; i += 2)
	{
		for (int j = 1; j <= numCells; j += 2)
		{
			double value = (myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0.0, (double)j *(zSize) / (float)numCells + (zLoc)));

			int vLoc = (i*(numCells + 1) + j)* vertElementCount;

			(*verts)[vLoc + 0] = (double)i *(xSize) / (float)numCells;
			(*verts)[vLoc + 1] = value * heightScale;
			(*verts)[vLoc + 2] = (double)j * (zSize) / (float)numCells;
			(*verts)[vLoc + 6] = i;
			(*verts)[vLoc + 7] = j;
			(*verts)[vLoc + 8] = (float)image.GetValue(i, j).red / 256;
			(*verts)[vLoc + 9] = (float)image.GetValue(i, j).green / 256;
			(*verts)[vLoc + 10] = (float)image.GetValue(i, j).blue / 256;
			(*verts)[vLoc + 11] = (float)image.GetValue(i, j).alpha / 256;
		}
	}

	//normals -- Look I know its verbose and proabably prone to being a waste of time, but since its something as banal as normal calculation and the optomizer doesn't know about how most of the center doesn't need to recalculate the heights, it feels like a useful thing to do
	{

		//center normals
		{
			for (int i = 1; i < numCells; i++)
			{
				for (int j = 1; j < numCells; j++)
				{
					//gets height values of its neighbors, rather than recalculating them from scratch
					double hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
					double hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
					double hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
					double hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
					glm::vec3 normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

					int vLoc = (i*(numCells + 1) + j)* vertElementCount;
					(*verts)[vLoc + 3] = normal.x;
					(*verts)[vLoc + 4] = normal.y;
					(*verts)[vLoc + 5] = normal.z;
				}
			}
		}

		//edge normals
		{
			// i = 0, j[1,numCells - 1]
			int i = 0, j = 0;
			double hL, hR, hD, hU;
			glm::vec3 normal;
			int vLoc;

			for (int j = 1; j < numCells; j++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = myModule.GetValue((double)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale;
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// i = numCells, j[1,numCells - 1]
			i = numCells;
			for (int j = 1; j < numCells; j++) {
				hL = myModule.GetValue((double)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale;
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1] ; // i - 1
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1]; // j + 1
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1]; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// j = 0, i[1, numCells - 1]
			j = 0;
			for (int i = 1; i < numCells; i++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
				hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1] ; // j + 1
				hU = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale;//
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}

			// j = numCells, i[1, numCells - 1]
			j = numCells;
			for (int i = 1; i < numCells; i++) {
				hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1]; // i + 1
				hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1]; // i - 1
				hD = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale;
				hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1] ; // j -1
				normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

				vLoc = (i*(numCells + 1) + j)* vertElementCount;
				(*verts)[vLoc + 3] = normal.x;
				(*verts)[vLoc + 4] = normal.y;
				(*verts)[vLoc + 5] = normal.z;
			}
		}

		//corner normals
		{
			double hL, hR, hD, hU;
			glm::vec3 normal;
			int vLoc;

			int i = 0, j = 0;
			hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1] ; // i + 1
			hR = myModule.GetValue((double)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
			hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1] ; // j + 1
			hU = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;


			i = 0, j = numCells;
			hL = (*verts)[((i + 1)*(numCells + 1) + j)* vertElementCount + 1] ; // i + 1
			hR = myModule.GetValue((double)(i - 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale; // i - 1
			hD = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
			hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1] ; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;

			i = numCells, j = 0;
			hL = myModule.GetValue((double)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
			hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1] ; // i - 1
			hD = (*verts)[(i*(numCells + 1) + j + 1)* vertElementCount + 1] ; // j + 1
			hU = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j - 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2*xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;

			i = numCells, j = numCells;
			hL = myModule.GetValue((double)(i + 1) *(xSize) / (float)numCells + (xLoc), 0, (double)j *(zSize) / (float)numCells + zLoc) * heightScale; // i + 1
			hR = (*verts)[((i - 1)*(numCells + 1) + j)* vertElementCount + 1] ; // i - 1
			hD = myModule.GetValue((double)i *(xSize) / (float)numCells + (xLoc), 0, (double)(j + 1)*(zSize) / (float)numCells + zLoc) * heightScale; // j + 1
			hU = (*verts)[(i*(numCells + 1) + j - 1)* vertElementCount + 1] ; // j -1
			normal = glm::normalize(glm::vec3(hR - hL, 2 * xSize / ((float)numCells), hU - hD));

			vLoc = (i*(numCells + 1) + j)* vertElementCount;
			(*verts)[vLoc + 3] = normal.x;
			(*verts)[vLoc + 4] = normal.y;
			(*verts)[vLoc + 5] = normal.z;
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


}