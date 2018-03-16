#include "Scene.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "../core/Input.h"
#include "../core/Logger.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

Scene::Scene() : renderer()
{
}

Scene::~Scene()
{
	Log::Debug << "scene deleted\n";
}

void Scene::PrepareScene(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, InternalGraph::GraphPrototype& graph) {
	this->renderer = renderer;

	camera = std::make_shared< Camera>(glm::vec3(-2, 2, 0), glm::vec3(0, 1, 0), 0, -45);

	pointLights.resize(5);
	pointLights[0] = PointLight(glm::vec4(0, 10, 0, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[1] = PointLight(glm::vec4(10, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[2] = PointLight(glm::vec4(50, 10, 10, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[3] = PointLight(glm::vec4(50, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[4] = PointLight(glm::vec4(75, 10, 75, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));

	skybox = std::make_shared< Skybox>();
	skybox->skyboxCubeMap = resourceMan->texManager.loadCubeMapFromFile("assets/Textures/Skybox/Skybox2", ".png");
	skybox->model.loadFromMesh(createCube(), renderer->device, renderer->device.graphics_queue);
	skybox->InitSkybox(renderer);

	std::shared_ptr<GameObject> cubeObject = std::make_shared<GameObject>();
	cubeObject->LoadModel(createCube());
	cubeObject->gameObjectTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/ColorGradientCube.png");
	//cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	cubeObject->InitGameObject(renderer);
	gameObjects.push_back(cubeObject);


	terrainManager = std::make_shared<TerrainManager>(graph);
	terrainManager->SetupResources(resourceMan, renderer);
	terrainManager->GenerateTerrain(resourceMan, renderer, camera);

	treesInstanced = std::make_shared<InstancedSceneObject>();
	treesInstanced->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/instancedSceneObject.frag.spv"));
	treesInstanced->LoadModel(createCube());
	treesInstanced->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/grass.jpg"));
	treesInstanced->InitInstancedSceneObject(renderer);
	treesInstanced->AddInstances({ glm::vec3(10,0,10),glm::vec3(10,0,20), glm::vec3(20,0,10), glm::vec3(10,0,40), glm::vec3(10,0,-40), glm::vec3(100,0,40) });

	rocksInstanced = std::make_shared<InstancedSceneObject>();

	// gltf2 integration
	//std::shared_ptr< gltf2::Asset> tree_test = std::make_shared<gltf2::Asset>();
	//*tree_test = gltf2::load("Resources/Assets/tree_test.gltf");

}

void Scene::UpdateScene(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<TimeManager> timeManager) {
	if (walkOnGround) {
		float groundHeight = terrainManager->GetTerrainHeightAtLocation(camera->Position.x, camera->Position.z) + 2.0f;
		float height = camera->Position.y;
		if (Input::GetKeyDown(Input::KeyCode::SPACE)) {
			verticalVelocity += 0.15f;
		}
		verticalVelocity += (float)gravity*timeManager->GetDeltaTime();
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

	//skybox->UpdateUniform(cbo.proj, camera->GetViewMatrix());
	//if(!Input::GetKey(GLFW_KEY_V))
	terrainManager->UpdateTerrains(resourceMan, renderer, camera, timeManager);

	UpdateSunData();

	GlobalVariableUniformBuffer cbo = {};
	cbo.view = camera->GetViewMatrix();
	cbo.proj = depthReverserMatrix * glm::perspective(glm::radians(45.0f), 
		renderer->vulkanSwapChain.swapChainExtent.width / (float)renderer->vulkanSwapChain.swapChainExtent.height, 
		0.05f, 10000000.0f);
	cbo.proj[1][1] *= -1;
	cbo.cameraDir = camera->Front;
	cbo.time = (float)timeManager->GetRunningTime();
	cbo.sunDir = sunSettings.dir;
	cbo.sunIntensity = sunSettings.intensity;
	cbo.sunColor = sunSettings.color;

	renderer->UpdateGlobalRenderResources(cbo, pointLights);
}

void Scene::RenderScene(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	terrainManager->RenderTerrain(commandBuffer, wireframe);
	
	for (auto obj : gameObjects) {
		obj->Draw(commandBuffer, wireframe, drawNormals);
	}
	
	treesInstanced->WriteToCommandBuffer(commandBuffer, wireframe);

	//skybox
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->mvp->layout, 2, 1, &skybox->m_descriptorSet.set, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->mvp->pipelines->at(0));

	skybox->model.BindModel(commandBuffer);
	//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skybox->model.vmaBufferVertex, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, skybox->model.vmaBufferIndex, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(skybox->model.indexCount), 1, 0, 0, 0);

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
		ImGui::DragFloat("Sun Intensity", &sunSettings.intensity, 0.0f, 1.0f);
		ImGui::DragFloat("Sun Horizontal", &sunSettings.horizontalAngle, 0.001f, 0.0f, 0.0f, "%.5f");
		ImGui::DragFloat("Sun Vertical", &sunSettings.verticalAngle, 0.001f, 0.0f, 0.0f, "%.5f");
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