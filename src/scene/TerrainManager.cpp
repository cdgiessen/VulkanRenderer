#include "TerrainManager.h"

#include "../../third-party/ImGui/imgui.h"

#include "../core/Logger.h"


TerrainManager::TerrainManager(InternalGraph::GraphPrototype& protoGraph) : protoGraph(protoGraph)
{
	if (settings.maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + 50 * settings.maxLevels; //with current quad density this is the average upper bound
											   //maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}

	//terrainQuadPool = std::make_shared<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>>();
	//nodeGraph.BuildNoiseGraph();


}

void TerrainManager::SetupResources(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer) {
	
	for (auto item : terrainTextureFileNames) {
			terrainTextureHandles.push_back(
				TerrainTextureNamedHandle(
					item, 
					resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TerrainTextures/" + item)));
	}
	
	terrainTextureArray = resourceMan->texManager.loadTextureArrayFromFile("assets/Textures/TerrainTextures/", terrainTextureFileNames);
	terrainVulkanTextureArray = std::make_shared<VulkanTexture2DArray>(renderer->device);
	terrainVulkanTextureArray->loadTextureArray(terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.GetTransferCommandBuffer(), 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);

	//WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
	//WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);

	instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	instancedWaters->SetBlendMode(VK_TRUE);
	instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width)));
	instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));
	instancedWaters->InitInstancedSceneObject(renderer);
}

void TerrainManager::CleanUpResources() {
	terrainVulkanTextureArray->destroy();

	instancedWaters->CleanUp();
	//WaterVulkanTexture.destroy(renderer->device);

	//	WaterModel.destroy(renderer->device);
}


TerrainManager::~TerrainManager()
{
	Log::Debug << "terrain manager deleted\n";
}

void AsyncTerrainCreation(
	std::shared_ptr<Terrain> terrainToCreate,
	std::shared_ptr<ResourceManager> resourceMan, 
	std::shared_ptr<VulkanRenderer> renderer, 
	std::shared_ptr<Camera> camera, 
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
	std::shared_ptr<MemoryPool<TerrainQuad>> pool, 
	InternalGraph::GraphPrototype& protoGraph,
	int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord) {
	
	auto terrain = std::make_shared<Terrain>(pool, protoGraph, numCells, maxLevels, heightScale, coord);

	std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();
	
	terrain->terrainSplatMap = resourceMan->texManager.loadTextureFromRGBAPixelData(sourceImageResolution + 1, sourceImageResolution + 1, imgData);

	terrain->InitTerrain(renderer, camera->Position, terrainVulkanTextureArray);


}

void TerrainManager::GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera) {
	this->renderer = renderer;
	settings.width = nextTerrainWidth;

	std::vector<std::thread> terrainCreators;

	for (int i = 0; i < settings.gridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < settings.gridDimentions; j++) {
			

			TerrainCoordinateData coord = TerrainCoordinateData(
				glm::vec2((i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2), //position
				glm::vec2(settings.width, settings.width), //size
				glm::i32vec2(i*settings.sourceImageResolution, j*settings.sourceImageResolution), //noise position
				glm::vec2(1.0 / (float)settings.sourceImageResolution, 1.0f / (float)settings.sourceImageResolution),//noiseSize 
				settings.sourceImageResolution + 1);

			/*terrainCreators.push_back(std::thread(AsyncTerrainCreation, 
				resourceMan, renderer, camera, terrainVulkanTextureArray,
				terrainQuadPool, protoGraph, settings.numCells, settings.maxLevels, settings.heightScale, coord
				));*/

			auto terrain = std::make_shared<Terrain>(terrainQuadPool, protoGraph, settings.numCells, settings.maxLevels, settings.heightScale, coord);

			std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();
			terrain->terrainSplatMap = resourceMan->texManager.loadTextureFromRGBAPixelData(settings.sourceImageResolution + 1, settings.sourceImageResolution + 1, imgData);
			//delete(imgData);
			terrains.push_back(terrain);
		}
	}
	for (auto& thread : terrainCreators) {
		thread.join();

	}

	//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	for (auto ter : terrains) {
		ter->InitTerrain(renderer, camera->Position, terrainVulkanTextureArray);
	}
	//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);

	//WaterMesh.reset();
	//WaterModel.destroy(renderer->device); 
	//WaterMesh = createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width));
	//WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);

	instancedWaters->CleanUp();
	instancedWaters.release();
	instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	instancedWaters->SetBlendMode(VK_TRUE);
	instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width)));
	instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));
	instancedWaters->InitInstancedSceneObject(renderer);

	for (int i = 0; i < settings.gridDimentions; i++) {
		for (int j = 0; j < settings.gridDimentions; j++) {
			InstancedSceneObject::InstanceData id;
			id.pos = glm::vec3((i - settings.gridDimentions / 2) * settings.width - settings.width / 2,
				0, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2);
			id.scale = 1;
			id.rot = glm::vec3(0, 0, 0);
			instancedWaters->AddInstance(id);

			//auto water = std::make_shared< Water>
			//	(64, (i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2, settings.width, settings.width);
			//
			////WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
			//water->InitWater(renderer, WaterVulkanTexture);
			//
			//waters.push_back(water);
		}
	}
	instancedWaters->UploadInstances();
	recreateTerrain = false;
}

void TerrainManager::UpdateTerrains(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager) {
	this->renderer = renderer;
	if (recreateTerrain) {
		CleanUpTerrain();
		GenerateTerrain(resourceMan, renderer, camera);
	}

	for (auto ter : terrains) {
		terrainUpdateTimer.StartTimer();
		//VkCommandBuffer copyCmdBuf = CreateTerrainMeshUpdateCommandBuffer(device->graphics_queue_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		ter->UpdateTerrain(camera->Position);

		//FlushTerrainMeshUpdateCommandBuffer(copyCmdBuf, device->graphics_queue, true);
		terrainUpdateTimer.EndTimer();
		//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
		//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
		//}
	}

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	for (auto ter : terrains) {
		ter->DrawTerrain(commandBuffer, wireframe);
	}

	instancedWaters->WriteToCommandBuffer(commandBuffer, wireframe);

}

//TODO : Reimplement getting height at terrain location
float TerrainManager::GetTerrainHeightAtLocation(float x, float z) {
	for (auto terrain : terrains)
	{
		glm::vec2 pos = terrain->coordinateData.pos;
		glm::vec2 size = terrain->coordinateData.size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z) {
			return terrain->GetHeightAtLocation((x - pos.x)/ settings.width, (z - pos.y)/ settings.width);
		}
	}
	return 0;
}

void TerrainManager::UpdateTerrainGUI() {

	ImGui::SetNextWindowSize(ImVec2(200, 220), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(220, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Debug Info", &settings.show_terrain_manager_window)) {
		ImGui::SliderFloat("Width", &nextTerrainWidth, 100, 10000);
		ImGui::SliderInt("Max Subdivision", &settings.maxLevels, 0, 10);
		ImGui::SliderInt("Grid Width", &settings.gridDimentions, 1, 10);
		ImGui::SliderFloat("Height Scale", &settings.heightScale, 1, 1000);
		ImGui::SliderInt("Image Resolution", &settings.sourceImageResolution, 32, 2048);

		if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
			recreateTerrain = true;
		}

		ImGui::Text("All terrains update Time: %u(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
		for (auto ter : terrains)
		{
			ImGui::Text("Terrain Draw Time: %u(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
			ImGui::Text("Terrain Quad Count %d", ter->numQuads);
		}
	}
	ImGui::End();

}

void TerrainManager::DrawTerrainTextureViewer() {

	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(0, 475), ImGuiSetCond_FirstUseEver);


	if (ImGui::Begin("Textures", &drawWindow, ImGuiWindowFlags_MenuBar))
	{

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Texture")) {

				}
				if (ImGui::MenuItem("Close"))
					drawWindow = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// left
		ImGui::BeginChild("left pane", ImVec2(150, 0), true);
		for (int i = 0; i < terrainTextureHandles.size(); i++)
		{
			char label[128];
			//lable = textureHandles.at(i).
			sprintf(label, "%s", terrainTextureHandles[i].name.c_str());
			if (ImGui::Selectable(label, selectedTexture == i))
				selectedTexture = i;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing())); // Leave room for 1 line below us
		ImGui::Text("MyObject: %d", selectedTexture);
		ImGui::Separator();


		ImGui::TextWrapped("TODO: Add texture preview");

		ImGui::EndChild();

		ImGui::BeginChild("buttons");
		if (ImGui::Button("PlaceHolder")) {}
		ImGui::EndChild();

		ImGui::EndGroup();

	}
	ImGui::End();

}

void TerrainManager::RecreateTerrain() {
	recreateTerrain = true;
}

void TerrainManager::CleanUpTerrain() {

	terrains.clear();
	waters.clear();
	//delete terrainQuadPool;
	//terrainQuadPool = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
}

/**
* Allocate a command buffer from the command pool
*
* @param level Level of the new command buffer (primary or secondary)
* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
*
* @return A handle to the allocated command buffer
*/
VkCommandBuffer TerrainManager::CreateTerrainMeshUpdateCommandBuffer(VkCommandPool commandPool, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = initializers::commandBufferAllocateInfo(commandPool, level, (uint32_t)terrains.size());

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(renderer->device.device, &cmdBufAllocateInfo, &cmdBuffer));


	VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();
	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

	return cmdBuffer;
}

/**
* Finish command buffer recording and submit it to a queue
*
* @param commandBuffer Command buffer to flush
* @param queue Queue to submit the command buffer to
* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
*
* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
* @note Uses a fence to ensure command buffer has finished executing
*/
void TerrainManager::FlushTerrainMeshUpdateCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}


	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = initializers::submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(renderer->device.device, &fenceInfo, nullptr, &fence));

	// Submit to the queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
	// Wait for the fence to signal that command buffer has finished executing
	VK_CHECK_RESULT(vkWaitForFences(renderer->device.device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

	vkDestroyFence(renderer->device.device, fence, nullptr);

	if (free)
	{
		vkFreeCommandBuffers(renderer->device.device, renderer->device.graphics_queue_command_pool, 1, &commandBuffer);
	}
}
