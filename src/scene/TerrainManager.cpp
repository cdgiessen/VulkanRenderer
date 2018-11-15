#include "TerrainManager.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>

#include <nlohmann/json.hpp>

#include <ImGui/imgui.h>

#include "core/Logger.h"


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

TerrainChunkBuffer::TerrainChunkBuffer (VulkanRenderer& renderer, int count, TerrainManager& man)
: renderer (renderer),
  man (man),
  vert_buffer (renderer.device, vertCount * count, vertElementCount),
  index_buffer (renderer.device, (uint32_t)indCount * (uint32_t)count),
  vert_staging (renderer.device, sizeof (TerrainMeshVertices) * count),
  index_staging (renderer.device, sizeof (TerrainMeshIndices) * count)
{
	// vert_buffer.CreateVertexBuffer(vertCount * count, vertElementCount);
	// index_buffer.CreateIndexBuffer(indCount * count);

	// vert_staging.CreateDataBuffer(sizeof(TerrainMeshVertices) * count);
	vert_staging_ptr = (TerrainMeshVertices*)vert_staging.buffer.allocationInfo.pMappedData;

	// index_staging.CreateDataBuffer(sizeof(TerrainMeshIndices) * count);
	index_staging_ptr = (TerrainMeshIndices*)index_staging.buffer.allocationInfo.pMappedData;

	chunkStates.resize (count, TerrainChunkBuffer::ChunkState::free);
	for (int i = 0; i < count; i++)
	{
		chunkReadySignals.push_back (std::make_shared<bool> (false));
	}
}


TerrainChunkBuffer::~TerrainChunkBuffer ()
{
	// if (chunkCount <= 0) Log::Error << "Not all terrain chunks were freed!\n";
}


int TerrainChunkBuffer::Allocate ()
{
	std::lock_guard<std::mutex> guard (lock);
	for (int i = 0; i < chunkStates.size (); i++)
	{
		if (chunkStates.at (i) == TerrainChunkBuffer::ChunkState::free)
		{
			chunkStates.at (i) = TerrainChunkBuffer::ChunkState::allocated;
			chunkCount++;
			return i;
		}
	}
	// should never reach here!
	throw std::runtime_error ("Ran out of terrain chunkStates!");
}


void TerrainChunkBuffer::Free (int index)
{
	std::lock_guard<std::mutex> guard (lock);
	if (chunkStates.at (index) == TerrainChunkBuffer::ChunkState::free)
		throw std::runtime_error ("Trying to free a free chunk! What?");
	chunkStates.at (index) = TerrainChunkBuffer::ChunkState::free;
	chunkCount--;
}

TerrainChunkBuffer::ChunkState TerrainChunkBuffer::GetChunkState (int index)
{
	std::lock_guard<std::mutex> guard (lock);
	return chunkStates.at (index);
}

void TerrainChunkBuffer::SetChunkWritten (int index)
{
	std::lock_guard<std::mutex> guard (lock);
	chunkStates.at (index) = TerrainChunkBuffer::ChunkState::written;
}

Signal TerrainChunkBuffer::GetChunkSignal (int index)
{
	std::lock_guard<std::mutex> guard (lock);
	return chunkReadySignals.at (index);
}

int TerrainChunkBuffer::ActiveQuadCount () { return chunkCount; }

void TerrainChunkBuffer::UpdateChunks ()
{
	std::lock_guard<std::mutex> guard (lock);

	std::vector<VkBufferCopy> vertexCopyRegions;
	std::vector<VkBufferCopy> indexCopyRegions;

	std::vector<Signal> signals;

	for (int i = 0; i < chunkStates.size (); i++)
	{
		switch (chunkStates.at (i))
		{
			case (TerrainChunkBuffer::ChunkState::free):
				break;

			case (TerrainChunkBuffer::ChunkState::allocated):
				break;

				// needs to have its data uploaded
			case (TerrainChunkBuffer::ChunkState::written):

				vertexCopyRegions.push_back (
				    initializers::bufferCopyCreate (vert_size, i * vert_size, i * vert_size));
				indexCopyRegions.push_back (
				    initializers::bufferCopyCreate (ind_size, i * ind_size, i * ind_size));

				//*chunkReadySignals.at(i) = false;

				// signals.push_back(chunkReadySignals.at(i));
				chunkStates.at (i) = TerrainChunkBuffer::ChunkState::ready;
				break;

				// data is on gpu, ready to draw
			case (TerrainChunkBuffer::ChunkState::ready):
				break;
		}
	}
	VkBuffer vert = vert_buffer.buffer.buffer;
	VkBuffer vert_s = vert_staging.buffer.buffer;
	VkBuffer index = index_buffer.buffer.buffer;
	VkBuffer index_s = index_staging.buffer.buffer;

	if (vertexCopyRegions.size () > 0)
	{
		renderer.SubmitWork (WorkType::transfer,
		    [=](const VkCommandBuffer cmdBuf) {
			    std::vector<VkBufferCopy> vRegions = vertexCopyRegions;
			    std::vector<VkBufferCopy> iRegions = indexCopyRegions;

			    vkCmdCopyBuffer (cmdBuf, vert_s, vert, vRegions.size (), vRegions.data ());
			    vkCmdCopyBuffer (cmdBuf, index_s, index, iRegions.size (), iRegions.data ());
		    },
		    {},
		    {},
		    {},
		    std::move (signals));
	}
}

TerrainMeshVertices* TerrainChunkBuffer::GetDeviceVertexBufferPtr (int index)
{
	return vert_staging_ptr + index;
}
TerrainMeshIndices* TerrainChunkBuffer::GetDeviceIndexBufferPtr (int index)
{
	return index_staging_ptr + index;
}

TerrainManager::TerrainManager (
    InternalGraph::GraphPrototype& protoGraph, Resource::AssetManager& resourceMan, VulkanRenderer& renderer)
: protoGraph (protoGraph),
  renderer (renderer),
  resourceMan (resourceMan),
  chunkBuffer (renderer, MaxChunkCount, *this)
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

	instancedWaters = std::make_unique<InstancedSceneObject> (renderer);
	instancedWaters->SetFragmentShaderToUse ("assets/shaders/water.frag.spv");
	instancedWaters->SetBlendMode (VK_TRUE);
	instancedWaters->SetCullMode (VK_CULL_MODE_NONE);
	instancedWaters->LoadModel (createFlatPlane (settings.numCells, glm::vec3 (1, 0, 1)));
	instancedWaters->LoadTexture (resourceMan.texManager.GetTexIDByName ("TileableWaterTexture"));

	instancedWaters->InitInstancedSceneObject ();

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
	// instancedWaters->RemoveAllInstances();
	// instancedWaters->CleanUp();
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

				InstancedSceneObject::InstanceData water;
				water.pos = glm::vec3 ((*it)->coordinateData.pos.x, 0, (*it)->coordinateData.pos.y);
				water.rot = glm::vec3 (0, 0, 0);
				water.scale = settings.width;
				instancedWaters->RemoveInstance (water);
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
						    chunkBuffer,
						    protoGraph,
						    terCreateData.numCells,
						    terCreateData.maxLevels,
						    terCreateData.heightScale,
						    terCreateData.coord);

						terrain->InitTerrain (curCameraPos,
						    terrainVulkanTextureArrayAlbedo,
						    terrainVulkanTextureArrayRoughness,
						    terrainVulkanTextureArrayMetallic,
						    terrainVulkanTextureArrayNormal);

						InstancedSceneObject::InstanceData water;
						water.pos = glm::vec3 (terCreateData.coord.pos.x, 0, terCreateData.coord.pos.y);
						water.rot = glm::vec3 (0, 0, 0);
						water.scale = settings.width;
						instancedWaters->AddInstance (water);

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

	instancedWaters->UploadData ();

	chunkBuffer.UpdateChunks ();
}

void TerrainManager::RenderDepthPrePass (VkCommandBuffer commandBuffer)
{
	{
		std::lock_guard<std::mutex> lock (terrain_mutex);
		for (auto& ter : terrains)
		{
			ter->DrawDepthPrePass (commandBuffer);
		}
	}
}

void TerrainManager::RenderTerrain (VkCommandBuffer commandBuffer, bool wireframe)
{
	if (*terrainVulkanTextureArrayAlbedo->readyToUse && *terrainVulkanTextureArrayRoughness->readyToUse &&
	    *terrainVulkanTextureArrayMetallic->readyToUse && *terrainVulkanTextureArrayNormal->readyToUse)
	{
		std::lock_guard<std::mutex> lock (terrain_mutex);
		for (auto& ter : terrains)
		{
			ter->DrawTerrain (commandBuffer, wireframe);
		}
	}

	instancedWaters->WriteToCommandBuffer (commandBuffer, wireframe);
}

// TODO : Reimplement getting height at terrain location
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
	j["grid_dimentions"] = settings.gridDimentions;
	j["view_distance"] = settings.viewDistance;
	j["souce_iamge_resolution"] = settings.sourceImageResolution;
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
		settings.gridDimentions = j["grid_dimentions"];
		settings.viewDistance = j["view_distance"];
		settings.sourceImageResolution = j["souce_iamge_resolution"];
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
		ImGui::SliderInt ("Grid Width", &settings.gridDimentions, 1, 10);
		ImGui::SliderFloat ("Height Scale", &settings.heightScale, 1, 1000);
		ImGui::SliderInt ("Image Resolution", &settings.sourceImageResolution, 32, 2048);
		ImGui::SliderInt ("View Distance", &settings.viewDistance, 1, 32);

		if (ImGui::Button ("Recreate Terrain", ImVec2 (130, 20)))
		{
			recreateTerrain = true;
		}
		ImGui::Text ("Terrain Count %lu", terrains.size ());
		ImGui::Text ("Generating %i Terrains", workContinueSignal->InQueue ());
		ImGui::Text ("Quad Count %i", chunkBuffer.ActiveQuadCount ());
		ImGui::Text ("All terrains update Time: %lu(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds ());

		{
			std::lock_guard<std::mutex> lk (terrain_mutex);
			for (auto& ter : terrains)
			{
				ImGui::Text ("Terrain Draw Time: %lu(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds ());
				ImGui::Text ("Terrain Quad Count %d", ter->numQuads);
			}
		}
	}
	ImGui::End ();
}

void TerrainManager::DrawTerrainTextureViewer ()
{

	ImGui::SetNextWindowSize (ImVec2 (300, 200), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (0, 475), ImGuiSetCond_FirstUseEver);


	if (ImGui::Begin ("Textures", &drawWindow, ImGuiWindowFlags_MenuBar))
	{

		if (ImGui::BeginMenuBar ())
		{
			if (ImGui::BeginMenu ("File"))
			{
				if (ImGui::MenuItem ("Open Texture"))
				{
				}
				if (ImGui::MenuItem ("Close")) drawWindow = false;
				ImGui::EndMenu ();
			}
			ImGui::EndMenuBar ();
		}

		// left
		ImGui::BeginChild ("left pane", ImVec2 (150, 0), true);
		for (int i = 0; i < terrainTextureHandles.size (); i++)
		{
			// char label[128];
			// sprintf(label, "%s", terrainTextureHandles.at(i).name.c_str());
			if (ImGui::Selectable (terrainTextureHandles.at (i).name.c_str (), selectedTexture == i))
				selectedTexture = i;
		}

		ImGui::EndChild ();

		ImGui::SameLine ();

		ImGui::BeginGroup ();
		ImGui::BeginChild ("item view", ImVec2 (0, -ImGui::GetItemsLineHeightWithSpacing ())); // Leave room for 1 line below us
		ImGui::Text ("MyObject: %d", selectedTexture);
		ImGui::Separator ();


		ImGui::TextWrapped ("TODO: Add texture preview");

		ImGui::EndChild ();

		ImGui::BeginChild ("buttons");
		if (ImGui::Button ("PlaceHolder"))
		{
		}
		ImGui::EndChild ();

		ImGui::EndGroup ();
	}
	ImGui::End ();
}
