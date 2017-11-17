#include "Scene.h"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

Scene::Scene() : renderer()
{
}

Scene::~Scene()
{
}

void Scene::PrepareScene(std::shared_ptr<VulkanRenderer> renderer) {
	this->renderer = renderer;
	
	camera = std::make_shared< Camera>(glm::vec3(-2, 2, 0), glm::vec3(0, 1, 0), 0, -45);

	pointLights.resize(5);
	pointLights[0] = PointLight(glm::vec4(0, 10, 0, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[1] = PointLight(glm::vec4(10, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[2] = PointLight(glm::vec4(50, 10, 10, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[3] = PointLight(glm::vec4(50, 10, 50, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));
	pointLights[4] = PointLight(glm::vec4(75, 10, 75, 1), glm::vec4(0, 0, 0, 0), glm::vec4(1.0, 0.045f, 0.0075f, 1.0f));

	CreateUniformBuffers();

	//Setup buffer descriptors
	globalVariableBuffer.setupDescriptor();
	lightsInfoBuffer.setupDescriptor();

	skybox = std::make_shared< Skybox>();
	skybox->InitSkybox(renderer, "Resources/Textures/Skybox/Skybox2", ".png");

	std::shared_ptr<GameObject> cubeObject = std::make_shared<GameObject>();
	cubeObject->LoadModel(createCube());
	cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	cubeObject->InitGameObject(renderer, globalVariableBuffer, lightsInfoBuffer);
	gameObjects.push_back(cubeObject);
	
	terrainManager = std::make_shared<TerrainManager>();
	terrainManager->GenerateTerrain(renderer, globalVariableBuffer, lightsInfoBuffer, camera);

	treesInstanced = std::make_shared<InstancedSceneObject>();
	treesInstanced->LoadModel(createCube());
	treesInstanced->LoadTexture("Resources/Textures/grass.jpg");
	treesInstanced->InitInstancedSceneObject(renderer, globalVariableBuffer, lightsInfoBuffer);
	treesInstanced->AddInstances({ glm::vec3(10,0,10),glm::vec3(10,0,20), glm::vec3(20,0,10), glm::vec3(10,0,40), glm::vec3(10,0,-40), glm::vec3(100,0,40) });

	rocksInstanced = std::make_shared<InstancedSceneObject>();
}

void Scene::CreateUniformBuffers() {
	renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, renderer->device.uniformBufferMemPropertyFlags,
		&globalVariableBuffer, sizeof(GlobalVariableUniformBuffer));
	renderer->device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, renderer->device.uniformBufferMemPropertyFlags,
		&lightsInfoBuffer, sizeof(PointLight) * pointLights.size());

	for (int i = 0; i < pointLights.size(); i++)
	{
		PointLight lbo;
		lbo.lightPos = pointLights[i].lightPos;
		lbo.color = pointLights[i].color;
		lbo.attenuation = pointLights[i].attenuation;

		lightsInfoBuffer.map(renderer->device.device, sizeof(PointLight), i * sizeof(PointLight));
		lightsInfoBuffer.copyTo(&lbo, sizeof(lbo));
		lightsInfoBuffer.unmap();
	}
}

void Scene::ReInitScene(std::shared_ptr<VulkanRenderer> renderer) {
	skybox->ReinitSkybox(renderer);
	for (auto obj : gameObjects) {
		obj->ReinitGameObject(renderer);
	}

	treesInstanced->ReinitInstancedSceneObject(renderer);
	terrainManager->ReInitTerrain(renderer);
}

void Scene::UpdateScene(std::shared_ptr<TimeManager> timeManager) {
	//if (walkOnGround) {
	//	//very choppy movement for right now, but since its just a quick 'n dirty way to put the camera at walking height, its just fine
	//	camera->Position.y = terrains.at(0)->terrainGenerator->SampleHeight(camera->Position.x, 0, camera->Position.z) * terrains.at(0)->heightScale + 2.0;
	//	if (camera->Position.y < 2) //for over water
	//		camera->Position.y = 2;
	//}

	glm::mat4 deptheReverser = glm::mat4(1, 0, 0, 0,	0, 1, 0, 0,		0, 0, -1, 0,	0, 0, 1, 1);

	GlobalVariableUniformBuffer cbo = {};
	cbo.view = camera->GetViewMatrix();
	cbo.proj = deptheReverser * glm::perspective(glm::radians(45.0f), renderer->vulkanSwapChain.swapChainExtent.width / (float)renderer->vulkanSwapChain.swapChainExtent.height, 0.05f, 100000.0f);
	cbo.proj[1][1] *= -1;
	cbo.cameraDir = camera->Front;
	cbo.time = (float)timeManager->GetRunningTime();

	globalVariableBuffer.map(renderer->device.device);
	globalVariableBuffer.copyTo(&cbo, sizeof(cbo));
	globalVariableBuffer.unmap();
	
	for (auto obj : gameObjects) {
		obj->UpdateUniformBuffer((float)timeManager->GetRunningTime());
	}

	skybox->UpdateUniform(cbo.proj, camera->GetViewMatrix());

	terrainManager->UpdateTerrains(renderer, globalVariableBuffer, lightsInfoBuffer, camera, timeManager);
}

void Scene::RenderScene(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	terrainManager->RenderTerrain(commandBuffer, wireframe);
	
	for (auto obj : gameObjects) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj->pipelineLayout, 0, 1, &obj->descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? obj->wireframe : obj->pipeline);

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &obj->gameObjectModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, obj->gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->gameObjectModel.indexCount), 1, 0, 0, 0);

		if (drawNormals) {
			bool drawNormals = false;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj->debugNormals);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->gameObjectModel.indexCount), 1, 0, 0, 0);
		}
	}
	
	treesInstanced->WriteToCommandBuffer(commandBuffer, wireframe);

	//skybox
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->pipelineLayout, 0, 1, &skybox->descriptorSet, 0, nullptr);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->pipeline);

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skybox->model.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, skybox->model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(skybox->model.indexCount), 1, 0, 0, 0);

}

void Scene::UpdateSceneGUI(){
	terrainManager->UpdateTerrainGUI();
}

void Scene::CleanUpScene() {

	skybox->CleanUp();
	for (auto obj : gameObjects){
		obj->CleanUp();
	}

	terrainManager->CleanUpTerrain();
	treesInstanced->CleanUp();

	globalVariableBuffer.cleanBuffer();
	lightsInfoBuffer.cleanBuffer();


}

std::shared_ptr<Camera> Scene::GetCamera() {
	return camera;
}