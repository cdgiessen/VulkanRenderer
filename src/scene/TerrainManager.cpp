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
	if (gui_settings.maxLevels < 0)
	{
		gui_settings.maxLevels = 0;
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

	terrainGridModel = std::make_unique<VulkanModel> (
	    renderer, createFlatPlane (gui_settings.numCells, cml::vec3f (1.0f)));

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


void TerrainManager::UpdateTerrains (cml::vec3f cameraPos)
{
	curCameraPos = cameraPos;
	t_settings = gui_settings;

	if (recreateTerrain)
	{
		CleanUpTerrain ();
		workContinueSignal = std::make_shared<job::TaskSignal> ();
		// GenerateTerrain(resourceMan, renderer, camera);
		recreateTerrain = false;
	}

	terrainUpdateTimer.StartTimer ();

	std::vector<std::vector<std::unique_ptr<Terrain>>::iterator> terToDelete;

	{
		std::lock_guard<std::mutex> lk (terrain_mutex);
		// delete terrains too far away
		for (auto it = std::begin (terrains); it != std::end (terrains); it++)
		{
			cml::vec3f center =
			    cml::vec3f ((*it)->coordinateData.pos.x, cameraPos.y, (*it)->coordinateData.pos.y);
			float distanceToViewer = cml::distance (cameraPos, center);
			if (distanceToViewer > t_settings.viewDistance * t_settings.width * 1.5)
			{
				// if ((*(*it)->terrainSplatMap->readyToUse) == true)
				//{

				terToDelete.push_back (it);
				// Log::Debug << "deleting terrain at x:" << (*it)->coordinateData.noisePos.x / (*it)->coordinateData.sourceImageResolution
				//	<< " z: " << (*it)->coordinateData.noisePos.y / (*it)->coordinateData.sourceImageResolution << "\n";
				auto activeIt = std::find (
				    std::begin (activeTerrains), std::end (activeTerrains), (*it)->coordinateData.gridPos);
				if (activeIt != std::end (activeTerrains)) activeTerrains.erase (activeIt);
				//}
			}
		}
		while (terToDelete.size () > 0)
		{
			terrains.erase (terToDelete.back ());
			terToDelete.pop_back ();
		}
	}

	// make new closer terrains

	cml::vec2i camGrid ((int)((cameraPos.x + 0 * t_settings.width / 2.0) / t_settings.width),
	    (int)((cameraPos.z + 0 * t_settings.width / 2.0) / t_settings.width));


	// Log::Debug << "cam grid x: " << camGridX << " z: " << camGridZ << "\n";
	for (int i = 0; i < t_settings.viewDistance * 2; i++)
	{
		for (int j = 0; j < t_settings.viewDistance * 2; j++)
		{

			cml::vec2i terGrid (camGrid.x + i - t_settings.viewDistance, camGrid.y + j - t_settings.viewDistance);

			cml::vec3f center =
			    cml::vec3f (terGrid.x * t_settings.width, cameraPos.y, terGrid.y * t_settings.width);
			// float distanceToViewer = cml::distance (cameraPos, center);

			// if (distanceToViewer <= t_settings.viewDistance * t_settings.width) {

			// Log::Debug << "noisePosX " << ter->coordinateData.noisePos.x/ter->coordinateData.sourceImageResolution << "\n";
			// Log::Debug << "relX " << camGridX + i - t_settings.viewDistance / 2.0 << "\n";

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

				auto pos = cml::vec2f ((terGrid.x) * t_settings.width - t_settings.width / 2,
				    (terGrid.y) * t_settings.width - t_settings.width / 2);

				TerrainCoordinateData coord = TerrainCoordinateData (pos, // position
				    cml::vec2f (t_settings.width, t_settings.width),      // size
				    cml::vec2i ((terGrid.x) * t_settings.sourceImageResolution,
				        (terGrid.y) * t_settings.sourceImageResolution), // noise position
				    cml::vec2f (1.0 / (float)t_settings.sourceImageResolution,
				        1.0f / (float)t_settings.sourceImageResolution), // noiseSize
				    t_settings.sourceImageResolution + 1,
				    terGrid);

				auto t = job::Task (workContinueSignal, [this, coord] {
					auto terCreateData = TerrainCreationData (t_settings.numCells,
					    t_settings.maxLevels,
					    t_settings.sourceImageResolution,
					    t_settings.heightScale,
					    coord);

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

	//	if (*terrainVulkanTextureArrayAlbedo->readyToUse && *terrainVulkanTextureArrayRoughness->readyToUse &&
	//	    *terrainVulkanTextureArrayMetallic->readyToUse && *terrainVulkanTextureArrayNormal->readyToUse)
	//	{
	std::lock_guard<std::mutex> lock (terrain_mutex);
	for (auto& ter : terrains)
	{
		ter->DrawTerrainGrid (commandBuffer, wireframe);
		// ter->DrawTerrain (commandBuffer, wireframe);
	}
	//	}
}

// TODO : Re-implement getting height at terrain location
float TerrainManager::GetTerrainHeightAtLocation (float x, float z)
{
	std::lock_guard<std::mutex> lock (terrain_mutex);
	for (auto& terrain : terrains)
	{
		cml::vec2f pos = terrain->coordinateData.pos;
		cml::vec2f size = terrain->coordinateData.size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z)
		{
			return terrain->GetHeightAtLocation (
			    (x - pos.x) / t_settings.width, (z - pos.y) / t_settings.width);
		}
	}
	return 0;
}

void TerrainManager::SaveSettingsToFile ()
{
	nlohmann::json j;

	j["show_window"] = gui_settings.show_terrain_manager_window;
	j["terrain_width"] = gui_settings.width;
	j["height_scale"] = gui_settings.heightScale;
	j["max_levels"] = gui_settings.maxLevels;
	j["grid_dimensions"] = gui_settings.gridDimensions;
	j["view_distance"] = gui_settings.viewDistance;
	j["source_image_resolution"] = gui_settings.sourceImageResolution;

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

		gui_settings.show_terrain_manager_window = j["show_window"];
		gui_settings.width = j["terrain_width"];
		gui_settings.heightScale = j["height_scale"];
		gui_settings.maxLevels = j["max_levels"];
		gui_settings.gridDimensions = j["grid_dimensions"];
		gui_settings.viewDistance = j["view_distance"];
		gui_settings.sourceImageResolution = j["source_image_resolution"];
	}
	else
	{

		gui_settings = GeneralSettings{};
		SaveSettingsToFile ();
	}
}


void TerrainManager::UpdateTerrainGUI ()
{

	ImGui::SetNextWindowSize (ImVec2 (200, 220), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (220, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin ("Terrain Info", &gui_settings.show_terrain_manager_window))
	{
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
		ImGui::SliderInt ("Max Subdivision", &gui_settings.maxLevels, 0, 10);
		ImGui::SliderInt ("Grid Width", &gui_settings.gridDimensions, 1, 10);
		ImGui::SliderFloat ("Height Scale", &gui_settings.heightScale, 1, 1000);
		ImGui::SliderInt ("Image Resolution", &gui_settings.sourceImageResolution, 32, 2048);
		ImGui::SliderInt ("View Distance", &gui_settings.viewDistance, 1, 32);

		if (ImGui::Button ("Recreate Terrain", ImVec2 (130, 20)))
		{
			recreateTerrain = true;
		}
		{
			std::lock_guard<std::mutex> lk (terrain_mutex);
			ImGui::Text ("Terrain Count %lu", terrains.size ());
			ImGui::Text ("Generating %i Terrains", workContinueSignal->InQueue ());
			ImGui::Text ("All terrains update Time: %lu(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds ());

			for (auto& ter : terrains)
			{
				ImGui::Text ("Terrain Draw Time: %lu(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds ());
				ImGui::Text ("Terrain Quad Count %d", ter->numQuads);
			}
		}
	}
	ImGui::End ();
}
