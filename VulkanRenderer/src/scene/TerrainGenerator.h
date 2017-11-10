#include <noise\basictypes.h>
#include <noiseutils.h>

#include <glm\common.hpp>

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  

#include "..\FastNoiseSIMD\FastNoiseSIMD.h"
#include "..\gui\TerGenNodeGraph.h"

struct BiomeColor {
	float r;
	float g;
	float b;

	BiomeColor() : r(1.0), g(0.0), b(1.0) {};
	BiomeColor(float r, float g, float b) :r(r), g(g), b(b) {};

	inline float GetColorChannel(int channel) {
		if (channel == 0)
			return r;
		if (channel == 1)
			return g;
		return b;
	}
};

#pragma once
class TerrainGenerator
{
public:
	TerrainGenerator(int numCells, int splatMapSize, glm::vec3 pos, glm::vec3 size);
	~TerrainGenerator();

	float SampleHeight(float x, float y, float z);
	float SampleColor(int channel, float x, float y, float z);
	float GetBiomeColor(int channel, float rainVal, float tempVal, float elevation);

	utils::Image* getImagePtr();

private:
	module::RidgedMulti mountainTerrain;
	module::Billow baseFlatTerrain;
	module::ScaleBias flatTerrainScaleBias;
	module::Perlin hillTerrain;
	module::ScaleBias hillTerrainScaleBias;

	module::Perlin hillMountainType;
	module::Perlin flatHillMountainType;

	noise::module::Select mountainHillSelector;
	noise::module::Select hillMountainFlatSelector;
	
	noise::module::Turbulence finalTerrain;

	noise::utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	utils::RendererImage renderer;
	utils::Image image;

	noise::module::Select biomeSelector;
	noise::module::Voronoi rainFallMap;
	noise::module::Voronoi temperatureMap;
	noise::module::Perlin elevationMap;

	NewNodeGraph::TerGenNodeGraph* nodeGraph;

	/*
	tropical forest
	temperate forest
	taiga
	tundra
	temperate grassland
	savanna
	desert
	*/

	struct BiomeColors {
		const BiomeColor Tundra = BiomeColor(1.0f, 1.0f, 1.0f);
		const BiomeColor Bare = BiomeColor(0.8f, 0.8f, 0.8f);
		const BiomeColor SeaFloor = BiomeColor(0.71f, 0.5f, 0.3f);
		const BiomeColor Desert = BiomeColor(0.95f, 0.88f, 0.46f);
		const BiomeColor GrassLand = BiomeColor(0.47f, 0.69f, 0.3f);
		const BiomeColor Savannah = BiomeColor(0.88f, 0.69f, 0.24f);
		const BiomeColor Forrest = BiomeColor(0.22f, 0.47f, 0.25f);
		const BiomeColor Tropical = BiomeColor(0.1f, 0.95f, 0.0f);
		const BiomeColor Snow = BiomeColor(0.95f, 0.95f, 0.95f);
		const BiomeColor Taiga = BiomeColor(0.29f, 0.43f, 0.31f);
	} bColors;

	BiomeColor seaFloot = bColors.SeaFloor;
	BiomeColor snowCap = bColors.Snow;

	BiomeColor temp_rain_chart[4][6] = { 
		{ bColors.Bare, bColors.Bare, bColors.Tundra, bColors.Tundra, bColors.Snow, bColors.Snow, },
		{ bColors.Taiga, bColors.Taiga, bColors.GrassLand, bColors.GrassLand, bColors.Savannah, bColors.Savannah,  },
		{ bColors.Desert, bColors.Savannah, bColors.Savannah, bColors.Bare, bColors.Savannah, bColors.Tropical,  },
		{ bColors.Desert, bColors.Desert, bColors.Desert, bColors.Tropical, bColors.Tropical, bColors.Tropical,  } };
};

class FastTerrainGenerator
{
public:
	FastTerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size);
	~FastTerrainGenerator();

private:
	NewNodeGraph::TerGenNodeGraph* nodeGraph;
	float* noiseSet;

};

