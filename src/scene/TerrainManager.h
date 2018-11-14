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

#include "core/JobSystem.h"
#include "core/CoreTools.h"
#include "core/TimeManager.h"

#include "util/ConcurrentQueue.h"
#include "util/MemoryPool.h"

#include "rendering/Renderer.h"

#include "resources/ResourceManager.h"

#include "gui/InternalGraph.h"

#include "Camera.h"
#include "Terrain.h"

#include "InstancedSceneObject.h"

constexpr size_t vert_size = sizeof(TerrainMeshVertices);
constexpr size_t ind_size = sizeof(TerrainMeshIndices);
constexpr int MaxChunkCount = 2048;

struct GeneralSettings {
	bool show_terrain_manager_window = true;
	float width = 1000;
	float heightScale = 100.0f;
	int maxLevels = 4;
	int gridDimentions = 1;
	int viewDistance = 1; //terrain chunks to load away from camera;
	int sourceImageResolution = 256;
	int numCells = 64; //compile time currently
	int workerThreads = 1;
};

struct TerrainTextureNamedHandle {
	std::string name;
	Resource::Texture::TexID handle;

	TerrainTextureNamedHandle(std::string name, Resource::Texture::TexID handle) :
		name(name), handle(handle) {}
};

struct TerrainCreationData {
	int numCells; int maxLevels;
	int sourceImageResolution;
	float heightScale;
	TerrainCoordinateData coord;

	TerrainCreationData(
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

	TerrainChunkBuffer(VulkanRenderer& renderer, int count,
		TerrainManager& man);
	~TerrainChunkBuffer();

	int Allocate();
	void Free(int index);

	void UpdateChunks();

	int ActiveQuadCount();

	ChunkState GetChunkState(int index);
	void SetChunkWritten(int index);

	Signal GetChunkSignal(int index);

	TerrainMeshVertices* GetDeviceVertexBufferPtr(int index);
	TerrainMeshIndices* GetDeviceIndexBufferPtr(int index);

	VulkanBufferVertex vert_buffer;
	VulkanBufferIndex index_buffer;

	TerrainManager& man;

private:
	std::mutex lock;

	VulkanRenderer& renderer;

	VulkanBufferData vert_staging;
	TerrainMeshVertices* vert_staging_ptr;

	VulkanBufferData index_staging;
	TerrainMeshIndices* index_staging_ptr;

	std::vector<ChunkState> chunkStates;
	std::vector<Signal> chunkReadySignals;

	std::atomic_int chunkCount = 0;
};

class TerrainManager
{
public:
	TerrainManager(InternalGraph::GraphPrototype& protoGraph,
		Resource::AssetManager& resourceMan, VulkanRenderer& renderer);
	~TerrainManager();

	void CleanUpTerrain();

	//void GenerateTerrain(std::shared_ptr<Camera> camera);

	void UpdateTerrains(glm::vec3 cameraPos);

	void RenderDepthPrePass(VkCommandBuffer commandBuffer);
	void RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe);

	void UpdateTerrainGUI();
	void DrawTerrainTextureViewer();

	float GetTerrainHeightAtLocation(float x, float z);

	Resource::AssetManager& resourceMan;
	VulkanRenderer& renderer;

	TerrainChunkBuffer chunkBuffer;

	std::shared_ptr<job::TaskSignal> workContinueSignal;

	bool isCreatingTerrain = true; //while condition for worker threads

	std::mutex terrain_mutex;
	std::vector<std::unique_ptr<Terrain>> terrains;
	std::vector<glm::i32vec2> activeTerrains;
	glm::vec3 curCameraPos;
	InternalGraph::GraphPrototype& protoGraph;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic;
	std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal;

	GeneralSettings settings;

	std::unique_ptr<InstancedSceneObject> instancedWaters;

private:

	void SaveSettingsToFile();
	void LoadSettingsFromFile();

	void StopActiveJobs();

	std::shared_ptr<MeshData> WaterMesh;

	Resource::Texture::TexID terrainTextureArrayAlbedo;
	Resource::Texture::TexID terrainTextureArrayRoughness;
	Resource::Texture::TexID terrainTextureArrayMetallic;
	Resource::Texture::TexID terrainTextureArrayNormal;

	std::vector<std::thread> terrainCreationWorkers;

	bool recreateTerrain = true;
	float nextTerrainWidth = 1000;
	SimpleTimer terrainUpdateTimer;

	int maxNumQuads = 1; //maximum quads managed by this

	bool drawWindow;
	int selectedTexture;

	int WorkerThreads = 6;


	std::vector<TerrainTextureNamedHandle> terrainTextureHandles;

	std::vector<std::string> terrainTextureFileNames = {
		"dirt.jpg",
		"grass.jpg",
		"rockSmall.jpg",
		"Snow.jpg",
	};

};

