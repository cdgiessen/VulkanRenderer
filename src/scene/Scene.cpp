#include "Scene.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "../core/Input.h"
#include "../core/Logger.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

Scene::Scene()
{
}

Scene::~Scene()
{
	Log::Debug << "scene deleted\n";
}

void Scene::PrepareScene(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, InternalGraph::GraphPrototype& graph) {
	this->renderer = renderer;

	

	camera = std::make_shared< Camera>(glm::vec3(0, 1, -5), glm::vec3(0, 1, 0), 0, 90);


	//pointLights.resize(5);
	//pointLights[0] = PointLight(glm::vec4(0, 10, 0, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[1] = PointLight(glm::vec4(10, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[2] = PointLight(glm::vec4(50, 10, 10, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[3] = PointLight(glm::vec4(50, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	//pointLights[4] = PointLight(glm::vec4(75, 10, 75, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));

	skybox = std::make_shared<Skybox>();
	skybox->vulkanCubeMap = std::make_shared<VulkanCubeMap>(renderer->device);
	skybox->skyboxCubeMap = resourceMan->texManager.loadCubeMapFromFile("assets/Textures/Skybox/Skybox2", ".png");
	skybox->model = std::make_shared<VulkanModel>(renderer->device);
	skybox->model->loadFromMesh(createCube(),  *renderer);
	skybox->InitSkybox(renderer);	

	//std::shared_ptr<GameObject> cubeObject = std::make_shared<GameObject>();
	//cubeObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
	//cubeObject->LoadModel(createCube());
	//cubeObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
	//cubeObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/ColorGradientCube.png");
	////cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	//cubeObject->InitGameObject(renderer);
	//gameObjects.push_back(cubeObject);

	std::shared_ptr<GameObject> sphereObject = std::make_shared<GameObject>();
	sphereObject->gameObjectModel = std::make_shared<VulkanModel>(renderer->device);
	sphereObject->LoadModel(createSphere(10));
	sphereObject->gameObjectVulkanTexture = std::make_shared<VulkanTexture2D>(renderer->device);
	sphereObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/Red.png");
	sphereObject->InitGameObject(renderer);
	gameObjects.push_back(sphereObject);

	terrainManager = std::make_shared<TerrainManager>(graph);
	terrainManager->SetupResources(resourceMan, renderer);
	terrainManager->GenerateTerrain(resourceMan, renderer, camera);

	treesInstanced = std::make_shared<InstancedSceneObject>(renderer);
	treesInstanced->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/instancedSceneObject.frag.spv"));
	treesInstanced->SetBlendMode(VK_FALSE);
	treesInstanced->SetCullMode(VK_CULL_MODE_BACK_BIT);
	treesInstanced->LoadModel(createCube());
	treesInstanced->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/grass.jpg"));
	treesInstanced->InitInstancedSceneObject(renderer);
	treesInstanced->AddInstances({ glm::vec3(10,0,10),glm::vec3(10,0,20), glm::vec3(20,0,10), glm::vec3(10,0,40), glm::vec3(10,0,-40), glm::vec3(100,0,40) });

	rocksInstanced = std::make_shared<InstancedSceneObject>(renderer);

	// gltf2 integration
	//std::shared_ptr< gltf2::Asset> tree_test = std::make_shared<gltf2::Asset>();
	//*tree_test = gltf2::load("Resources/Assets/tree_test.gltf");

}

void Scene::UpdateScene(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<TimeManager> timeManager) {

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
		verticalVelocity += (float)timeManager->GetDeltaTime()*gravity;
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
	
	for (auto obj : gameObjects) {
		obj->UpdateUniformBuffer((float)timeManager->GetRunningTime());
	}

	if (Input::GetKeyDown(Input::KeyCode::V))
		UpdateTerrain = !UpdateTerrain;
	if (UpdateTerrain)
		terrainManager->UpdateTerrains(resourceMan, renderer, camera, timeManager);

	UpdateSunData();

	GlobalData gd;
	gd.time = (float)timeManager->GetRunningTime();

	glm::mat4 proj = depthReverserMatrix * glm::perspective(glm::radians(45.0f),
		renderer->vulkanSwapChain.swapChainExtent.width / (float)renderer->vulkanSwapChain.swapChainExtent.height,
		0.05f, 10000000.0f);
	proj[1][1] *= -1;

	CameraData cd;
	cd.view = camera->GetViewMatrix();
	cd.projView = proj * cd.view;
	cd.cameraDir = camera->Front;
	cd.cameraPos = camera->Position;

	DirectionalLight sun;
	sun.direction = sunSettings.dir;
	sun.intensity = sunSettings.intensity;
	sun.color = sunSettings.color;

	skybox->UpdateUniform(proj, cd.view );
	renderer->UpdateRenderResources(gd, cd, sun, pointLights, spotLights);

}

void Scene::RenderScene(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	terrainManager->RenderTerrain(commandBuffer, wireframe);
	
	for (auto obj : gameObjects) {
		obj->Draw(commandBuffer, wireframe, drawNormals);
	}
	
	treesInstanced->WriteToCommandBuffer(commandBuffer, wireframe);

	skybox->WriteToCommandBuffer(commandBuffer);
}

void Scene::UpdateSunData() {
	if (sunSettings.autoMove) {
		sunSettings.horizontalAngle += sunSettings.moveSpeed;
	}

	float X = glm::cos(sunSettings.horizontalAngle) * glm::cos(sunSettings.verticalAngle);
	float Z = glm::sin(sunSettings.horizontalAngle) * glm::cos(sunSettings.verticalAngle);
	float Y = glm::sin(sunSettings.verticalAngle);
	sunSettings.dir = glm::vec3(X, Y, Z);

}

void Scene::DrawSkySettingsGui() {
	ImGui::SetNextWindowPos(ImVec2(0, 675), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(356, 180), ImGuiSetCond_FirstUseEver);

	if (ImGui::Begin("Sky editor", &sunSettings.show_skyEditor)) {
		ImGui::Checkbox("Sun motion", &sunSettings.autoMove);
		ImGui::DragFloat("Sun Move Speed", &sunSettings.moveSpeed, 0.0001f, 0.0f, 0.0f, "%.5f");
		ImGui::SliderFloat("Sun Intensity", &sunSettings.intensity, 0.0f, 1.0f);
		ImGui::DragFloat("Sun Horizontal", &sunSettings.horizontalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat("Sun Vertical", &sunSettings.verticalAngle, 0.01f, 0.0f, 0.0f, "%.5f");
		ImGui::SliderFloat3("Sun Color", ((float*)glm::value_ptr(sunSettings.color)), 0.0f, 1.0f);
	}
	ImGui::End();
}

void Scene::UpdateSceneGUI(){
	terrainManager->UpdateTerrainGUI();
	terrainManager->DrawTerrainTextureViewer();

	DrawSkySettingsGui();
}

void Scene::CleanUpScene() {

	skybox->CleanUp();
	for (auto obj : gameObjects){
		obj->CleanUp();
	}

	terrainManager->CleanUpResources();
	terrainManager->CleanUpTerrain();
	treesInstanced->CleanUp();

}

std::shared_ptr<Camera> Scene::GetCamera() {
	return camera;
}