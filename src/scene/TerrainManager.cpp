#include "TerrainManager.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>

#include <nlohmann/json.hpp>

#include <ImGui/imgui.h>

#include "core/Logger.h"
#include "rendering/Initializers.h"


constexpr auto TerrainSettingsFileName = "terrain_settings.json";

TerrainCreationData::TerrainCreationData (
    int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord)
: numCells (numCells),
  maxLevels (maxLevels),
  sourceImageResolution (sourceImageResolution),
  heightScale (heightScale),
  coord (coord)
{
}


TerrainManager::TerrainManager (
    InternalGraph::GraphPrototype& protoGraph, Resource::AssetManager& resourceMan, VulkanRenderer& renderer)
: resourceMan (resourceMan), renderer (renderer), protoGraph (protoGraph)
{
	if (settings.maxLevels < 0)
	{
		settings.maxLevels = 0;
	}
	LoadSettingsFromFile ();

	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8);

	terrainTextureArrayAlbedo = resourceMan.texManager.GetTexIDByName ("terrain_albedo");
	terrainVulkanTextureArrayAlbedo =
	    renderer.textureManager.CreateTexture2DArray (terrainTextureArrayAlbedo, details);

	terrainTextureArrayRoughness = resourceMan.texManager.GetTexIDByName ("terrain_roughness");
	terrainVulkanTextureArrayRoughness =
	    renderer.textureManager.CreateTexture2DArray (terrainTextureArrayRoughness, details);

	terrainTextureArrayMetallic = resourceMan.texManager.GetTexIDByName ("terrain_metalness");
	terrainVulkanTextureArrayMetallic =
	    renderer.textureManager.CreateTexture2DArray (terrainTextureArrayMetallic, details);

	terrainTextureArrayNormal = resourceMan.texManager.GetTexIDByName ("terrain_normal");
	terrainVulkanTextureArrayNormal =
	    renderer.textureManager.CreateTexture2DArray (terrainTextureArrayNormal, details);

	terrainGridMesh = createFlatPlane (settings.numCells, glm::vec3 (1.0f));
	terrainGridModel = std::make_shared<VulkanModel> (renderer, terrainGridMesh);

	// StartWorkerThreads ();
	workContinueSignal = std::make_shared<job::TaskSignal> ();
}

TerrainManager::~TerrainManager () { CleanUpTerrain (); }


void TerrainManager::StopActiveJobs ()
{
	workContinueSignal->Cancel ();
	workContinueSignal->Wait ();
}


void TerrainManager::CleanUpTerrain ()
{
	StopActiveJobs ();
	terrains.clear ();
	activeTerrains.clear ();
}


void TerrainManager::UpdateTerrains (glm::vec3 cameraPos)
{
	curCameraPos = cameraPos;

	if (recreateTerrain)
	{

		// StopWorkerThreads ();
		CleanUpTerrain ();
		workContinueSignal = std::make_shared<job::TaskSignal> ();
		/*
		    StartWorkerThreads ();
		*/	// need to rework to involve remaking the graph
		// GenerateTerrain(resourceMan, renderer, camera);
		recreateTerrain = false;
	}

	terrainUpdateTimer.StartTimer ();

	std::vector<std::vector<std::unique_ptr<Terrain>>::iterator> terToDelete;

	// delete terrains too far away
	terrain_mutex.lock ();


	for (auto it = std::begin (terrains); it != std::end (terrains); it++)
	{
		glm::vec3 center =
		    glm::vec3 ((*it)->coordinateData.pos.x, cameraPos.y, (*it)->coordinateData.pos.y);
		float distanceToViewer = glm::distance (cameraPos, center);
		if (distanceToViewer > settings.viewDistance * settings.width * 1.5)
		{
			if ((*(*it)->terrainVulkanSplatMap->readyToUse) == true)
			{

				terToDelete.push_back (it);
				// Log::Debug << "deleting terrain at x:" << (*it)->coordinateData.noisePos.x / (*it)->coordinateData.sourceImageResolution
				//	<< " z: " << (*it)->coordinateData.noisePos.y / (*it)->coordinateData.sourceImageResolution << "\n";
				auto activeIt = std::find (
				    std::begin (activeTerrains), std::end (activeTerrains), (*it)->coordinateData.gridPos);
				if (activeIt != std::end (activeTerrains)) activeTerrains.erase (activeIt);
			}
		}
	}
	while (terToDelete.size () > 0)
	{
		terrains.erase (terToDelete.back ());
		terToDelete.pop_back ();
	}

	terrain_mutex.unlock ();

	// make new closer terrains

	glm::ivec2 camGrid ((int)((cameraPos.x + 0 * settings.width / 2.0) / settings.width),
	    (int)((cameraPos.z + 0 * settings.width / 2.0) / settings.width));


	// Log::Debug << "cam grid x: " << camGridX << " z: " << camGridZ << "\n";
	for (int i = 0; i < settings.viewDistance * 2; i++)
	{
		for (int j = 0; j < settings.viewDistance * 2; j++)
		{

			glm::ivec2 terGrid (camGrid.x + i - settings.viewDistance, camGrid.y + j - settings.viewDistance);

			glm::vec3 center =
			    glm::vec3 (terGrid.x * settings.width, cameraPos.y, terGrid.y * settings.width);
			float distanceToViewer = glm::distance (cameraPos, center);

			// if (distanceToViewer <= settings.viewDistance * settings.width) {

			// Log::Debug << "noisePosX " << ter->coordinateData.noisePos.x/ter->coordinateData.sourceImageResolution << "\n";
			// Log::Debug << "relX " << camGridX + i - settings.viewDistance / 2.0 << "\n";

			// see if there are any terrains already there
			bool found = false;
			for (auto& ter : activeTerrains)
			{
				if (ter.x == terGrid.x && ter.y == terGrid.y)
				{
					found = true;
				}
			}
			if (!found)
			{
				// Log::Debug << "creating new terrain at x:" << terGrid.x << " z: " << terGrid.y << "\n";

				activeTerrains.push_back (terGrid);

				auto pos = glm::vec2 ((terGrid.x) * settings.width - settings.width / 2,
				    (terGrid.y) * settings.width - settings.width / 2);

				TerrainCoordinateData coord = TerrainCoordinateData (pos, // position
				    glm::vec2 (settings.width, settings.width),           // size
				    glm::i32vec2 ((terGrid.x) * settings.sourceImageResolution,
				        (terGrid.y) * settings.sourceImageResolution), // noise position
				    glm::vec2 (1.0 / (float)settings.sourceImageResolution,
				        1.0f / (float)settings.sourceImageResolution), // noiseSize
				    settings.sourceImageResolution + 1,
				    terGrid);

				auto t = job::Task (workContinueSignal, [this, coord] {
					auto terCreateData = TerrainCreationData (settings.numCells,
					    settings.maxLevels,
					    settings.sourceImageResolution,
					    settings.heightScale,
					    coord);

					glm::vec3 center = glm::vec3 (
					    terCreateData.coord.pos.x, curCameraPos.y, terCreateData.coord.pos.y);
					float distanceToViewer = glm::distance (curCameraPos, center);
					if (distanceToViewer < settings.viewDistance * settings.width * 1.5)
					{
						auto terrain = std::make_unique<Terrain> (renderer,

						    protoGraph,
						    terCreateData.numCells,
						    terCreateData.maxLevels,
						    terCreateData.heightScale,
						    terCreateData.coord,
						    terrainGridModel.get ());

						terrain->InitTerrain (curCameraPos,
						    terrainVulkanTextureArrayAlbedo,
						    terrainVulkanTextureArrayRoughness,
						    terrainVulkanTextureArrayMetallic,
						    terrainVulkanTextureArrayNormal);
						{
							std::lock_guard<std::mutex> lk (terrain_mutex);
							terrains.push_back (std::move (terrain));
						}
					}
				});

				taskManager.Submit (std::move (t), job::TaskType::currentFrame);
			}
		}
	}



	// update all terrains
	terrain_mutex.lock ();
	for (auto& ter : terrains)
	{
		ter->UpdateTerrain (cameraPos);
	}
	terrain_mutex.unlock ();

	// if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
	//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
	//}
	terrainUpdateTimer.EndTimer ();

	// chunkBuffer.UpdateChunks ();
}

void TerrainManager::RenderTerrain (VkCommandBuffer commandBuffer, bool wireframe)
{

	if (*terrainVulkanTextureArrayAlbedo->readyToUse && *terrainVulkanTextureArrayRoughness->readyToUse &&
	    *terrainVulkanTextureArrayMetallic->readyToUse && *terrainVulkanTextureArrayNormal->readyToUse)
	{
		std::lock_guard<std::mutex> lock (terrain_mutex);
		for (auto& ter : terrains)
		{
			ter->DrawTerrainGrid (commandBuffer, wireframe);
			// ter->DrawTerrain (commandBuffer, wireframe);
		}
	}
}

// TODO : Re-implement getting height at terrain location
float TerrainManager::GetTerrainHeightAtLocation (float x, float z)
{
	std::lock_guard<std::mutex> lock (terrain_mutex);
	for (auto& terrain : terrains)
	{
		glm::vec2 pos = terrain->coordinateData.pos;
		glm::vec2 size = terrain->coordinateData.size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z)
		{
			return terrain->GetHeightAtLocation (
			    (x - pos.x) / settings.width, (z - pos.y) / settings.width);
		}
	}
	return 0;
}

void TerrainManager::SaveSettingsToFile ()
{
	nlohmann::json j;

	j["show_window"] = settings.show_terrain_manager_window;
	j["terrain_width"] = settings.width;
	j["height_scale"] = settings.heightScale;
	j["max_levels"] = settings.maxLevels;
	j["grid_dimensions"] = settings.gridDimensions;
	j["view_distance"] = settings.viewDistance;
	j["source_image_resolution"] = settings.sourceImageResolution;
	j["worker_threads"] = settings.workerThreads;

	std::ofstream outFile (TerrainSettingsFileName);
	outFile << std::setw (4) << j;
	outFile.close ();
}

void TerrainManager::LoadSettingsFromFile ()
{
	if (fileExists (TerrainSettingsFileName))
	{
		std::ifstream input (TerrainSettingsFileName);

		nlohmann::json j;
		input >> j;

		settings.show_terrain_manager_window = j["show_window"];
		settings.width = j["terrain_width"];
		settings.heightScale = j["height_scale"];
		settings.maxLevels = j["max_levels"];
		settings.gridDimensions = j["grid_dimensions"];
		settings.viewDistance = j["view_distance"];
		settings.sourceImageResolution = j["source_image_resolution"];
		settings.workerThreads = j["worker_threads"];
		if (settings.workerThreads < 1) settings.workerThreads = 1;
	}
	else
	{

		settings = GeneralSettings{};
		SaveSettingsToFile ();
	}
}


void TerrainManager::UpdateTerrainGUI ()
{

	ImGui::SetNextWindowSize (ImVec2 (200, 220), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (220, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin ("Terrain Info", &settings.show_terrain_manager_window))
	{

		std::lock_guard<std::mutex> lk (terrain_mutex);
		ImGui::BeginGroup ();
		if (ImGui::Button ("Save"))
		{
			SaveSettingsToFile ();
		}
		ImGui::SameLine ();
		if (ImGui::Button ("Load"))
		{
			LoadSettingsFromFile ();
		}
		ImGui::EndGroup ();
		ImGui::SliderFloat ("Width", &nextTerrainWidth, 100, 10000);
		ImGui::SliderInt ("Max Subdivision", &settings.maxLevels, 0, 10);
		ImGui::SliderInt ("Grid Width", &settings.gridDimensions, 1, 10);
		ImGui::SliderFloat ("Height Scale", &settings.heightScale, 1, 1000);
		ImGui::SliderInt ("Image Resolution", &settings.sourceImageResolution, 32, 2048);
		ImGui::SliderInt ("View Distance", &settings.viewDistance, 1, 32);

		if (ImGui::Button ("Recreate Terrain", ImVec2 (130, 20)))
		{
			recreateTerrain = true;
		}
		ImGui::Text ("Terrain Count %lu", terrains.size ());
		ImGui::Text ("Generating %i Terrains", workContinueSignal->InQueue ());
		ImGui::Text ("All terrains update Time: %lu(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds ());

		{
			for (auto& ter : terrains)
			{
				ImGui::Text ("Terrain Draw Time: %lu(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds ());
				ImGui::Text ("Terrain Quad Count %d", ter->numQuads);
			}
		}
	}
	ImGui::End ();
}
