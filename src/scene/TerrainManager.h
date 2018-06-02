#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>

#include "../core/CoreTools.h"
#include "../core/TimeManager.h"

#include "../util/ConcurrentQueue.h"
#include "../util/MemoryPool.h"

#include "../rendering/Renderer.h"

#include "../resources/ResourceManager.h"

#include "../gui/InternalGraph.h"

#include "Camera.h"
#include "Terrain.h"
#include "Water.h"

#include "InstancedSceneObject.h"

struct GeneralSettings {
	bool show_terrain_manager_window = true;
	float width = 1000;
	float heightScale = 100.0f;
	int maxLevels = 4;
	int gridDimentions = 1;
	int viewDistance = 4; //terrain chunks to load away from camera;
	int sourceImageResolution = 512;
	int numCells = 64; //compile time currently
};

struct TerrainTextureNamedHandle {
	std::string name;
	std::shared_ptr<Texture> handle;

	TerrainTextureNamedHandle(std::string name, std::shared_ptr<Texture> handle) : name(name), handle(handle) {}
};

struct TerrainCreationData {
	std::shared_ptr<ResourceManager> resourceMan;
	std::shared_ptr<VulkanRenderer> renderer;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray;
	std::shared_ptr<MemoryPool<TerrainQuad>> pool;
	InternalGraph::GraphPrototype& protoGraph;
	int numCells; int maxLevels;
	int sourceImageResolution;
	float heightScale;
	TerrainCoordinateData coord;

	TerrainCreationData(
		std::shared_ptr<ResourceManager> resourceMan,
		std::shared_ptr<VulkanRenderer> renderer,
		std::shared_ptr<Camera> camera,
		std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
		std::shared_ptr<MemoryPool<TerrainQuad>> pool,
		InternalGraph::GraphPrototype& protoGraph,
		int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord);
};

class TerrainManager
{
public:
	TerrainManager(InternalGraph::GraphPrototype& protoGraph);
	~TerrainManager();

	void SetupResources(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer);
	void CleanUpResources();

	void GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera);

	void UpdateTerrains(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager);

	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);

	void UpdateTerrainGUI();
	void DrawTerrainTextureViewer();

	void CleanUpTerrain();

	void ResetWorkerThreads();

	float GetTerrainHeightAtLocation(float x, float z);

	void RecreateTerrain();

	std::mutex workerMutex;
	std::condition_variable workerConditionVariable;

	std::mutex creationDataQueueMutex;
	ConcurrentQueue<TerrainCreationData> terrainCreationWork;

	bool isCreatingTerrain = true; //while condition for worker threads

	std::mutex terrain_mutex;
	std::vector<std::shared_ptr<Terrain>> terrains;

private:

	void SaveSettingsToFile();
	void LoadSettingsFromFile();



	InternalGraph::GraphPrototype& protoGraph;
	//NewNodeGraph::TerGenNodeGraph& nodeGraph;

	std::shared_ptr<VulkanRenderer> renderer;

	std::shared_ptr<MemoryPool<TerrainQuad>> terrainQuadPool;

	std::unique_ptr<InstancedSceneObject> instancedWaters;

	std::vector<std::shared_ptr<Water>> waters;

	std::shared_ptr<Mesh> WaterMesh;
	std::shared_ptr<VulkanModel> WaterModel;

	std::shared_ptr<TextureArray> terrainTextureArray;
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray;

	std::shared_ptr<Texture> WaterTexture;
	std::shared_ptr<VulkanTexture2D> WaterVulkanTexture;

	std::vector<std::thread> terrainCreationWorkers;

	GeneralSettings settings;
	bool recreateTerrain = false;
	float nextTerrainWidth = 1000;
	SimpleTimer terrainUpdateTimer;

	int maxNumQuads = 1; //maximum quads managed by this

	bool drawWindow;
	int selectedTexture;

	int WorkerThreads = 0;

	std::vector<TerrainTextureNamedHandle> terrainTextureHandles;

	std::vector<std::string> terrainTextureFileNames = {
		"dirt.jpg",
		"grass.jpg",
		"rockSmall.jpg",
		"Snow.jpg",
	};
	// Extra splatmap images, starting with just 4 for now
	//	"OakTreeLeaf.png",
	//	"Sand.png",
	//	"DeadOakTreeTrunk.png",
	//	"OakTreeTrunk.png",
	//	"SpruceTreeTrunk.png"};
	//

};

