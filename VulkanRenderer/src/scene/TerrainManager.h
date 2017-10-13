#pragma once

#include "..\core\TimeManager.h"
#include "..\core\MemoryPool.h"

#include "..\vulkan\VulkanSwapChain.hpp"
#include "..\vulkan\VulkanPipeline.hpp"
#include "..\vulkan\VulkanTools.h"

#include "Camera.h"
#include "Terrain.h"
#include "Water.h"

class TerrainManager
{
public:
	TerrainManager(VulkanDevice* device);
	~TerrainManager();

	void GenerateTerrain(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera);
	void ReInitTerrain(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void UpdateTerrains(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera, TimeManager* timeManager);
	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateTerrainGUI();

	void CleanUpTerrain();

	VkCommandBuffer CreateTerrainMeshUpdateCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level);
	void FlushTerrainMeshUpdateCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

private:
	VulkanDevice* device;

	std::vector<Terrain*> terrains;
	MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)> terrainQuadPool;

	std::vector<Water*> waters;

	bool show_terrain_manager_window = true;
	bool recreateTerrain = false;
	float terrainWidth = 1000;
	float terrainHeightScale = 100.0f;
	int terrainMaxLevels = 3;
	int terrainGridDimentions = 1;
	SimpleTimer terrainUpdateTimer;

	int maxNumQuads = 1; //maximum quads managed by this
};

