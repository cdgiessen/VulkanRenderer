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

struct SkySettings {
	bool show_skyEditor = true;
	bool autoMove = false;
	float moveSpeed = 0.0002f;
	float horizontalAngle = 0.0f;
	float verticalAngle = 1.0f;

	DirectionalLight sun;
	DirectionalLight moon;
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

	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	std::vector<SpotLight> spotLights;

	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::shared_ptr<TerrainManager> terrainManager;
	std::shared_ptr<InstancedSceneObject> treesInstanced;
	std::shared_ptr<InstancedSceneObject> rocksInstanced;

	std::shared_ptr<Skybox> skybox;

	std::shared_ptr<Camera> camera;

	float verticalVelocity = 0;
	float gravity = -0.25f;
	float heightOfGround = 1.4f;

	SkySettings skySettings;
	void UpdateSunData();
	void DrawSkySettingsGui();

	bool pressedControllerJumpButton = false;
	bool releasedControllerJumpButton = false;

	bool UpdateTerrain = true;

	PBR_Material testMat;
	InstancedSceneObject::InstanceData testInstanceData;
};

