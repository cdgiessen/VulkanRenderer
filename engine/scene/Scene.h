#pragma once

#include "rendering/ViewCamera.h"

#include "PlayerController.h"

namespace job
{
class ThreadPool;
}
class Time;
namespace Input
{
class InputDirector;
}
namespace Resource
{
class Resources;
}
class VulkanRenderer;

class Scene
{
	public:
	Scene (job::ThreadPool& thread_pool,
	    Time& time,
	    Input::InputDirector const& input,
	    Resource::Resources& resourceMan,
	    VulkanRenderer& renderer);

	void Update ();

	private:
	job::ThreadPool& thread_pool;
	Time& time;
	Input::InputDirector const& input;
	Resource::Resources& resources;
	VulkanRenderer& renderer;

	public:
	PlayerController player;

	ViewCameraID main_camera;
};


//#include "Camera.h"
//#include "Skybox.h"
//#include "Terrain.h"
//#include "TerrainSystem.h"
//#include "Water.h"
//
// struct SkySettings
//{
//	bool show_skyEditor = true;
//	bool autoMove = false;
//	float moveSpeed = 0.0002f;
//	float horizontalAngle = 0.0f;
//	float verticalAngle = 1.0f;
//
//	DirectionalLight sun;
//	DirectionalLight moon;
//};
//
// class Scene
//{
//	public:
//	Scene (job::ThreadPool& thread_pool,
//	    Resource::Resources& resourceMan,
//	    VulkanRenderer& renderer,
//	    Time& time,
//	    InternalGraph::GraphPrototype& graph);
//
//	void UpdateScene ();
//	void RenderDepthPrePass (VkCommandBuffer commandBuffer);
//	void RenderOpaque (VkCommandBuffer commandBuffer, bool wireframe);
//	void RenderTransparent (VkCommandBuffer commandBuffer, bool wireframe);
//	void RenderSkybox (VkCommandBuffer commandBuffer);
//	void UpdateSceneGUI ();
//
//	Camera* GetCamera ();
//
//	bool drawNormals = false;
//	bool walkOnGround = false;
//
//	private:
//	job::ThreadPool& thread_pool;
//	VulkanRenderer& renderer;
//	Resource::Resources& resourceMan;
//	Time& time;
//
//	std::vector<DirectionalLight> directionalLights;
//	std::vector<PointLight> pointLights;
//	std::vector<SpotLight> spotLights;
//
//	std::unique_ptr<TerrainSystem> terrainSystem;
//	std::unique_ptr<Water> water_plane;
//
//	std::unique_ptr<Skybox> skybox;
//
//	std::unique_ptr<Camera> camera;
//
//	float verticalVelocity = 0;
//	float gravity = -0.25f;
//	float heightOfGround = 1.4f;
//
//	SkySettings skySettings;
//	void UpdateSunData ();
//	void DrawSkySettingsGui ();
//
//	bool pressedControllerJumpButton = false;
//	bool releasedControllerJumpButton = false;
//
//	bool UpdateTerrain = true;
//};
