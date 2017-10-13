#pragma once

#include "..\vulkan\VulkanDevice.hpp"

#include "..\core\Mesh.h"
#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"
#include "GameObject.h"
#include "Water.h"
#include "TerrainManager.h"
#include "InstancedSceneObject.h"

#include "..\core\TimeManager.h"

class Scene
{
public:
	Scene(VulkanDevice* device);
	~Scene();

	void PrepareScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void ReInitScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain);
	void UpdateScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, TimeManager* timeManager);
	void RenderScene(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateSceneGUI();
	void CleanUpScene();

	void CreateUniformBuffers();

	Camera* GetCamera();

	bool drawNormals = false;
private:

	VulkanDevice* device;

	VulkanBuffer globalVariableBuffer;
	VulkanBuffer lightsInfoBuffer;
	std::vector<PointLight> pointLights;

	std::vector<GameObject*> gameObjects;
	TerrainManager* terrainManager;
	InstancedSceneObject* treesInstanced;
	InstancedSceneObject* rocksInstanced;

	Skybox* skybox;

	Camera* camera;

	//glm::mat4 reverseDepth = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1); //reverses the clip plane to be from [1,0] instead of [0,1], to help with z fighting isseus
};

