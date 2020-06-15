//#include "TerrainSystem.h"
//
//#include <algorithm>
//#include <chrono>
//#include <filesystem>
//#include <functional>
//#include <iomanip>
//
//#include <nlohmann/json.hpp>
//
//#include <imgui/imgui.hpp>
//
//#include "core/Logger.h"
//#include "rendering/Initializers.h"
//
//
// const auto TerrainSettingsFileName = "terrain_settings.json";
//
// TerrainCreationData::TerrainCreationData (
//    int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord)
//: numCells (numCells),
//  maxLevels (maxLevels),
//  sourceImageResolution (sourceImageResolution),
//  heightScale (heightScale),
//  coord (coord)
//{
//}
//
//
// TerrainSystem::TerrainSystem (job::ThreadPool& thread_pool,
//    InternalGraph::GraphPrototype& protoGraph,
//    Resource::Resources& resourceMan,
//    VulkanRenderer& renderer)
//: thread_pool (thread_pool), resourceMan (resourceMan), renderer (renderer), protoGraph (protoGraph)
//{
//	if (gui_settings.maxLevels < 0)
//	{
//		gui_settings.maxLevels = 0;
//	}
//	LoadSettingsFromFile ();
//
//	TexCreateDetails details (VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 8);
//
//	terrainTextureArrayAlbedo = resourceMan.textures.get_tex_id_by_name ("terrain_albedo");
//	terrainVulkanTextureArrayAlbedo =
//	    renderer.textures.create_texture_2d_array(terrainTextureArrayAlbedo, details);
//
//	terrainTextureArrayRoughness = resourceMan.textures.get_tex_id_by_name ("terrain_roughness");
//	terrainVulkanTextureArrayRoughness =
//	    renderer.textures.create_texture_2d_array(terrainTextureArrayRoughness, details);
//
//	terrainTextureArrayMetallic = resourceMan.textures.get_tex_id_by_name ("terrain_metalness");
//	terrainVulkanTextureArrayMetallic =
//	    renderer.textures.create_texture_2d_array(terrainTextureArrayMetallic, details);
//
//	terrainTextureArrayNormal = resourceMan.textures.get_tex_id_by_name ("terrain_normal");
//	terrainVulkanTextureArrayNormal =
//	    renderer.textures.create_texture_2d_array(terrainTextureArrayNormal, details);
//
//	terrainGridModel = std::make_unique<VulkanModel> (renderer.device,
//	    renderer.async_task_queue,
//	    createFlatPlane (gui_settings.numCells, cml::vec3f (1.0f)));
//
//	// StartWorkerThreads ();
//	workContinueSignal = std::make_shared<job::TaskSignal> ();
//}
//
// TerrainSystem::~TerrainSystem () { CleanUpTerrain (); }
//
//
// void TerrainSystem::StopActiveJobs ()
//{
//	workContinueSignal->cancel ();
//	workContinueSignal->wait ();
//}
//
//
// void TerrainSystem::CleanUpTerrain ()
//{
//	StopActiveJobs ();
//	terrains.clear ();
//	activeTerrains.clear ();
//}
//
//
// void TerrainSystem::UpdateTerrains (cml::vec3f camera_pos)
//{
//	curCameraPos = camera_pos;
//	t_settings = gui_settings;
//
//	if (recreateTerrain)
//	{
//		CleanUpTerrain ();
//		workContinueSignal = std::make_shared<job::TaskSignal> ();
//		// GenerateTerrain(resourceMan, renderer, camera);
//		recreateTerrain = false;
//	}
//
//	terrainUpdateTimer.start_timer ();
//
//	std::vector<std::vector<std::unique_ptr<Terrain>>::iterator> terToDelete;
//
//	{
//		std::lock_guard<std::mutex> lk (terrain_mutex);
//		// delete terrains too far away
//		for (auto it = std::begin (terrains); it != std::end (terrains); it++)
//		{
//			cml::vec3f center =
//			    cml::vec3f ((*it)->coordinateData.pos.x, camera_pos.y, (*it)->coordinateData.pos.y);
//			float distanceToViewer = cml::distance (camera_pos, center);
//			if (distanceToViewer > t_settings.viewDistance * t_settings.width * 1.5)
//			{
//				terToDelete.push_back (it);
//				// Log.Debug << "deleting terrain at x:" << (*it)->coordinateData.noisePos.x / (*it)->coordinateData.sourceImageResolution
//				//	<< " z: " << (*it)->coordinateData.noisePos.y / (*it)->coordinateData.sourceImageResolution << "\n";
//				auto activeIt = std::find (
//				    std::begin (activeTerrains), std::end (activeTerrains), (*it)->coordinateData.gridPos);
//				if (activeIt != std::end (activeTerrains))
//				{
//					activeTerrains.erase (activeIt);
//				}
//				//}
//			}
//		}
//		while (terToDelete.size () > 0)
//		{
//			terrains.erase (terToDelete.back ());
//			terToDelete.pop_back ();
//		}
//	}
//
//	// make new closer terrains
//
//	cml::vec2i camGrid ((int)((camera_pos.x + 0 * t_settings.width / 2.0) / t_settings.width),
//	    (int)((camera_pos.z + 0 * t_settings.width / 2.0) / t_settings.width));
//
//
//	// Log.Debug << "cam grid x: " << camGridX << " z: " << camGridZ << "\n";
//	for (int i = 0; i < t_settings.viewDistance * 2; i++)
//	{
//		for (int j = 0; j < t_settings.viewDistance * 2; j++)
//		{
//
//			cml::vec2i terGrid (camGrid.x + i - t_settings.viewDistance, camGrid.y + j - t_settings.viewDistance);
//
//			cml::vec3f center =
//			    cml::vec3f (terGrid.x * t_settings.width, camera_pos.y, terGrid.y * t_settings.width);
//			// float distanceToViewer = cml::distance (camera_pos, center);
//
//			// if (distanceToViewer <= t_settings.viewDistance * t_settings.width) {
//
//			// Log.Debug << "noisePosX " << ter->coordinateData.noisePos.x/ter->coordinateData.sourceImageResolution << "\n";
//			// Log.Debug << "relX " << camGridX + i - t_settings.viewDistance / 2.0 << "\n";
//
//			// see if there are any terrains already there
//			bool found = false;
//			for (auto& ter : activeTerrains)
//			{
//				if (ter.x == terGrid.x && ter.y == terGrid.y)
//				{
//					found = true;
//				}
//			}
//			if (!found)
//			{
//				// Log.Debug << "creating new terrain at x:" << terGrid.x << " z: " << terGrid.y << "\n";
//
//				activeTerrains.push_back (terGrid);
//
//				auto pos = cml::vec2f ((terGrid.x) * t_settings.width - t_settings.width / 2,
//				    (terGrid.y) * t_settings.width - t_settings.width / 2);
//
//				TerrainCoordinateData coord = TerrainCoordinateData (pos, // position
//				    cml::vec2f (t_settings.width, t_settings.width),      // size
//				    cml::vec2i ((terGrid.x) * t_settings.sourceImageResolution,
//				        (terGrid.y) * t_settings.sourceImageResolution), // noise position
//				    cml::vec2f (1.0 / (float)t_settings.sourceImageResolution,
//				        1.0f / (float)t_settings.sourceImageResolution), // noiseSize
//				    t_settings.sourceImageResolution + 1,
//				    terGrid);
//
//				auto t = [this, coord] {
//					auto terCreateData = TerrainCreationData (t_settings.numCells,
//					    t_settings.maxLevels,
//					    t_settings.sourceImageResolution,
//					    t_settings.heightScale,
//					    coord);
//
//					auto terrain = std::make_unique<Terrain> (renderer,
//
//					    protoGraph,
//					    terCreateData.numCells,
//					    terCreateData.maxLevels,
//					    terCreateData.heightScale,
//					    terCreateData.coord,
//					    terrainGridModel.get ());
//
//					terrain->InitTerrain (curCameraPos,
//					    terrainVulkanTextureArrayAlbedo,
//					    terrainVulkanTextureArrayRoughness,
//					    terrainVulkanTextureArrayMetallic,
//					    terrainVulkanTextureArrayNormal);
//
//					std::lock_guard<std::mutex> lk (terrain_mutex);
//					in_progress_terrains.push_back (std::move (terrain));
//				};
//
//				thread_pool.submit (std::move (t), workContinueSignal);
//			}
//		}
//	}
//
//	terrain_mutex.lock ();
//
//	for (auto it = in_progress_terrains.begin (); it != in_progress_terrains.end ();)
//	{
//		if ((*it)->IsReady ())
//		{
//			terrains.push_back (std::unique_ptr<Terrain> ());
//			std::swap (*it, terrains.back ());
//			in_progress_terrains.erase (it);
//		}
//		else
//		{
//			++it;
//		}
//	}
//	terrain_mutex.unlock ();
//
//
//	// update all terrains
//	terrain_mutex.lock ();
//	for (auto& ter : terrains)
//	{
//		ter->UpdateTerrain (camera_pos);
//	}
//	terrain_mutex.unlock ();
//
//	// if (terrainUpdateTimer.get_elapsed_time_micro_seconds() > 1000) {
//	//	Log.Debug << terrainUpdateTimer.get_elapsed_time_micro_seconds() << "\n";
//	//}
//	terrainUpdateTimer.end_timer ();
//
//	// chunkBuffer.UpdateChunks ();
//}
//
// void TerrainSystem::RenderTerrain (VkCommandBuffer commandBuffer, bool wireframe)
//{
//	std::lock_guard lock (terrain_mutex);
//	for (auto& ter : terrains)
//	{
//		ter->DrawTerrainGrid (commandBuffer, wireframe);
//		// ter->DrawTerrain (commandBuffer, wireframe);
//	}
//}
//
//// TODO : Re-implement getting height at terrain location
// float TerrainSystem::GetTerrainHeightAtLocation (float x, float z)
//{
//	std::lock_guard lock (terrain_mutex);
//	for (auto& terrain : terrains)
//	{
//		cml::vec2f pos = terrain->coordinateData.pos;
//		cml::vec2f size = terrain->coordinateData.size;
//		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z)
//		{
//			return terrain->get_heightAtLocation (
//			    (x - pos.x) / t_settings.width, (z - pos.y) / t_settings.width);
//		}
//	}
//	return 0;
//}
//
// void TerrainSystem::SaveSettingsToFile ()
//{
//	nlohmann::json j;
//
//	j["show_window"] = gui_settings.show_terrain_system_window;
//	j["terrain_width"] = gui_settings.width;
//	j["height_scale"] = gui_settings.heightScale;
//	j["max_levels"] = gui_settings.maxLevels;
//	j["grid_dimensions"] = gui_settings.gridDimensions;
//	j["view_distance"] = gui_settings.viewDistance;
//	j["source_image_resolution"] = gui_settings.sourceImageResolution;
//
//	std::ofstream outFile (TerrainSettingsFileName);
//	outFile << std::setw (4) << j;
//	outFile.close ();
//}
//
// void TerrainSystem::LoadSettingsFromFile ()
//{
//	if (std::filesystem::exists (std::filesystem::path (TerrainSettingsFileName)))
//	{
//		std::ifstream input (TerrainSettingsFileName);
//
//		nlohmann::json j;
//		input >> j;
//
//		gui_settings.show_terrain_system_window = j["show_window"];
//		gui_settings.width = j["terrain_width"];
//		gui_settings.heightScale = j["height_scale"];
//		gui_settings.maxLevels = j["max_levels"];
//		gui_settings.gridDimensions = j["grid_dimensions"];
//		gui_settings.viewDistance = j["view_distance"];
//		gui_settings.sourceImageResolution = j["source_image_resolution"];
//	}
//	else
//	{
//
//		gui_settings = GeneralSettings{};
//		SaveSettingsToFile ();
//	}
//}
//
//
// void TerrainSystem::UpdateTerrainGUI ()
//{
//
//	ImGui::SetNextWindowSize (ImVec2 (200, 220), ImGuiCond_FirstUseEver);
//	ImGui::SetNextWindowPos (ImVec2 (220, 0), ImGuiCond_FirstUseEver);
//	if (ImGui::Begin ("Terrain Info", &gui_settings.show_terrain_system_window))
//	{
//		ImGui::BeginGroup ();
//		if (ImGui::Button ("Save"))
//		{
//			SaveSettingsToFile ();
//		}
//		ImGui::SameLine ();
//		if (ImGui::Button ("Load"))
//		{
//			LoadSettingsFromFile ();
//		}
//		ImGui::EndGroup ();
//		ImGui::SliderFloat ("Width", &nextTerrainWidth, 100, 10000);
//		ImGui::SliderInt ("Max Subdivision", &gui_settings.maxLevels, 0, 10);
//		ImGui::SliderInt ("Grid Width", &gui_settings.gridDimensions, 1, 10);
//		ImGui::SliderFloat ("Height Scale", &gui_settings.heightScale, 1, 1000);
//		ImGui::SliderInt ("Image Resolution", &gui_settings.sourceImageResolution, 32, 2048);
//		ImGui::SliderInt ("View Distance", &gui_settings.viewDistance, 1, 32);
//
//		if (ImGui::Button ("Recreate Terrain", ImVec2 (130, 20)))
//		{
//			recreateTerrain = true;
//		}
//		{
//			std::lock_guard<std::mutex> lk (terrain_mutex);
//			ImGui::Text ("Terrain Count %lu", terrains.size ());
//			ImGui::Text ("Generating %i Terrains", workContinueSignal->in_queue ());
//			ImGui::Text ("All terrains update Time: %lu(uS)", terrainUpdateTimer.get_elapsed_time_micro_seconds ());
//
//			for (auto& ter : terrains)
//			{
//				ImGui::Text ("Terrain Draw Time: %lu(uS)", ter->drawTimer.get_elapsed_time_micro_seconds ());
//				ImGui::Text ("Terrain Quad Count %d", ter->numQuads);
//			}
//		}
//	}
//	ImGui::End ();
//}
