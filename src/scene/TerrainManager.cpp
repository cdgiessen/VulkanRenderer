#include "TerrainManager.h"

#include <chrono>
#include <algorithm>

#include <json.hpp>

#include "../../third-party/ImGui/imgui.h"

#include "../core/Logger.h"

template<size_t size>
ChunkBuffer<size>::ChunkBuffer(VulkanDevice& device, int count):
	buffer(device)
{
	buffer.CreateDataBuffer(count * size);
}

template<size_t size>
ChunkBuffer<size>::~ChunkBuffer(){
	buffer.CleanBuffer();
}
	
template<size_t size>
int ChunkBuffer<size>::Allocate(){
	for(int i = 0; i < freeList.size(); i++){
		if(freeList[i] == false){
			freeList[i] = true;
			return i;
		}
	}
}

template<size_t size>
void ChunkBuffer<size>::Free(int index){
	freeList[index] = false;
}

constexpr auto TerrainSettingsFileName = "terrain_settings.json";

TerrainCreationData::TerrainCreationData(
	ResourceManager* resourceMan,
	VulkanRenderer* renderer,
	glm::vec3 cameraPos,
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
	InternalGraph::GraphPrototype& protoGraph,
	int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord) :

	resourceMan(resourceMan),
	renderer(renderer),
	cameraPos(cameraPos),
	terrainVulkanTextureArray(terrainVulkanTextureArray),
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
		{
			std::unique_lock<std::mutex> lock(man->workerMutex);
			man->workerConditionVariable.wait(lock);
		}

		while (!man->terrainCreationWork.empty()) {
			auto data = man->terrainCreationWork.pop_if();
			if (data.has_value())
			{
				auto terrain = std::make_unique<Terrain>(man->renderer,
					man->poolMesh_vertices, man->poolMesh_indices,
					data->protoGraph, data->numCells, data->maxLevels,
					data->heightScale, data->coord);

				std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();

				terrain->terrainSplatMap = data->resourceMan->texManager.loadTextureFromRGBAPixelData(data->sourceImageResolution + 1, data->sourceImageResolution + 1, imgData);

				terrain->InitTerrain(data->renderer, data->cameraPos, data->terrainVulkanTextureArray);

				{
					std::lock_guard<std::mutex> lk(man->terrain_mutex);
					man->terrains.push_back(std::move(terrain));
				}

			}
			//break out of loop if work shouldn't be continued
			if (!man->isCreatingTerrain)
				return;
		}
	}

}


TerrainManager::TerrainManager(InternalGraph::GraphPrototype& protoGraph)
	: protoGraph(protoGraph), 
	poolMesh_vertices(), poolMesh_indices()
	//,
	// buffChunks_vets(renderer->device, MaxChunkCount),
	// buffChunks_inds(renderer->device, MaxChunkCount)

{
	if (settings.maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + 50 * settings.maxLevels; //with current quad density this is the average upper bound
											   //maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}
	LoadSettingsFromFile();
	//terrainQuadPool = std::make_shared<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>>();
	//poolMesh_vertices = MemoryPool<TerrainMeshVertices>(500);
	//poolMesh_indices = MemoryPool<TerrainMeshIndices(500);



}

TerrainManager::~TerrainManager()
{
}

void TerrainManager::StartWorkerThreads() {
	isCreatingTerrain = true;
	for (int i = 0; i < WorkerThreads; i++) {
		terrainCreationWorkers.push_back(std::thread(TerrainCreationWorker, this));
	}
}

void TerrainManager::StopWorkerThreads() {
	isCreatingTerrain = false;
	workerConditionVariable.notify_all();
	for (auto& thread : terrainCreationWorkers) {
		thread.join();
	}
	terrainCreationWorkers.clear();
}

void TerrainManager::SetupResources(ResourceManager* resourceMan, VulkanRenderer* renderer) {

	this->renderer = renderer;

	for (auto& item : terrainTextureFileNames) {
		terrainTextureHandles.push_back(
			TerrainTextureNamedHandle(
				item,
				resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TerrainTextures/" + item)));
	}

	terrainTextureArray = resourceMan->texManager.loadTextureArrayFromFile("assets/Textures/TerrainTextures/", terrainTextureFileNames);

	terrainVulkanTextureArray = std::make_shared<VulkanTexture2DArray>(renderer->device);
	terrainVulkanTextureArray->loadTextureArray(terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, *renderer,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);

	//WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
	//WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);

	instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	instancedWaters->SetFragmentShaderToUse("assets/shaders/water.frag.spv");
	instancedWaters->SetBlendMode(VK_TRUE);
	instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(1, 0, 1)));
	instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));

	instancedWaters->InitInstancedSceneObject(renderer);

	StartWorkerThreads();

}

void TerrainManager::CleanUpResources() {
	terrainVulkanTextureArray->destroy();

	instancedWaters->CleanUp();

	//buffChunks_vets.CleanUp();
	//buffChunks_inds.CleanUp();
}

void TerrainManager::GenerateTerrain(ResourceManager* resourceMan, VulkanRenderer* renderer, std::shared_ptr<Camera> camera) {
	this->renderer = renderer;
	settings.width = nextTerrainWidth;

	//ResetWorkerThreads();

	for (int i = 0; i < settings.gridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < settings.gridDimentions; j++) {

			glm::ivec2 terGrid(i - settings.gridDimentions / 2, j - settings.gridDimentions / 2);

			TerrainCoordinateData coord = TerrainCoordinateData(
				glm::vec2((i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2), //position
				glm::vec2(settings.width, settings.width), //size
				glm::i32vec2(i*settings.sourceImageResolution, j*settings.sourceImageResolution), //noise position
				glm::vec2(1.0 / (float)settings.sourceImageResolution, 1.0f / (float)settings.sourceImageResolution),//noiseSize 
				settings.sourceImageResolution + 1,
				terGrid);

			terrainCreationWork.push_back(TerrainCreationData(
				resourceMan, renderer, camera->Position, terrainVulkanTextureArray,
				protoGraph,
				settings.numCells, settings.maxLevels, settings.sourceImageResolution, settings.heightScale,
				coord));

		}
	}
	std::lock_guard<std::mutex> lk(workerMutex);
	workerConditionVariable.notify_one();

	std::vector<InstancedSceneObject::InstanceData> waterData;
	for (int i = 0; i < settings.gridDimentions; i++) {
		for (int j = 0; j < settings.gridDimentions; j++) {
			InstancedSceneObject::InstanceData id;
			id.pos = glm::vec3((i - settings.gridDimentions / 2) * settings.width - settings.width / 2,
				0, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2);
			id.rot = glm::vec3(0, 0, 0);
			id.scale = settings.width;
			waterData.push_back(id);
		}
	}
	instancedWaters->ReplaceAllInstances(waterData);
	//instancedWaters->UploadInstances();
	recreateTerrain = false;
}

void TerrainManager::UpdateTerrains(ResourceManager* resourceMan,
	glm::vec3 cameraPos)
{
	if (recreateTerrain) {
		StopWorkerThreads();
		CleanUpTerrain();
		StartWorkerThreads();
		//need to rework to involve remaking the graph
		//GenerateTerrain(resourceMan, renderer, camera);
		recreateTerrain = false;
	}

	terrainUpdateTimer.StartTimer();

	std::vector<std::vector<std::unique_ptr<Terrain>>::iterator> terToDelete;

	//delete terrains too far away
	terrain_mutex.lock();


	for (auto it = std::begin(terrains); it != std::end(terrains); it++) {
		glm::vec3 center = glm::vec3((*it)->coordinateData.pos.x, cameraPos.y, (*it)->coordinateData.pos.y);
		float distanceToViewer = glm::distance(cameraPos, center);
		if (distanceToViewer > settings.viewDistance * settings.width * 1.5) {

			terToDelete.push_back(it);
			//Log::Debug << "deleting terrain at x:" << (*it)->coordinateData.noisePos.x / (*it)->coordinateData.sourceImageResolution
			//	<< " z: " << (*it)->coordinateData.noisePos.y / (*it)->coordinateData.sourceImageResolution << "\n";
			auto activeIt = std::find(std::begin(activeTerrains), std::end(activeTerrains), (*it)->coordinateData.gridPos);
			activeTerrains.erase(activeIt);

			InstancedSceneObject::InstanceData water;
			water.pos = glm::vec3((*it)->coordinateData.pos.x, 0, (*it)->coordinateData.pos.y);
			water.rot = glm::vec3(0, 0, 0);
			water.scale = settings.width;
			instancedWaters->RemoveInstance(water);

		}
	}
	while (terToDelete.size() > 0) {
		terrains.erase(terToDelete.back());
		terToDelete.pop_back();
	}

	terrain_mutex.unlock();

	//make new closer terrains

	glm::ivec2 camGrid((int)((cameraPos.x + 0 * settings.width / 2.0) / settings.width),
		(int)((cameraPos.z + 0 * settings.width / 2.0) / settings.width));


	//Log::Debug << "cam grid x: " << camGridX << " z: " << camGridZ << "\n";
	for (int i = 0; i < settings.viewDistance * 2; i++) {
		for (int j = 0; j < settings.viewDistance * 2; j++) {

			glm::ivec2 terGrid(camGrid.x + i - settings.viewDistance, camGrid.y + j - settings.viewDistance);

			glm::vec3 center = glm::vec3(terGrid.x * settings.width, cameraPos.y, terGrid.y * settings.width);
			float distanceToViewer = glm::distance(cameraPos, center);

			//if (distanceToViewer <= settings.viewDistance * settings.width) {

				//Log::Debug << "noisePosX " << ter->coordinateData.noisePos.x/ter->coordinateData.sourceImageResolution << "\n";
				//Log::Debug << "relX " << camGridX + i - settings.viewDistance / 2.0 << "\n";

				//see if there are any terrains already there
			bool found = false;
			for (auto& ter : activeTerrains) {
				if (ter.x == terGrid.x && ter.y == terGrid.y)
				{
					found = true;
				}
			}
			if (!found) {
				// Log::Debug << "creating new terrain at x:" << terGrid.x << " z: " << terGrid.y << "\n";

				activeTerrains.push_back(terGrid);

				auto pos = glm::vec2((terGrid.x)* settings.width - settings.width / 2,
					(terGrid.y)* settings.width - settings.width / 2);

				TerrainCoordinateData coord = TerrainCoordinateData(
					pos, //position
					glm::vec2(settings.width, settings.width), //size
					glm::i32vec2((terGrid.x)*settings.sourceImageResolution,
					(terGrid.y)*settings.sourceImageResolution), //noise position
					glm::vec2(1.0 / (float)settings.sourceImageResolution, 1.0f / (float)settings.sourceImageResolution),//noiseSize 
					settings.sourceImageResolution + 1,
					terGrid);

				terrain_mutex.lock();
				terrainCreationWork.push_back(TerrainCreationData(
					resourceMan, renderer, cameraPos, terrainVulkanTextureArray,
					protoGraph,
					settings.numCells, settings.maxLevels, settings.sourceImageResolution, settings.heightScale,
					coord));
				terrain_mutex.unlock();
				workerConditionVariable.notify_one();

				InstancedSceneObject::InstanceData water;
				water.pos = glm::vec3(pos.x, 0, pos.y);
				water.rot = glm::vec3(0, 0, 0);
				water.scale = settings.width;
				instancedWaters->AddInstance(water);
			}
			//}
		}
	}



	//update all terrains
	terrain_mutex.lock();
	for (auto& ter : terrains) {
		ter->UpdateTerrain(cameraPos);
	}
	terrain_mutex.unlock();

	//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
	//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
	//}
	terrainUpdateTimer.EndTimer();

	instancedWaters->UploadData();
}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	{
		std::lock_guard<std::mutex> lock(terrain_mutex);
		for (auto& ter : terrains) {
			ter->DrawTerrain(commandBuffer, wireframe);
		}
	}

	instancedWaters->WriteToCommandBuffer(commandBuffer, wireframe);
}

//TODO : Reimplement getting height at terrain location
float TerrainManager::GetTerrainHeightAtLocation(float x, float z) {
	for (auto& terrain : terrains)
	{
		glm::vec2 pos = terrain->coordinateData.pos;
		glm::vec2 size = terrain->coordinateData.size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z) {
			return terrain->GetHeightAtLocation((x - pos.x) / settings.width, (z - pos.y) / settings.width);
		}
	}
	return 0;
}

void TerrainManager::SaveSettingsToFile() {
	nlohmann::json j;

	j["show_window"] = settings.show_terrain_manager_window;
	j["terrain_width"] = settings.width;
	j["height_scale"] = settings.heightScale;
	j["max_levels"] = settings.maxLevels;
	j["grid_dimentions"] = settings.gridDimentions;
	j["view_distance"] = settings.viewDistance;
	j["souce_iamge_resolution"] = settings.sourceImageResolution;
	j["worker_threads"] = settings.workerThreads;

	std::ofstream outFile(TerrainSettingsFileName);
	outFile << std::setw(4) << j;
	outFile.close();
}

void TerrainManager::LoadSettingsFromFile() {
	if (fileExists(TerrainSettingsFileName)) {
		std::ifstream input(TerrainSettingsFileName);

		nlohmann::json j;
		input >> j;

		settings.show_terrain_manager_window = j["show_window"];
		settings.width = j["terrain_width"];
		settings.heightScale = j["height_scale"];
		settings.maxLevels = j["max_levels"];
		settings.gridDimentions = j["grid_dimentions"];
		settings.viewDistance = j["view_distance"];
		settings.sourceImageResolution = j["souce_iamge_resolution"];
		settings.workerThreads = j["worker_threads"];
		if (settings.workerThreads < 1)
			settings.workerThreads = 1;
	}
	else {

		settings = GeneralSettings{};
		SaveSettingsToFile();
	}
}


void TerrainManager::UpdateTerrainGUI() {

	ImGui::SetNextWindowSize(ImVec2(200, 220), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(220, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Terrain Info", &settings.show_terrain_manager_window)) {

		ImGui::BeginGroup();
		if (ImGui::Button("Save")) {
			SaveSettingsToFile();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load")) {
			LoadSettingsFromFile();
		}
		ImGui::EndGroup();

		ImGui::SliderFloat("Width", &nextTerrainWidth, 100, 10000);
		ImGui::SliderInt("Max Subdivision", &settings.maxLevels, 0, 10);
		ImGui::SliderInt("Grid Width", &settings.gridDimentions, 1, 10);
		ImGui::SliderFloat("Height Scale", &settings.heightScale, 1, 1000);
		ImGui::SliderInt("Image Resolution", &settings.sourceImageResolution, 32, 2048);
		ImGui::SliderInt("View Distance", &settings.viewDistance, 1, 32);

		if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
			recreateTerrain = true;
		}
		ImGui::Text("Terrain Count %lu", terrains.size());
		ImGui::Text("Generating %i Terrains", terrainCreationWork.size());
		ImGui::Text("All terrains update Time: %lu(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
		for (auto& ter : terrains)
		{
			ImGui::Text("Terrain Draw Time: %lu(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
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

	StopWorkerThreads();
	terrains.clear();
	//waters.clear();
	//instancedWaters->RemoveAllInstances();
	//instancedWaters->CleanUp();
	activeTerrains.clear();


	//delete terrainQuadPool;
	//terrainQuadPool = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
}

