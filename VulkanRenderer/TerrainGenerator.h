#include <noise\basictypes.h>
#include <noiseutils.h>

#include <glm\common.hpp>

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

	noise::module::Select finalTerrain;
	noise::utils::NoiseMap heightMap;
	utils::NoiseMapBuilderPlane heightMapBuilder;
	utils::RendererImage renderer;
	utils::Image image;
};

