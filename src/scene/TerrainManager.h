#pragma once


  
  
#include "../core/CoreTools.h"
#include "../core/TimeManager.h"
#include "../core/MemoryPool.h"

#include "../rendering/VulkanRenderer.hpp"
#include "../resources/ResourceManager.h"

#include "../gui/TerGenNodeGraph.h"

#include "Camera.h"
#include "Terrain.h"
#include "Water.h"

class TerrainManager
{
public:
	TerrainManager(NewNodeGraph::TerGenNodeGraph& nodeGraph);
	~TerrainManager();

	void GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer globalVariableBuffer,
		VulkanBuffer lightsInfoBuffer, std::shared_ptr<Camera> camera);

	void UpdateTerrains(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, VulkanBuffer globalVariableBuffer,
		VulkanBuffer lightsInfoBuffer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager);

	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);

	void UpdateTerrainGUI();

	void CleanUpTerrain();

	float GetTerrainHeightAtLocation(float x, float z);

	VkCommandBuffer CreateTerrainMeshUpdateCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level);
	void FlushTerrainMeshUpdateCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

private:
	NewNodeGraph::TerGenNodeGraph& nodeGraph;

	std::shared_ptr<VulkanRenderer> renderer;

	std::vector<std::shared_ptr<Terrain>> terrains;
	std::shared_ptr<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>> terrainQuadPool;

	std::vector<std::shared_ptr<Water>> waters;
	
	bool show_terrain_manager_window = true;
	bool recreateTerrain = false;
	float terrainWidth = 1000;
	float terrainHeightScale = 100.0f;
	int terrainMaxLevels = 2;
	int terrainGridDimentions = 1;
	int sourceImageResolution = 128;
	SimpleTimer terrainUpdateTimer;

	int maxNumQuads = 1; //maximum quads managed by this

	std::vector<std::string> terrainTextureFileNames = {
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

