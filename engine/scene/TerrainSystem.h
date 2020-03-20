//#pragma once
//
//#include <atomic>
//#include <memory>
//#include <mutex>
//#include <queue>
//#include <thread>
//#include <unordered_map>
//#include <vector>
//
//#include "util/SimpleTimer.h"
//
//#include "core/JobSystem.h"
//#include "core/Time.h"
//
//#include "util/ConcurrentQueue.h"
//
//#include "rendering/Renderer.h"
//
//#include "resources/Resource.h"
//
//#include "gui/InternalGraph.h"
//
//#include "Camera.h"
//#include "Terrain.h"
//
//
// struct GeneralSettings
//{
//	bool show_terrain_system_window = true;
//	float width = 1000;
//	float heightScale = 100.0f;
//	int maxLevels = 4;
//	int gridDimensions = 1;
//	int viewDistance = 3; // terrain chunks to load away from camera;
//	int sourceImageResolution = 256;
//	int numCells = 64; // compile time currently
//};
//
// struct TerrainCreationData
//{
//	int numCells;
//	int maxLevels;
//	int sourceImageResolution;
//	float heightScale;
//	TerrainCoordinateData coord;
//
//	TerrainCreationData (
//	    int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord);
//};
//
// class TerrainSystem
//{
//	public:
//	TerrainSystem (job::ThreadPool& thread_pool,
//	    InternalGraph::GraphPrototype& protoGraph,
//	    Resource::Resources& resourceMan,
//	    VulkanRenderer& renderer);
//	~TerrainSystem ();
//
//	void CleanUpTerrain ();
//
//	void UpdateTerrains (cml::vec3f cameraPos);
//
//	void RenderTerrain (VkCommandBuffer commandBuffer, bool wireframe);
//
//	void UpdateTerrainGUI ();
//
//	float GetTerrainHeightAtLocation (float x, float z);
//
//	job::ThreadPool& thread_pool;
//	Resource::Resources& resourceMan;
//	VulkanRenderer& renderer;
//
//	std::shared_ptr<job::TaskSignal> workContinueSignal;
//
//	bool isCreatingTerrain = true; // while condition for worker threads
//
//	std::mutex terrain_mutex;
//	std::vector<std::unique_ptr<Terrain>> in_progress_terrains;
//	std::vector<std::unique_ptr<Terrain>> terrains;
//	std::vector<cml::vec2i> activeTerrains;
//	cml::vec3f curCameraPos;
//	InternalGraph::GraphPrototype& protoGraph;
//	VulkanTextureID terrainVulkanTextureArrayAlbedo;
//	VulkanTextureID terrainVulkanTextureArrayRoughness;
//	VulkanTextureID terrainVulkanTextureArrayMetallic;
//	VulkanTextureID terrainVulkanTextureArrayNormal;
//
//	GeneralSettings gui_settings; // for gui to edit (main)
//	GeneralSettings t_settings;   // for terrains to use (copy)
//
//	private:
//	void SaveSettingsToFile ();
//	void LoadSettingsFromFile ();
//
//	void StopActiveJobs ();
//
//	std::unique_ptr<VulkanModel> terrainGridModel;
//
//	Resource::Texture::TexID terrainTextureArrayAlbedo;
//	Resource::Texture::TexID terrainTextureArrayRoughness;
//	Resource::Texture::TexID terrainTextureArrayMetallic;
//	Resource::Texture::TexID terrainTextureArrayNormal;
//
//	bool recreateTerrain = true;
//	float nextTerrainWidth = 1000;
//	SimpleTimer terrainUpdateTimer;
//
//	std::vector<std::string> terrainTextureFileNames = {
//		"dirt.jpg",
//		"grass.jpg",
//		"rockSmall.jpg",
//		"Snow.jpg",
//	};
//};
