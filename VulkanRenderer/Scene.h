#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanModel.hpp"
#include "VulkanTexture.hpp"

#include "Mesh.h"
#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"
#include "GameObject.h"
#include "Water.h"
#include "TerrainManager.h"
#include "TimeManager.h"

class Scene
{
public:
	Scene(VulkanDevice device);
	~Scene();

	void PrepareScene(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void ReInitScene(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void UpdateScene(VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, TimeManager* timeManager);
	void RenderScene(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateSceneGUI();
	void CleanUpScene();

	void CreateUniformBuffers();

	Camera* GetCamera();
private:

	VulkanDevice device;

	VulkanBuffer globalVariableBuffer;
	VulkanBuffer lightsInfoBuffer;
	std::vector<PointLight> pointLights;

	std::vector<GameObject*> gameObjects;
	TerrainManager* terrainManager;

	Skybox* skybox;

	Camera* camera;
};

