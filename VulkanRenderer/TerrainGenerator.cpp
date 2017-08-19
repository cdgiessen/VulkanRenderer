#include "TerrainGenerator.h"



TerrainGenerator::TerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size)
{
	//module::RidgedMulti mountainTerrain;
	mountainTerrain.SetFrequency(0.002);

	//module::Billow baseFlatTerrain;
	baseFlatTerrain.SetFrequency(0.002);

	//module::ScaleBias flatTerrain;
	flatTerrain.SetSourceModule(0, baseFlatTerrain);
	flatTerrain.SetScale(0.5);
	flatTerrain.SetBias(1.1);

	//module::Perlin terrainType;
	terrainType.SetFrequency(0.001);
	terrainType.SetPersistence(0.5);

	finalTerrain.SetSourceModule(0, flatTerrain);
	finalTerrain.SetSourceModule(1, mountainTerrain);
	finalTerrain.SetControlModule(terrainType);
	finalTerrain.SetBounds(0.0, 1000);
	finalTerrain.SetEdgeFalloff(0.125);

	heightMapBuilder.SetSourceModule(finalTerrain);
	heightMapBuilder.SetDestNoiseMap(heightMap);
	heightMapBuilder.SetDestSize(splatMapSize, splatMapSize);
	heightMapBuilder.SetBounds(pos.x, pos.x + size.x + size.x / (float)splatMapSize, pos.z, pos.z + size.z + size.z / (float)splatMapSize);
	heightMapBuilder.Build();

	/*
	"DeadSpruceTreeTrunk.png",
	"Sand.png",
	"SpruceTreeLeaf.png",
	"Rock.png",
	"Snow.png",
	*/
	
	renderer.SetSourceNoiseMap(heightMap);
	renderer.SetDestImage(image);
	renderer.ClearGradient();

	renderer.AddGradientPoint(-1.000, utils::Color(255, 0, 0, 32));
	renderer.AddGradientPoint(0.0000, utils::Color(255, 0, 0, 255));
	renderer.AddGradientPoint(0.1000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.4000, utils::Color(0, 255, 0, 255));
	renderer.AddGradientPoint(0.5000, utils::Color(255, 0, 0, 255));
	renderer.AddGradientPoint(0.6000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(0.8000, utils::Color(0, 0, 255, 255));
	renderer.AddGradientPoint(1.0000, utils::Color(0, 0, 0, 255));

	renderer.Render();
}


TerrainGenerator::~TerrainGenerator()
{
}

utils::Image* TerrainGenerator::getImagePtr() {
	return &image;
}

float TerrainGenerator::SampleHeight(float x, float y, float z) {
	return finalTerrain.GetValue(x, y, z);
}