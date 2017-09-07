#include "Scene.h"



Scene::Scene(VulkanDevice* device) : device(device)
{
}

Scene::~Scene()
{
}

void Scene::PrepareScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain) {

	camera = new Camera(glm::vec3(-2, 2, 0), glm::vec3(0, 1, 0), 0, -45);

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

	skybox = new Skybox();
	skybox->InitSkybox(device, "Resources/Textures/Skybox/Skybox2", ".png", pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);

	GameObject* cubeObject = new GameObject();
	cubeObject->LoadModel(createCube());
	cubeObject->LoadTexture("Resources/Textures/ColorGradientCube.png");
	cubeObject->InitGameObject(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height, globalVariableBuffer, lightsInfoBuffer);
	gameObjects.push_back(cubeObject);
	
	terrainManager = new TerrainManager(device);
	terrainManager->GenerateTerrain(pipelineManager, renderPass, vulkanSwapChain, globalVariableBuffer, lightsInfoBuffer, camera);
}

void Scene::CreateUniformBuffers() {
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &globalVariableBuffer, sizeof(GlobalVariableUniformBuffer));
	device->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &lightsInfoBuffer, sizeof(PointLight) * pointLights.size());

	for (int i = 0; i < pointLights.size(); i++)
	{
		PointLight lbo;
		lbo.lightPos = pointLights[i].lightPos;
		lbo.color = pointLights[i].color;
		lbo.attenuation = pointLights[i].attenuation;

		lightsInfoBuffer.map(device->device, sizeof(PointLight), i * sizeof(PointLight));
		lightsInfoBuffer.copyTo(&lbo, sizeof(lbo));
		lightsInfoBuffer.unmap();
	}
}

void Scene::ReInitScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain) {
	skybox->ReinitSkybox(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	for (GameObject* obj : gameObjects) {
		obj->ReinitGameObject(device, pipelineManager, renderPass, vulkanSwapChain.swapChainExtent.width, vulkanSwapChain.swapChainExtent.height);
	}

	terrainManager->ReInitTerrain(pipelineManager, renderPass, vulkanSwapChain);
}

void Scene::UpdateScene(VulkanPipeline pipelineManager, VkRenderPass renderPass, VulkanSwapChain vulkanSwapChain, TimeManager* timeManager) {
	//if (walkOnGround) {
	//	//very choppy movement for right now, but since its just a quick 'n dirty way to put the camera at walking height, its just fine
	//	camera->Position.y = terrains.at(0)->terrainGenerator->SampleHeight(camera->Position.x, 0, camera->Position.z) * terrains.at(0)->heightScale + 2.0;
	//	if (camera->Position.y < 2) //for over water
	//		camera->Position.y = 2;
	//}

	GlobalVariableUniformBuffer cbo = {};
	cbo.view = camera->GetViewMatrix();
	cbo.proj = glm::perspective(glm::radians(45.0f), vulkanSwapChain.swapChainExtent.width / (float)vulkanSwapChain.swapChainExtent.height, 1.0f, 100000.0f);
	cbo.proj[1][1] *= -1;
	cbo.cameraDir = camera->Front;
	cbo.time = timeManager->GetRunningTime();

	globalVariableBuffer.map(device->device);
	globalVariableBuffer.copyTo(&cbo, sizeof(cbo));
	globalVariableBuffer.unmap();
	
	for (GameObject* obj : gameObjects) {
		obj->UpdateUniformBuffer(timeManager->GetRunningTime());
	}

	skybox->UpdateUniform(cbo.proj, camera->GetViewMatrix());

	terrainManager->UpdateTerrains(pipelineManager, renderPass, vulkanSwapChain, globalVariableBuffer, lightsInfoBuffer, camera, timeManager);
}

void Scene::RenderScene(VkCommandBuffer commandBuffer, bool wireframe) {
	VkDeviceSize offsets[] = { 0 };

	terrainManager->RenderTerrain(commandBuffer, wireframe);

	for (GameObject* obj : gameObjects) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj->pipelineLayout, 0, 1, &obj->descriptorSet, 0, nullptr);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? obj->wireframe : obj->pipeline);

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &obj->gameObjectModel.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, obj->gameObjectModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->gameObjectModel.indexCount), 1, 0, 0, 0);
	}

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
	for (GameObject* obj : gameObjects){
		obj->CleanUp();
	}

	terrainManager->CleanUpTerrain();

	globalVariableBuffer.cleanBuffer();
	lightsInfoBuffer.cleanBuffer();


}

Camera* Scene::GetCamera() {
	return camera;
}