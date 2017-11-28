#pragma once


#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\vulkan\VulkanRenderer.hpp"

#include "..\core\Mesh.h"
#include "..\core\TimeManager.h"

#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"
#include "GameObject.h"
#include "Water.h"
#include "TerrainManager.h"
#include "InstancedSceneObject.h"


class Scene
{
public:
	Scene();
	~Scene();

	void PrepareScene(std::shared_ptr<VulkanRenderer> renderer);
	void UpdateScene(std::shared_ptr<TimeManager> timeManager);
	void RenderScene(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateSceneGUI();
	void CleanUpScene();

	void CreateUniformBuffers();

	std::shared_ptr<Camera> GetCamera();

	bool drawNormals = false;
private:

	std::shared_ptr<VulkanRenderer> renderer;

	VulkanBuffer globalVariableBuffer;
	VulkanBuffer lightsInfoBuffer;
	std::vector<PointLight> pointLights;

	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::shared_ptr<TerrainManager> terrainManager;
	std::shared_ptr<InstancedSceneObject> treesInstanced;
	std::shared_ptr<InstancedSceneObject> rocksInstanced;

	std::shared_ptr<Skybox> skybox;

	std::shared_ptr<Camera> camera;

	//glm::mat4 reverseDepth = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1); //reverses the clip plane to be from [1,0] instead of [0,1], to help with z fighting isseus
};

