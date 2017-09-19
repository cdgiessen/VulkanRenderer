#include <noise\basictypes.h>
#include <noiseutils.h>

#include <glm\common.hpp>

#include "FastNoiseSIMD\FastNoiseSIMD.h"


#pragma once
class TerrainGenerator
{
public:
	TerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size);
	~TerrainGenerator();

	float SampleHeight(float x, float y, float z);

	utils::Image* getImagePtr();

private:
	module::RidgedMulti mountainTerrain;
	module::Billow baseFlatTerrain;
	module::ScaleBias flatTerrain;


	module::Perlin terrainType;

	noise::module::Select terrainSelector;

	noise::module::Turbulence finalTerrain;

	noise::utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	utils::RendererImage renderer;
	utils::Image image;
};

class FastTerrainGenerator
{
public:
	FastTerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size);
	~FastTerrainGenerator();

	//float* SampleHeight(float x, float y, float z);

private:
	FastNoiseSIMD* myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
	float* noiseSet;

};

