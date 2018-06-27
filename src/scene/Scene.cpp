#include "Scene.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "../core/Input.h"
#include "../core/Logger.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

Scene::Scene(ResourceManager* resourceMan,
	VulkanRenderer* renderer,
	InternalGraph::GraphPrototype& graph)
{
	this->renderer = renderer;
	this->resourceMan = resourceMan;

	camera = std::make_unique< Camera>(glm::vec3(0, 1, -5), glm::vec3(0, 1, 0), 0, 90);

	directionalLights.resize(5);
	pointLights.resize(5);

	skySettings.sun = DirectionalLight(glm::vec3(0, 60, 25), 15.0f, glm::vec3(1.0f, 0.98f, 0.9f));
	skySettings.moon = DirectionalLight(-glm::vec3(0, 60, 25), 0.5f, glm::vec3(0.9f, 0.95f, 1.0f));

	directionalLights.at(0) = skySettings.sun;

	//pointLights.resize(1);
	//PointLight pl;
	//pl.position = glm::vec3(0, 3, 3);
	//pl.color = glm::vec3(0, 1, 0);
	//pl.attenuation = 5;
	//pointLights[0] = pl;

	//pointLights[0] = PointLight(glm::vec4(0, 4, 3, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[1] = PointLight(glm::vec4(10, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[2] = PointLight(glm::vec4(50, 10, 10, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[3] = PointLight(glm::vec4(50, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[4] = PointLight(glm::vec4(75, 10, 75, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));

	skybox = std::make_unique<Skybox>();
	skybox->vulkanCubeMap = std::make_shared<VulkanCubeMap>(renderer->device);
	skybox->skyboxCubeMap = resourceMan->texManager.loadCubeMapFromFile("assets/Textures/Skybox/Skybox2", ".png");
	skybox->model = std::make_shared<VulkanModel>(renderer->device);
	skybox->model->loadFromMesh(createCube(), *renderer);
	skybox->InitSkybox(renderer);

	//std::shared_ptr<GameObject> cubeObject = std::make_shared<GameObject>();
	//cubeObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
	//cubeObject->LoadModel(createCube());
	//cubeObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
	//cubeObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/ColorGradientCube.png");
	////cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	//cubeObject->InitGameObject(renderer);
	//gameObjects.push_back(cubeObject);
	//for (int i = 0; i < 9; i++) {
	//	for (int j = 0; j < 9; j++)
	//	{

	//		std::shared_ptr<GameObject> sphereObject = std::make_shared<GameObject>();
	//		sphereObject->usePBR = true;
	//		sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
	//		sphereObject->LoadModel(createSphere(10));
	//		sphereObject->position = glm::vec3(0, i * 2.2, j * 2.2 );
	//		//sphereObject->pbr_mat.albedo = glm::vec3(0.8, 0.2, 0.2);
	//		sphereObject->pbr_mat.metallic = 0.1f + (float)i / 10.0f;
	//		sphereObject->pbr_mat.roughness = 0.1f + (float)j / 10.0f;

	//		//sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
	//		//sphereObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/Red.png");
	//		sphereObject->InitGameObject(renderer);
	//		gameObjects.push_back(sphereObject);
	//	}
	//}

	//for (int i = 0; i < 9; i++) {
	//	for (int j = 0; j < 9; j++)
	//	{

	//		std::shared_ptr<GameObject> sphereObject = std::make_shared<GameObject>();
	//		sphereObject->usePBR = false;
	//		sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
	//		sphereObject->LoadModel(createSphere(10));
	//		sphereObject->position = glm::vec3(0, i * 2.2, (float) 8 * 2.2 - j * 2.2 - 30);
	//		sphereObject->phong_mat.diffuse = (float)(i ) / 10.0;
	//		sphereObject->phong_mat.specular = (float)j / 6;
	//		sphereObject->phong_mat.reflectivity = 128;
	//		sphereObject->phong_mat.color = glm::vec4(1.0, 0.3, 0.3, 1.0);
	//		//sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
	//		//sphereObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/Red.png");
	//		sphereObject->InitGameObject(renderer);
	//		gameObjects.push_back(sphereObject);
	//	}
	//}

	//std::shared_ptr<GameObject> pbr_test = std::make_shared<GameObject>();
	//pbr_test->usePBR = true;

	//UNTIL FURTHER NOTICE!
	//terrainManager = std::make_unique<TerrainManager>(graph, resourceMan, renderer);

	//terrainManager->SetupResources(resourceMan, renderer);
	//terrainManager->GenerateTerrain(resourceMan, renderer, camera);

	//treesInstanced = std::make_shared<InstancedSceneObject>(renderer);
	//treesInstanced->SetFragmentShaderToUse("assets/shaders/instancedSceneObject.frag.spv"));
	//treesInstanced->SetBlendMode(VK_FALSE);
	//treesInstanced->SetCullMode(VK_CULL_MODE_BACK_BIT);
	//treesInstanced->LoadModel(createCube());
	//treesInstanced->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/grass.jpg"));
	//treesInstanced->InitInstancedSceneObject(renderer);

	//treesInstanced->AddInstances({ glm::vec3(10,0,10),glm::vec3(10,0,20), glm::vec3(20,0,10), glm::vec3(10,0,40), glm::vec3(10,0,-40), glm::vec3(100,0,40) });

	std::vector< InstancedSceneObject::InstanceData> data;
	int size = 2;
	for (int i = 0; i < size; i++)
		for (int j = 0; j < size; j++)
			for (int k = 0; k < size; k++)
			{
				data.push_back(InstancedSceneObject::InstanceData(glm::vec3(i * 2, k * 2, j * 2), glm::vec3(0, 0, 0), 1, 0));
			}
	//treesInstanced->AddInstances(data);
	//rocksInstanced = std::make_shared<InstancedSceneObject>(renderer);

	// gltf2 integration
	//std::shared_ptr< gltf2::Asset> tree_test = std::make_shared<gltf2::Asset>();
	//*tree_test = gltf2::load("Resources/Assets/tree_test.gltf");

}

Scene::~Scene()
{
	skybox->CleanUp();
	for (auto& obj : gameObjects) {
		obj->CleanUp();
	}

	//treesInstanced->CleanUp();

	Log::Debug << "scene deleted\n";
}

void Scene::UpdateScene(ResourceManager* resourceMan, TimeManager* timeManager) {

	if (walkOnGround) {
		float groundHeight = terrainManager->GetTerrainHeightAtLocation(camera->Position.x, camera->Position.z) + 2.0f;
		float height = camera->Position.y;

		if (pressedControllerJumpButton && !releasedControllerJumpButton) {
			if (Input::IsJoystickConnected(0) && Input::GetControllerButton(0, 0)) {
				pressedControllerJumpButton = false;
				releasedControllerJumpButton = true;
				verticalVelocity += 0.15f;
			}
		}
		if (Input::IsJoystickConnected(0) && Input::GetControllerButton(0, 0)) {
			pressedControllerJumpButton = true;
		}
		if (Input::IsJoystickConnected(0) && !Input::GetControllerButton(0, 0)) {
			releasedControllerJumpButton = false;
		}

		if (Input::GetKeyDown(Input::KeyCode::SPACE)) {
			verticalVelocity += 0.15f;
		}
		verticalVelocity += (float)timeManager->DeltaTime()*gravity;
		height += verticalVelocity;
		camera->Position.y = height;
		if (camera->Position.y < groundHeight) { //for over land
			camera->Position.y = groundHeight;
			verticalVelocity = 0;
		}
		else if (camera->Position.y < heightOfGround) {//for over water
			camera->Position.y = heightOfGround;
			verticalVelocity = 0;
		}
	}

	if (Input::GetKeyDown(Input::KeyCode::V))
		UpdateTerrain = !UpdateTerrain;
	if (UpdateTerrain && terrainManager != nullptr)
		terrainManager->UpdateTerrains(resourceMan, camera->Position);

	UpdateSunData();

	for (auto& obj : gameObjects) {
		obj->UpdateUniformBuffer((float)timeManager->RunningTime());
	}

	GlobalData gd;
	gd.time = (float)timeManager->RunningTime();

	glm::mat4 proj = depthReverserMatrix * glm::perspective(glm::radians(45.0f),
		renderer->vulkanSwapChain.swapChainExtent.width / (float)renderer->vulkanSwapChain.swapChainExtent.height,
		0.05f, 10000000.0f);
	proj[1][1] *= -1;

	CameraData cd;
	cd.view = camera->GetViewMatrix();
	cd.projView = proj * cd.view;
	cd.cameraDir = camera->Front;
	cd.cameraPos = camera->Position;

	//directionalLights.at(0) = DirectionalLight(skySettings.sun);
	//directionalLights.at(1) = DirectionalLight(skySettings.moon);

	skybox->UpdateUniform(proj, cd.view);
	renderer->UpdateRenderResources(gd, cd, directionalLights, pointLights, spotLights);

}

void Scene::RenderScene(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };


	for (auto& obj : gameObjects) {
		obj->Draw(commandBuffer, wireframe, drawNormals);
	}

	//treesInstanced->WriteToCommandBuffer(commandBuffer, wireframe);

	if (terrainManager != nullptr)
		terrainManager->RenderTerrain(commandBuffer, wireframe);

	skybox->WriteToCommandBuffer(commandBuffer);
}

void Scene::UpdateSunData() {
	if (skySettings.autoMove) {
		skySettings.horizontalAngle += skySettings.moveSpeed;
	}

	float X = glm::cos(skySettings.horizontalAngle) * glm::cos(skySettings.verticalAngle);
	float Z = glm::sin(skySettings.horizontalAngle) * glm::cos(skySettings.verticalAngle);
	float Y = glm::sin(skySettings.verticalAngle);
	skySettings.sun.direction = glm::vec3(X, Y, Z);
	skySettings.moon.direction = -glm::vec3(X, Y, Z);
}

void Scene::DrawSkySettingsGui() {
	ImGui::SetNextWindowPos(ImVec2(0, 675), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(356, 180), ImGuiSetCond_FirstUseEver);

	if (ImGui::Begin("Sky editor", &skySettings.show_skyEditor)) {
		ImGui::Checkbox("Sun motion", &skySettings.autoMove);
		ImGui::DragFloat("Sun Move Speed", &skySettings.moveSpeed, 0.0001f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat("Sun Intensity", &skySettings.sun.intensity, 0.01f);
		ImGui::DragFloat("Sun Horizontal", &skySettings.horizontalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat("Sun Vertical", &skySettings.verticalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::SliderFloat3("Sun Color", ((float*)glm::value_ptr(skySettings.sun.color)), 0.0f, 1.0f);
	}
	ImGui::End();
}

void Scene::UpdateSceneGUI() {
	if (terrainManager != nullptr){
		terrainManager->UpdateTerrainGUI();
		terrainManager->DrawTerrainTextureViewer();
	}

	DrawSkySettingsGui();
	bool value;
	if (ImGui::Begin("Lighting Tester", &value)) {
		ImGui::Text("Directional Lights");
		for (int i = 0; i < directionalLights.size(); i++)
		{

			ImGui::DragFloat3(std::string("Direction##" + std::to_string(i)).c_str(), (float*)glm::value_ptr(directionalLights[i].direction), 0.01f);
			ImGui::SliderFloat3(std::string("Color##" + std::to_string(i)).c_str(), (float*)glm::value_ptr(directionalLights[i].color), 0.0f, 1.0f);
			ImGui::DragFloat(std::string("Intensity##" + std::to_string(i)).c_str(), &directionalLights[i].intensity, 0.01f);
			ImGui::Text(" ");
		}
		ImGui::Text("Point Lights");
		for (int i = 0; i < pointLights.size(); i++)
		{

			ImGui::DragFloat3(std::string("Position##" + std::to_string(i + 1000)).c_str(), ((float*)glm::value_ptr(pointLights[i].position)), 0.01f);
			ImGui::SliderFloat3(std::string("Color##" + std::to_string(i + 1000)).c_str(), ((float*)glm::value_ptr(pointLights[i].color)), 0.0f, 1.0f);
			ImGui::DragFloat(std::string("Attenuation##" + std::to_string(i + 1000)).c_str(), &pointLights[i].attenuation, 0.0f, 0.01f);
			ImGui::Text(" ");
		}
	}
	ImGui::End();

	if (ImGui::Begin("instance tester", &value)) {
		ImGui::DragFloat3("Position", ((float*)glm::value_ptr(testInstanceData.pos)));
		ImGui::DragFloat3("Rotation", ((float*)glm::value_ptr(testInstanceData.rot)));
		ImGui::DragFloat("Scale", &testInstanceData.scale);
		if (ImGui::Button("Add instance")) {
			treesInstanced->AddInstance(testInstanceData);
		}
		if (ImGui::Button("Remove Instance")) {
			treesInstanced->RemoveInstance(testInstanceData);
		}
	}
	ImGui::End();

	treesInstanced->ImGuiShowInstances();
	static int x = 0;
	if (ImGui::Begin("create game object", &value)) {
		if (ImGui::Button("Add game object")) {
			std::unique_ptr<GameObject> sphereObject = std::make_unique<GameObject>();
			sphereObject->usePBR = true;
			sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
			sphereObject->LoadModel(createSphere(10));
			sphereObject->position = glm::vec3(x++, 3, 2.2);
			//sphereObject->pbr_mat.albedo = glm::vec3(0.8, 0.2, 0.2);
			sphereObject->pbr_mat.metallic = 0.1f + (float)5 / 10.0f;
			sphereObject->pbr_mat.roughness = 0.1f + (float)5 / 10.0f;

			//sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
			//sphereObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/Red.png");
			sphereObject->InitGameObject(renderer);
			gameObjects.push_back(std::move(sphereObject));
		}
	}
	ImGui::End();
	for (auto& go : gameObjects) {
		go->pbr_mat.albedo = testMat.albedo;
		go->phong_mat.color = glm::vec4(testMat.albedo, 1.0f);
	}
}

Camera* Scene::GetCamera() {
	return camera.get();
}