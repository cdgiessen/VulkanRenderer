#include "TerrainManager.h"

#include <chrono>

#include "../../third-party/ImGui/imgui.h"

#include "../core/Logger.h"

TerrainCreationData::TerrainCreationData(
	std::shared_ptr<ResourceManager> resourceMan,
	std::shared_ptr<VulkanRenderer> renderer,
	std::shared_ptr<Camera> camera,
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
	std::shared_ptr<MemoryPool<TerrainQuad>> pool,
	InternalGraph::GraphPrototype& protoGraph,
	int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord):

	resourceMan(resourceMan),
	renderer(renderer),
	camera(camera),
	terrainVulkanTextureArray(terrainVulkanTextureArray),
	pool(pool),
	protoGraph(protoGraph),
	numCells(numCells),
	maxLevels(maxLevels),
	sourceImageResolution(sourceImageResolution),
	heightScale(heightScale),
	coord(coord)
{

}

void TerrainCreationWorker(TerrainManager* man) {

	while (man->isCreatingTerrain) {

		std::unique_lock<std::mutex> lock(man->workerMutex);
		man->workerConditionVariable.wait_for(lock, std::chrono::milliseconds(100));
		lock.unlock();

		if (!man->isCreatingTerrain)
			break;

		std::unique_lock<std::mutex> queue_lock(man->creationDataQueueMutex);
		if (!man->terrainCreationWork.empty()) {
			TerrainCreationData data{ man->terrainCreationWork.pop() };
			queue_lock.unlock();

			auto terrain = std::make_shared<Terrain>(data.pool, data.protoGraph, data.numCells, data.maxLevels, data.heightScale, data.coord);

			std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();

			terrain->terrainSplatMap = data.resourceMan->texManager.loadTextureFromRGBAPixelData(data.sourceImageResolution + 1, data.sourceImageResolution + 1, imgData);

			terrain->InitTerrain(data.renderer, data.camera->Position, data.terrainVulkanTextureArray);

			if (!man->isCreatingTerrain)
				break;

			{
				std::lock_guard<std::mutex> lk(man->terrain_mutex);
				man->terrains.push_back(terrain);
			}
			if (!man->isCreatingTerrain)
				break;
		}
		else {
			queue_lock.unlock();
		}

		if (!man->isCreatingTerrain)
			break;
	}

}


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

TerrainManager::~TerrainManager()
{
	isCreatingTerrain = false;
	for (auto& thread : terrainCreationWorkers) {
		thread.join();
	}
	terrainCreationWorkers.clear();
	//Log::Debug << "terrain manager deleted\n";
}

void TerrainManager::ResetWorkerThreads() {
	isCreatingTerrain = false;
	for (auto& thread : terrainCreationWorkers) {
		thread.join();
	}
	terrainCreationWorkers.clear();
	isCreatingTerrain = true;
	for (int i = 0; i < WorkerThreads; i++) {
		terrainCreationWorkers.push_back(std::thread(TerrainCreationWorker, this));
	}
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
	terrainVulkanTextureArray->loadTextureArray(terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, *renderer, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);

	//WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
	//WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);

	instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	instancedWaters->SetBlendMode(VK_TRUE);
	instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(1, 0, 1)));
	instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));
	instancedWaters->InitInstancedSceneObject(renderer);

}

void TerrainManager::CleanUpResources() {
	terrainVulkanTextureArray->destroy();

	instancedWaters->CleanUp();
	//WaterVulkanTexture.destroy(renderer->device);

	//	WaterModel.destroy(renderer->device);
}

void TerrainManager::GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera) {
	this->renderer = renderer;
	settings.width = nextTerrainWidth;

	ResetWorkerThreads();

	for (int i = 0; i < settings.gridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < settings.gridDimentions; j++) {
			

			TerrainCoordinateData coord = TerrainCoordinateData(
				glm::vec2((i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2), //position
				glm::vec2(settings.width, settings.width), //size
				glm::i32vec2(i*settings.sourceImageResolution, j*settings.sourceImageResolution), //noise position
				glm::vec2(1.0 / (float)settings.sourceImageResolution, 1.0f / (float)settings.sourceImageResolution),//noiseSize 
				settings.sourceImageResolution + 1);

			//auto terrain = std::make_shared<Terrain>(terrainQuadPool, protoGraph, settings.numCells, settings.maxLevels, settings.heightScale, coord);
			//
			//std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();
			//terrain->terrainSplatMap = resourceMan->texManager.loadTextureFromRGBAPixelData(settings.sourceImageResolution + 1, settings.sourceImageResolution + 1, imgData);
			////delete(imgData);
			//terrains.push_back(terrain);

			terrainCreationWork.push_back(TerrainCreationData(
			resourceMan, renderer, camera, terrainVulkanTextureArray,
			terrainQuadPool, protoGraph, 
			settings.numCells, settings.maxLevels, settings.sourceImageResolution, settings.heightScale, 
			coord));

			std::lock_guard<std::mutex> lk(workerMutex);
			workerConditionVariable.notify_one();
		}
	}
	//for (auto& thread : terrainCreators) {
	//	thread.join();

	//}

	for (auto ter : terrains) {
		ter->InitTerrain(renderer, camera->Position, terrainVulkanTextureArray);
	}

	//WaterMesh.reset();
	//WaterModel.destroy(renderer->device); 
	//WaterMesh = createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width));
	//WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);



	//instancedWaters->CleanUp();
	//instancedWaters.release();
	//instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	//instancedWaters->SetBlendMode(VK_TRUE);
	//instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	//instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	//instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width)));
	//instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));
	//instancedWaters->InitInstancedSceneObject(renderer);


	std::vector<InstancedSceneObject::InstanceData> waterData;
	for (int i = 0; i < settings.gridDimentions; i++) {
		for (int j = 0; j < settings.gridDimentions; j++) {
			InstancedSceneObject::InstanceData id;
			id.pos = glm::vec3((i - settings.gridDimentions / 2) * settings.width - settings.width / 2,
				0, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2);
			id.rot = glm::vec3(0, 0, 0);
			id.scale = settings.width;
			waterData.push_back(id);
			//instancedWaters->AddInstance(id);

			//auto water = std::make_shared< Water>
			//	(64, (i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2, settings.width, settings.width);
			//
			////WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
			//water->InitWater(renderer, WaterVulkanTexture);
			//
			//waters.push_back(water);
		}
	}
	instancedWaters->RemoveAllInstances(waterData);
	//instancedWaters->UploadInstances();
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

		terrain_mutex.lock();
		ter->UpdateTerrain(camera->Position);
		terrain_mutex.unlock();

		terrainUpdateTimer.EndTimer();
		//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
		//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
		//}
	}

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	terrain_mutex.lock();
	for (auto ter : terrains) {
		ter->DrawTerrain(commandBuffer, wireframe);
	}
	terrain_mutex.unlock();

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
			//char label[128];
			//sprintf(label, "%s", terrainTextureHandles[i].name.c_str());
			if (ImGui::Selectable(terrainTextureHandles[i].name.c_str(), selectedTexture == i))
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

