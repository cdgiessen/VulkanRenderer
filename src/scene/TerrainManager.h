#pragma once

#include <thread>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <unordered_map>

//#include <foonathan/memory/container.hpp> // vector, list, list_node_size
//#include <foonathan/memory/memory_pool.hpp> // memory_pool

#include "../core/CoreTools.h"
#include "../core/TimeManager.h"

#include "../util/ConcurrentQueue.h"
#include "../util/MemoryPool.h"

#include "../rendering/Renderer.h"

#include "../resources/ResourceManager.h"

#include "../gui/InternalGraph.h"

#include "Camera.h"
#include "Terrain.h"

#include "InstancedSceneObject.h"

constexpr size_t vert_size = sizeof(TerrainMeshVertices);
constexpr size_t ind_size = sizeof(TerrainMeshIndices);
constexpr int MaxChunkCount = 1024;

struct GeneralSettings {
	bool show_terrain_manager_window = true;
	float width = 1000;
	float heightScale = 100.0f;
	int maxLevels = 4;
	int gridDimentions = 1;
	int viewDistance = 2; //terrain chunks to load away from camera;
	int sourceImageResolution = 512;
	int numCells = 64; //compile time currently
	int workerThreads = 1;
};

struct TerrainTextureNamedHandle {
	std::string name;
	std::shared_ptr<Texture> handle;

	TerrainTextureNamedHandle(std::string name, std::shared_ptr<Texture> handle) :
		name(name), handle(handle) {}
};

struct TerrainCreationData {
	ResourceManager* resourceMan;
	VulkanRenderer* renderer;
	glm::vec3 cameraPos;
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray;
	InternalGraph::GraphPrototype& protoGraph;
	int numCells; int maxLevels;
	int sourceImageResolution;
	float heightScale;
	TerrainCoordinateData coord;

	TerrainCreationData(
		ResourceManager* resourceMan,
		VulkanRenderer* renderer,
		glm::vec3 cameraPos,
		std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
		InternalGraph::GraphPrototype& protoGraph,
		int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord);
};

class TerrainManager;

class TerrainChunkBuffer {
public:

	enum class ChunkState {
		free,
		allocated,
		written,
		ready,
	};

	TerrainChunkBuffer(VulkanRenderer* renderer, int count,
		TerrainManager* man);
	~TerrainChunkBuffer();

	int Allocate();
	void Free(int index);

	void UpdateChunks();

	int ActiveQuadCount();

	ChunkState GetChunkState(int index);
	void SetChunkWritten(int index);

	TerrainMeshVertices* GetDeviceVertexBufferPtr(int index);
	TerrainMeshIndices* GetDeviceIndexBufferPtr(int index);

	VulkanBufferVertex vert_buffer;
	VulkanBufferIndex index_buffer;

	TerrainManager* man;

private:
	std::mutex lock;

	VulkanRenderer* renderer;

	VulkanBufferData vert_staging;
	TerrainMeshVertices* vert_staging_ptr;
	VulkanBufferData index_staging;
	TerrainMeshIndices* index_staging_ptr;

	std::vector<ChunkState> chunkStates;

	std::atomic_int chunkCount = 0;
};

class TerrainManager
{
public:
	TerrainManager(InternalGraph::GraphPrototype& protoGraph, ResourceManager* resourceMan, VulkanRenderer* renderer);
	~TerrainManager();

	void CleanUpResources();
	void CleanUpTerrain();

	void GenerateTerrain(ResourceManager* resourceMan, VulkanRenderer* renderer, std::shared_ptr<Camera> camera);

	void UpdateTerrains(ResourceManager* resourceMan, glm::vec3 cameraPos);

	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);

	void UpdateTerrainGUI();
	void DrawTerrainTextureViewer();

	float GetTerrainHeightAtLocation(float x, float z);

	void RecreateTerrain();

	VulkanRenderer* renderer;

	TerrainChunkBuffer chunkBuffer;

	std::mutex workerMutex;
	std::condition_variable workerConditionVariable;

	ConcurrentQueue<TerrainCreationData> terrainCreationWork;

	bool isCreatingTerrain = true; //while condition for worker threads

	std::mutex terrain_mutex;
	std::vector<std::unique_ptr<Terrain>> terrains;
	std::vector<glm::i32vec2> activeTerrains;

private:

	void SaveSettingsToFile();
	void LoadSettingsFromFile();

	void StartWorkerThreads();
	void StopWorkerThreads();

	InternalGraph::GraphPrototype& protoGraph;

	std::unique_ptr<InstancedSceneObject> instancedWaters;

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

	int WorkerThreads = 2;

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

