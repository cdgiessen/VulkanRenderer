#pragma once

#include <glm/vec3.hpp>

#include "../rendering/Renderer.hpp"

#include "../resources/ResourceManager.h"
#include "../resources/Mesh.h"
#include "../core/TimeManager.h"

#include "Camera.h"
#include "Terrain.h"
#include "Skybox.h"
#include "GameObject.h"
#include "Water.h"
#include "TerrainManager.h"
#include "InstancedSceneObject.h"

//#include <gltf2\glTF2.hpp>

struct SunSettings {
	bool show_skyEditor = true;
	bool autoMove = false;
	float moveSpeed = 0.0002f;
	float intensity = 1.0f;
	float horizontalAngle = 0.0f;
	float verticalAngle = 1.0f;

	glm::vec3 dir = glm::vec3(0, 60, 25);
	glm::vec3 color = glm::vec3(1.0f, 0.98f, 0.9f);
};

class Scene
{
public:
	Scene();
	~Scene();

	void PrepareScene(std::shared_ptr<ResourceManager> resourceMan,  std::shared_ptr<VulkanRenderer> renderer, InternalGraph::GraphPrototype& graph);
	void UpdateScene(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<TimeManager> timeManager);
	void RenderScene(VkCommandBuffer commandBuffer, bool wireframe);
	void UpdateSceneGUI();
	void CleanUpScene();

	std::shared_ptr<Camera> GetCamera();

	bool drawNormals = false;
	bool walkOnGround = false;
private:

	std::shared_ptr<VulkanRenderer> renderer;

	std::vector<PointLight> pointLights;

	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::shared_ptr<TerrainManager> terrainManager;
	std::shared_ptr<InstancedSceneObject> treesInstanced;
	std::shared_ptr<InstancedSceneObject> rocksInstanced;

	std::shared_ptr<Skybox> skybox;

	std::shared_ptr<Camera> camera;

	float verticalVelocity = 0;
	float gravity = -0.25f;
	float heightOfGround = 1.4f;

	SunSettings sunSettings;
	void UpdateSunData();
	void DrawSkySettingsGui();

	//TODO: gltf2 integration
	//std::vector<std::shared_ptr<gltf2::Asset>> assets;

	bool pressedControllerJumpButton = false;
	bool releasedControllerJumpButton = false;
};

