#include "Scene.h"

#include "core/JobSystem.h"
#include "core/TimeManager.h"

#include "resources/Resource.h"

#include "rendering/Renderer.h"

#include "cml/cml.h"

#include "core/Input.h"
#include "core/Logger.h"

#include <entt/entt.hpp>

Scene::Scene (job::TaskManager& task_manager,
    TimeManager& time_manager,
    Resource::ResourceManager& resource_manager,
    VulkanRenderer& renderer)
: task_manager (task_manager), time_manager (time_manager), resource_manager (resource_manager), renderer (renderer)
{
	main_registry = entt::registry{};
}

void Scene::Update () {}

// Scene::Scene (job::TaskManager& task_manager,
//    Resource::ResourceManager& resourceMan,
//    VulkanRenderer& renderer,
//    TimeManager& time_manager,
//    InternalGraph::GraphPrototype& graph)
//: task_manager (task_manager), renderer (renderer), resourceMan (resourceMan), time_manager (time_manager)
//{
//
//	camera = std::make_unique<Camera> (cml::vec3f (0, 1, -5), cml::vec3f (0, 1, 0), 0, 90);
//
//	directionalLights.resize (5);
//	pointLights.resize (5);
//
//	skySettings.sun = DirectionalLight (cml::vec3f (0, 60, 25), cml::vec3f (1.0f, 0.98f, 0.9f), 10.0f);
//	skySettings.moon = DirectionalLight (-cml::vec3f (0, 60, 25), cml::vec3f (0.9f, 0.95f, 1.0f), 0.0f);
//
//	directionalLights.at (0) = skySettings.sun;
//
//	skybox = std::make_unique<Skybox> (renderer);
//	// skybox->skyboxCubeMap = resourceMan.texture_manager.GetTexIDByName ("Skybox");
//	// skybox->model = std::make_unique<VulkanModel> (
//	//     renderer.device, renderer.async_task_manager, createCube ());
//	// skybox->InitSkybox ();
//
//
//	terrainManager = std::make_unique<TerrainManager> (task_manager, graph, resourceMan, renderer);
//
//	water_plane = std::make_unique<Water> (resourceMan, renderer);
//}
//
// void Scene::UpdateScene ()
//{
//
//	GlobalData gd;
//	gd.time = (float)time_manager.RunningTime ();
//
//	// cml::mat4f proj = cml::perspective (cml::radians (55.0f),
//	//     renderer.vulkanSwapChain.swapChainExtent.width /
//	//         (float)renderer.vulkanSwapChain.swapChainExtent.height,
//	//     0.05f,
//	//     100000.f);
//
//	std::vector<CameraData> cd (1);
//	// cd.at (0).view = camera->GetViewMatrix ();
//	// cd.at (0).projView = proj * cd.at (0).view;
//	// cd.at (0).cameraDir = camera->Front;
//	// cd.at (0).cameraPos = camera->Position;
//
//	UpdateSunData ();
//
//	// renderer.dynamic_data.Update (gd, { cd }, directionalLights, pointLights, spotLights);
//
//	if (walkOnGround)
//	{
//		float groundHeight =
//		    (float)terrainManager->GetTerrainHeightAtLocation (camera->Position.x, camera->Position.z) + 2.0f;
//		float height = (float)camera->Position.y;
//
//		if (pressedControllerJumpButton && !releasedControllerJumpButton)
//		{
//			if (Input::IsJoystickConnected (0) && Input::GetControllerButton (0, 0))
//			{
//				pressedControllerJumpButton = false;
//				releasedControllerJumpButton = true;
//				verticalVelocity += 0.15f;
//			}
//		}
//		if (Input::IsJoystickConnected (0) && Input::GetControllerButton (0, 0))
//		{
//			pressedControllerJumpButton = true;
//		}
//		if (Input::IsJoystickConnected (0) && !Input::GetControllerButton (0, 0))
//		{
//			releasedControllerJumpButton = false;
//		}
//
//		if (Input::GetKeyDown (Input::KeyCode::SPACE))
//		{
//			verticalVelocity += 0.15f;
//		}
//		verticalVelocity += (float)time_manager.DeltaTime () * gravity;
//		height += verticalVelocity;
//		camera->Position.y = height;
//		if (camera->Position.y < groundHeight)
//		{ // for over land
//			camera->Position.y = groundHeight;
//			verticalVelocity = 0;
//		}
//		else if (camera->Position.y < heightOfGround)
//		{ // for over water
//			camera->Position.y = heightOfGround;
//			verticalVelocity = 0;
//		}
//	}
//
//	if (Input::GetKeyDown (Input::KeyCode::V)) UpdateTerrain = !UpdateTerrain;
//
//	skybox->UpdateUniform (proj, cd.at (0).view);
//
//	if (UpdateTerrain && terrainManager != nullptr)
//		terrainManager->UpdateTerrains (camera->Position);
//
//	water_plane->UpdateUniform (camera->Position);
//}
//
// void Scene::RenderOpaque (VkCommandBuffer commandBuffer, bool wireframe)
//{
//
//	if (terrainManager != nullptr) terrainManager->RenderTerrain (commandBuffer, wireframe);
//}
//
// void Scene::RenderTransparent (VkCommandBuffer cmdBuf, bool wireframe)
//{
//	water_plane->Draw (cmdBuf, wireframe);
//}
//
// void Scene::RenderSkybox (VkCommandBuffer commandBuffer)
//{
//	skybox->WriteToCommandBuffer (commandBuffer);
//}
//
// void Scene::UpdateSunData ()
//{
//	if (skySettings.autoMove)
//	{
//		skySettings.horizontalAngle += skySettings.moveSpeed;
//	}
//
//	float X = cml::cos (skySettings.horizontalAngle) * cml::cos (skySettings.verticalAngle);
//	float Z = cml::sin (skySettings.horizontalAngle) * cml::cos (skySettings.verticalAngle);
//	float Y = cml::sin (skySettings.verticalAngle);
//	skySettings.sun.direction = cml::vec3f (X, Y, Z);
//	skySettings.moon.direction = -cml::vec3f (X, Y, Z);
//
//	directionalLights.at (0) = skySettings.sun;
//	directionalLights.at (1) = skySettings.moon;
//}
//
// void Scene::DrawSkySettingsGui ()
//{
//	ImGui::SetNextWindowPos (ImVec2 (0, 675), ImGuiCond_FirstUseEver);
//	ImGui::SetNextWindowSize (ImVec2 (356, 180), ImGuiCond_FirstUseEver);
//
//	if (ImGui::Begin ("Sky editor", &skySettings.show_skyEditor))
//	{
//		ImGui::Checkbox ("Sun motion", &skySettings.autoMove);
//		ImGui::DragFloat ("Sun Move Speed", &skySettings.moveSpeed, 0.0001f, 0.0f, 0.0f, "%.5f");
//		ImGui::DragFloat ("Sun Intensity", &skySettings.sun.intensity, 0.01f);
//		ImGui::DragFloat ("Sun Horizontal", &skySettings.horizontalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
//		ImGui::DragFloat ("Sun Vertical", &skySettings.verticalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
//		ImGui::SliderFloat3 ("Sun Color", (&(skySettings.sun.color.x)), 0.0f, 1.0f);
//	}
//	ImGui::End ();
//}
//
// void Scene::UpdateSceneGUI ()
//{
//	if (terrainManager != nullptr)
//	{
//		terrainManager->UpdateTerrainGUI ();
//	}
//
//	DrawSkySettingsGui ();
//	return;
//
//	bool value;
//	if (ImGui::Begin ("Lighting Tester", &value))
//	{
//		ImGui::Text ("Directional Lights");
//		for (int i = 0; i < 2 /*directionalLights.size()*/; i++)
//		{
//
//			ImGui::DragFloat3 (std::string ("Direction##" + std::to_string (i)).c_str (),
//			    &(directionalLights[i].direction.x),
//			    0.01f);
//			ImGui::SliderFloat3 (
//			    std::string ("Color##" + std::to_string (i)).c_str (), &(directionalLights[i].color.x), 0.0f, 1.0f);
//			ImGui::DragFloat (std::string ("Intensity##" + std::to_string (i)).c_str (),
//			    &directionalLights[i].intensity,
//			    0.01f);
//			ImGui::Text (" ");
//		}
//		ImGui::Text ("Point Lights");
//		for (int i = 0; i < 2 /*pointLights.size()*/; i++)
//		{
//
//			ImGui::DragFloat3 (std::string ("Position##" + std::to_string (i + 1000)).c_str (),
//			    (&(pointLights[i].position.x)),
//			    0.01f);
//			ImGui::SliderFloat3 (std::string ("Color##" + std::to_string (i + 1000)).c_str (),
//			    (&(pointLights[i].color.x)),
//			    0.0f,
//			    1.0f);
//			ImGui::DragFloat (std::string ("Attenuation##" + std::to_string (i + 1000)).c_str (),
//			    &pointLights[i].attenuation,
//			    0.0f,
//			    0.01f);
//			ImGui::Text (" ");
//		}
//	}
//	ImGui::End ();
//}
//
// Camera* Scene::GetCamera () { return camera.get (); }