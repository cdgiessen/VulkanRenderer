#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/CoreTools.h"
#include "core/JobSystem.h"
#include "core/TimeManager.h"

#include "util/ConcurrentQueue.h"

#include "rendering/Renderer.h"

#include "resources/ResourceManager.h"

#include "gui/InternalGraph.h"

#include "Camera.h"
#include "InstancedSceneObject.h"
#include "Terrain.h"


struct GeneralSettings
{
	bool show_terrain_manager_window = true;
	float width = 1000;
	float heightScale = 100.0f;
	int maxLevels = 4;
	int gridDimensions = 1;
	int viewDistance = 1; // terrain chunks to load away from camera;
	int sourceImageResolution = 256;
	int numCells = 64; // compile time currently
};

struct TerrainCreationData
{
	int numCells;
	int maxLevels;
	int sourceImageResolution;
	float heightScale;
	TerrainCoordinateData coord;

	TerrainCreationData (
	    int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord);
};

class TerrainManager
{
	public:
	TerrainManager (InternalGraph::GraphPrototype& protoGraph,
	    Resource::AssetManager& resourceMan,
	    VulkanRenderer& renderer);
	~TerrainManager ();

	void CleanUpTerrain ();

	void UpdateTerrains (glm::vec3 cameraPos);

	void RenderTerrain (VkCommandBuffer commandBuffer, bool wireframe);

	void UpdateTerrainGUI ();

	float GetTerrainHeightAtLocation (float x, float z);

	Resource::AssetManager& resourceMan;
	VulkanRenderer& renderer;

	std::shared_ptr<job::TaskSignal> workContinueSignal;

	bool isCreatingTerrain = true; // while condition for worker threads

	std::mutex terrain_mutex;
	std::vector<std::unique_ptr<Terrain>> terrains;
	std::vector<glm::i32vec2> activeTerrains;
	glm::vec3 curCameraPos;
	InternalGraph::GraphPrototype& protoGraph;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal;

	GeneralSettings gui_settings; // for gui to edit (main)
	GeneralSettings t_settings;   // for terrains to use (copy)

	private:
	void SaveSettingsToFile ();
	void LoadSettingsFromFile ();

	void StopActiveJobs ();

	std::shared_ptr<MeshData> terrainGridMesh;
	std::shared_ptr<VulkanModel> terrainGridModel;

	Resource::Texture::TexID terrainTextureArrayAlbedo;
	Resource::Texture::TexID terrainTextureArrayRoughness;
	Resource::Texture::TexID terrainTextureArrayMetallic;
	Resource::Texture::TexID terrainTextureArrayNormal;

	bool recreateTerrain = true;
	float nextTerrainWidth = 1000;
	SimpleTimer terrainUpdateTimer;

	std::vector<std::string> terrainTextureFileNames = {
		"dirt.jpg",
		"grass.jpg",
		"rockSmall.jpg",
		"Snow.jpg",
	};
};
