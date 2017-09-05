#pragma once

#include "Camera.h"
#include "Terrain.h"
#include "Water.h"
#include "VulkanTools.h"
#include "TimeManager.h"

class TerrainManager
{
public:
	TerrainManager(VulkanDevice device);
	~TerrainManager();

	void GenerateTerrain(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera);
	void ReInitTerrain(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void UpdateTerrains(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, VulkanBuffer globalVariableBuffer, VulkanBuffer lightsInfoBuffer, Camera* camera, TimeManager* timeManager);
	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateTerrainGUI();

	void CleanUpTerrain();

private:
	VulkanDevice device;

	std::vector<Terrain*> terrains;
	std::vector<Water*> waters;

	bool show_terrain_manager_window = true;
	bool recreateTerrain = false;
	float terrainWidth = 1000;
	int terrainMaxLevels = 1;
	int terrainGridDimentions = 1;
	SimpleTimer terrainUpdateTimer;
};

