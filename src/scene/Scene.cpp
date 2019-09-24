#include "Scene.h"

#include "cml/cml.h"

#include "core/Input.h"
#include "core/Logger.h"

#include "imgui.hpp"

#include "entt/entt.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

Scene::Scene (Resource::AssetManager& resourceMan,
    VulkanRenderer& renderer,
    TimeManager& timeManager,
    InternalGraph::GraphPrototype& graph)
: renderer (renderer), resourceMan (resourceMan), timeManager (timeManager)
{

	camera = std::make_unique<Camera> (cml::vec3f (0, 1, -5), cml::vec3f (0, 1, 0), 0, 90);

	directionalLights.resize (5);
	pointLights.resize (5);

	skySettings.sun = DirectionalLight (cml::vec3f (0, 60, 25), cml::vec3f (1.0f, 0.98f, 0.9f), 10.0f);
	skySettings.moon = DirectionalLight (-cml::vec3f (0, 60, 25), cml::vec3f (0.9f, 0.95f, 1.0f), 0.0f);

	directionalLights.at (0) = skySettings.sun;

	// pointLights.resize(1);
	// PointLight pl;
	// pl.position = cml::vec3f(0, 3, 3);
	// pl.color = cml::vec3f(0, 1, 0);
	// pl.attenuation = 5;
	// pointLights[0] = pl;

	// pointLights[0] = PointLight(cml::vec4f(0, 4, 3, 1), cml::vec4f(0, 0, 0, 0), cml::vec4f(1.0,
	// 0.045f, 0.0075f, 1.0f)); pointLights[1] = PointLight(cml::vec4f(10, 10, 50, 1), cml::vec4f(0,
	// 0, 0, 0), cml::vec4f(1.0, 0.045f, 0.0075f, 1.0f)); pointLights[2] = PointLight(cml::vec4f(50,
	// 10, 10, 1), cml::vec4f(0, 0, 0, 0), cml::vec4f(1.0, 0.045f, 0.0075f, 1.0f)); pointLights[3] =
	// PointLight(cml::vec4f(50, 10, 50, 1), cml::vec4f(0, 0, 0, 0), cml::vec4f(1.0, 0.045f,
	// 0.0075f, 1.0f)); pointLights[4] = PointLight(cml::vec4f(75, 10, 75, 1), cml::vec4f(0, 0, 0,
	// 0), cml::vec4f(1.0, 0.045f, 0.0075f, 1.0f));

	skybox = std::make_unique<Skybox> (renderer);
	skybox->skyboxCubeMap = resourceMan.texManager.GetTexIDByName ("Skybox");
	skybox->model = std::make_unique<VulkanModel> (renderer, createCube ());
	skybox->InitSkybox ();

	// auto cubeObject 	= std::make_unique<GameObject> (renderer);
	// cubeObject->LoadModel (createCube ());
	// cubeObject->gameObjectModel = std::make_shared<VulkanModel> (renderer, cubeObject->gameObjectMesh);

	// TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);

	// cubeObject->gameObjectTexture = resourceMan.texManager.GetTexIDByName ("ColorGradientCube");
	// cubeObject->gameObjectVulkanTexture =
	//     renderer.textureManager.CreateTexture2D (cubeObject->gameObjectTexture, details);
	// cubeObject->InitGameObject ();
	// gameObjects.push_back (std::move (cubeObject));
	// for (int i = 0; i < 9; i++) {
	//	for (int j = 0; j < 9; j++)
	//	{

	//		std::unique_ptr<GameObject> sphereObject = std::make_shared<GameObject>(renderer);
	//		sphereObject->usePBR = true;
	//		sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer.device);
	//		sphereObject->LoadModel(createSphere(10));
	//		sphereObject->position = cml::vec3f(0, i * 2.2, j * 2.2 );
	//		//sphereObject->pbr_mat.albedo = cml::vec3f(0.8, 0.2, 0.2);
	//		sphereObject->pbr_mat.metallic = 0.1f + (float)i / 10.0f;
	//		sphereObject->pbr_mat.roughness = 0.1f + (float)j / 10.0f;

	//		//sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture>(renderer.device);
	//		//sphereObject->gameObjectTexture = resourceMan.texManager.loadTextureFromFileRGBA("assets/textures/Red.png");
	//		sphereObject->InitGameObject();
	//		gameObjects.push_back(sphereObject);
	//	}
	//}

	// for (int i = 0; i < 9; i++) {
	//	for (int j = 0; j < 9; j++)
	//	{

	//		std::unique_ptr<GameObject> sphereObject = std::make_shared<GameObject>(renderer);
	//		sphereObject->usePBR = false;
	//		sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer.device);
	//		sphereObject->LoadModel(createSphere(10));
	//		sphereObject->position = cml::vec3f(0, i * 2.2, (float) 8 * 2.2 - j * 2.2 - 30);
	//		sphereObject->phong_mat.diffuse = (float)(i ) / 10.0;
	//		sphereObject->phong_mat.specular = (float)j / 6;
	//																																																																																																																																																	sphereObject->phong_mat.reflectivity
	//= 128; 		sphereObject->phong_mat.color = cml::vec4f(1.0, 0.3, 0.3, 1.0);
	//		//sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture>(renderer.device);
	//		//sphereObject->gameObjectTexture = resourceMan.texManager.loadTextureFromFileRGBA("assets/textures/Red.png");
	//		sphereObject->InitGameObject();
	//		gameObjects.push_back(sphereObject);
	//	}
	//}

	// std::unique_ptr<GameObject> pbr_test = std::make_shared<GameObject>(renderer);
	// pbr_test->usePBR = true;

	terrainManager = std::make_unique<TerrainManager> (graph, resourceMan, renderer);

	// terrainManager->SetupResources(resourceMan, renderer);
	// terrainManager->GenerateTerrain(resourceMan, renderer, camera);

	// treesInstanced = std::make_unique<InstancedSceneObject>(renderer);
	// treesInstanced->SetFragmentShaderToUse("assets/shaders/instancedSceneObject.frag.spv");
	// treesInstanced->SetBlendMode(VK_FALSE);
	// treesInstanced->SetCullMode(VK_CULL_MODE_BACK_BIT);
	// treesInstanced->LoadModel(createCube());
	// treesInstanced->LoadTexture(resourceMan.texManager.loadTextureFromFileRGBA("assets/textures/grass.jpg"));
	// treesInstanced->InitInstancedSceneObject(renderer);

	// treesInstanced->AddInstances({ cml::vec3f(10,0,10),cml::vec3f(10,0,20), cml::vec3f(20,0,10),
	// cml::vec3f(10,0,40), cml::vec3f(10,0,-40), cml::vec3f(100,0,40) });
	// treesInstanced->UploadInstances();

	std::vector<InstancedSceneObject::InstanceData> data;
	int size = 2;
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			for (int k = 0; k < size; k++)
			{
				data.push_back (InstancedSceneObject::InstanceData (
				    cml::vec3f (i * 2, k * 2, j * 2), cml::vec3f (0, 0, 0), 1, 0));
			}
	// treesInstanced->AddInstances(data);
	// rocksInstanced = std::make_shared<InstancedSceneObject>(renderer);

	// gltf2 integration
	// std::unique_ptr< gltf2::Asset> tree_test = std::make_shared<gltf2::Asset>();
	//*tree_test = gltf2::load("Resources/Assets/tree_test.gltf");

	water_plane = std::make_unique<Water> (resourceMan, renderer);
}

Scene::~Scene ()
{

	// Log::Debug << "scene deleted\n";
}

void Scene::UpdateScene ()
{

	GlobalData gd;
	gd.time = (float)timeManager.RunningTime ();

	cml::mat4f proj = cml::perspective (cml::radians (55.0f),
	    renderer.vulkanSwapChain.swapChainExtent.width /
	        (float)renderer.vulkanSwapChain.swapChainExtent.height,
	    0.05f,
	    100000.f);

	std::vector<CameraData> cd (1);
	cd.at (0).view = camera->GetViewMatrix ();
	cd.at (0).projView = proj * cd.at (0).view;
	cd.at (0).cameraDir = camera->Front;
	cd.at (0).cameraPos = camera->Position;

	UpdateSunData ();

	renderer.UpdateRenderResources (gd, { cd }, directionalLights, pointLights, spotLights);

	if (walkOnGround)
	{
		float groundHeight =
		    (float)terrainManager->GetTerrainHeightAtLocation (camera->Position.x, camera->Position.z) + 2.0f;
		float height = (float)camera->Position.y;

		if (pressedControllerJumpButton && !releasedControllerJumpButton)
		{
			if (Input::IsJoystickConnected (0) && Input::GetControllerButton (0, 0))
			{
				pressedControllerJumpButton = false;
				releasedControllerJumpButton = true;
				verticalVelocity += 0.15f;
			}
		}
		if (Input::IsJoystickConnected (0) && Input::GetControllerButton (0, 0))
		{
			pressedControllerJumpButton = true;
		}
		if (Input::IsJoystickConnected (0) && !Input::GetControllerButton (0, 0))
		{
			releasedControllerJumpButton = false;
		}

		if (Input::GetKeyDown (Input::KeyCode::SPACE))
		{
			verticalVelocity += 0.15f;
		}
		verticalVelocity += (float)timeManager.DeltaTime () * gravity;
		height += verticalVelocity;
		camera->Position.y = height;
		if (camera->Position.y < groundHeight)
		{ // for over land
			camera->Position.y = groundHeight;
			verticalVelocity = 0;
		}
		else if (camera->Position.y < heightOfGround)
		{ // for over water
			camera->Position.y = heightOfGround;
			verticalVelocity = 0;
		}
	}

	if (Input::GetKeyDown (Input::KeyCode::V)) UpdateTerrain = !UpdateTerrain;


	skybox->UpdateUniform (proj, cd.at (0).view);

	for (auto& obj : gameObjects)
	{
		obj->UpdateUniformBuffer ((float)timeManager.RunningTime ());
	}

	if (UpdateTerrain && terrainManager != nullptr)
		terrainManager->UpdateTerrains (camera->Position);

	water_plane->UpdateUniform (camera->Position);
}

void Scene::RenderOpaque (VkCommandBuffer commandBuffer, bool wireframe)
{
	for (auto& obj : gameObjects)
	{
		obj->Draw (commandBuffer, wireframe, drawNormals);
	}

	// treesInstanced->WriteToCommandBuffer(commandBuffer, wireframe);

	if (terrainManager != nullptr) terrainManager->RenderTerrain (commandBuffer, wireframe);
}

void Scene::RenderTransparent (VkCommandBuffer cmdBuf, bool wireframe)
{
	water_plane->Draw (cmdBuf, wireframe);
}

void Scene::RenderSkybox (VkCommandBuffer commandBuffer)
{
	skybox->WriteToCommandBuffer (commandBuffer);
}

void Scene::UpdateSunData ()
{
	if (skySettings.autoMove)
	{
		skySettings.horizontalAngle += skySettings.moveSpeed;
	}

	float X = cml::cos (skySettings.horizontalAngle) * cml::cos (skySettings.verticalAngle);
	float Z = cml::sin (skySettings.horizontalAngle) * cml::cos (skySettings.verticalAngle);
	float Y = cml::sin (skySettings.verticalAngle);
	skySettings.sun.direction = cml::vec3f (X, Y, Z);
	skySettings.moon.direction = -cml::vec3f (X, Y, Z);

	directionalLights.at (0) = skySettings.sun;
	directionalLights.at (1) = skySettings.moon;
}

void Scene::DrawSkySettingsGui ()
{
	ImGui::SetNextWindowPos (ImVec2 (0, 675), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize (ImVec2 (356, 180), ImGuiCond_FirstUseEver);

	if (ImGui::Begin ("Sky editor", &skySettings.show_skyEditor))
	{
		ImGui::Checkbox ("Sun motion", &skySettings.autoMove);
		ImGui::DragFloat ("Sun Move Speed", &skySettings.moveSpeed, 0.0001f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat ("Sun Intensity", &skySettings.sun.intensity, 0.01f);
		ImGui::DragFloat ("Sun Horizontal", &skySettings.horizontalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat ("Sun Vertical", &skySettings.verticalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::SliderFloat3 ("Sun Color", (&(skySettings.sun.color.x)), 0.0f, 1.0f);
	}
	ImGui::End ();
}

void Scene::UpdateSceneGUI ()
{
	if (terrainManager != nullptr)
	{
		terrainManager->UpdateTerrainGUI ();
	}

	DrawSkySettingsGui ();
	return;

	bool value;
	if (ImGui::Begin ("Lighting Tester", &value))
	{
		ImGui::Text ("Directional Lights");
		for (int i = 0; i < 2 /*directionalLights.size()*/; i++)
		{

			ImGui::DragFloat3 (std::string ("Direction##" + std::to_string (i)).c_str (),
			    &(directionalLights[i].direction.x),
			    0.01f);
			ImGui::SliderFloat3 (
			    std::string ("Color##" + std::to_string (i)).c_str (), &(directionalLights[i].color.x), 0.0f, 1.0f);
			ImGui::DragFloat (std::string ("Intensity##" + std::to_string (i)).c_str (),
			    &directionalLights[i].intensity,
			    0.01f);
			ImGui::Text (" ");
		}
		ImGui::Text ("Point Lights");
		for (int i = 0; i < 2 /*pointLights.size()*/; i++)
		{

			ImGui::DragFloat3 (std::string ("Position##" + std::to_string (i + 1000)).c_str (),
			    (&(pointLights[i].position.x)),
			    0.01f);
			ImGui::SliderFloat3 (std::string ("Color##" + std::to_string (i + 1000)).c_str (),
			    (&(pointLights[i].color.x)),
			    0.0f,
			    1.0f);
			ImGui::DragFloat (std::string ("Attenuation##" + std::to_string (i + 1000)).c_str (),
			    &pointLights[i].attenuation,
			    0.0f,
			    0.01f);
			ImGui::Text (" ");
		}
	}
	ImGui::End ();

	if (ImGui::Begin ("instance tester", &value))
	{
		ImGui::DragFloat3 ("Position", (&(testInstanceData.pos.x)));
		ImGui::DragFloat3 ("Rotation", (&(testInstanceData.rot.x)));
		ImGui::DragFloat ("Scale", &testInstanceData.scale);
		if (ImGui::Button ("Add instance"))
		{
			treesInstanced->AddInstance (testInstanceData);
		}
		if (ImGui::Button ("Remove Instance"))
		{
			treesInstanced->RemoveInstance (testInstanceData);
		}
	}
	ImGui::End ();

	treesInstanced->ImGuiShowInstances ();
	static int x = 0;
	if (ImGui::Begin ("create game object", &value))
	{
		if (ImGui::Button ("Add game object"))
		{
			std::unique_ptr<GameObject> sphereObject = std::make_unique<GameObject> (renderer);
			sphereObject->usePBR = true;
			sphereObject->position = cml::vec3f (x++, 3, 2.2);
			// sphereObject->pbr_mat.albedo = cml::vec3f(0.8, 0.2, 0.2);
			sphereObject->pbr_mat.metallic = 0.1f + (float)5 / 10.0f;
			sphereObject->pbr_mat.roughness = 0.1f + (float)5 / 10.0f;

			sphereObject->gameObjectTexture = resourceMan.texManager.GetTexIDByName ("Red.png");
			sphereObject->InitGameObject ();
			gameObjects.push_back (std::move (sphereObject));
		}
	}
	ImGui::End ();
	for (auto& go : gameObjects)
	{
		go->pbr_mat.albedo = testMat.albedo;
		go->phong_mat.color = cml::to_vec4 (testMat.albedo, 1.0f);
	}
}

Camera* Scene::GetCamera () { return camera.get (); }