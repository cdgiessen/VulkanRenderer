#include "TerrainGenerator.h"

#include <iostream>

TerrainGenerator::TerrainGenerator(int splatMapSize, int numCells, glm::vec3 pos, glm::vec3 size)
{
	//module::RidgedMulti mountainTerrain;
	mountainTerrain.SetFrequency(0.001);
	mountainTerrain.SetOctaveCount(6);
	
	//module::Billow baseFlatTerrain;
	baseFlatTerrain.SetFrequency(0.001);
	baseFlatTerrain.SetOctaveCount(6);
	
	//module::ScaleBias flatTerrain;
	flatTerrain.SetSourceModule(0, baseFlatTerrain);
	flatTerrain.SetScale(0.5);
	flatTerrain.SetBias(1.1);

	//module::Perlin terrainType;
	terrainType.SetFrequency(0.001);
	terrainType.SetPersistence(0.5);


	//terrain selector
	terrainSelector.SetSourceModule(0, flatTerrain);
	terrainSelector.SetSourceModule(1, mountainTerrain);
	terrainSelector.SetControlModule(terrainType);
	terrainSelector.SetBounds(0.0, 1000);
	terrainSelector.SetEdgeFalloff(0.125);

	//turbulence
	finalTerrain.SetSourceModule(0, terrainSelector);
	finalTerrain.SetFrequency(4.0);
	finalTerrain.SetPower(0.125);

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